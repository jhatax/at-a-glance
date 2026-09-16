from __future__ import annotations

from abc import ABC
from dataclasses import dataclass, field
from pathlib import Path
from queue import Queue
from textwrap import fill
from threading import Thread
from typing import Any, cast

from pebbleadapter import PebbleAdapter, PebbleEmulatorConnection
from qaharnessruntime import (
    ANSI_CYAN,
    ANSI_GREEN,
    ANSI_RED,
    ANSI_RESET,
    ConsolidatedQARunOutputs,
    HarnessRuntime,
    QAStepOutput,
    RawStepResult,
    ScreenshotsContext,
    create_harness_runtime,
)
from qaplanresolver import PlanDefinition, PlanStep


@dataclass
class PlanExecutionState:
  runtime: HarnessRuntime
  plan: PlanDefinition
  results_q: Queue[ToProcess]
  step_results: list[RawStepResult] = field(default_factory=list)
  step_outputs: list[QAStepOutput] = field(default_factory=list)
  pebble: PebbleAdapter = field(init=False)

  def __post_init__(self) -> None:
    # Create a singleton PebbleAdapter
    self.pebble = PebbleAdapter(lambda msg: self.log_adapter_message(msg))

  def log_adapter_message(
      self,
      message: str,
      log_only: bool = False,
      text_color: str = ANSI_CYAN,
  ) -> None:
    from textwrap import fill
    self.results_q.put(QAHarnessUpdate(fill(f"{message}\n", width=80, subsequent_indent=" ")))

  def inform_operator(
      self,
      message: str,
      log_only: bool = False,
      text_color: str = "",
  ) -> None:
    from textwrap import fill
    formatted = fill(f"{message}\n", width=80, subsequent_indent=" ")
    if not log_only:
      print(f"{text_color}{formatted}{ANSI_RESET}" if text_color else message)
    with self.runtime.commands_log_path.open("a", encoding="utf-8") as handle:
      handle.write(message)
      handle.flush()

  def _missing_outputs(self) -> list[str]:
    required = ["root", "commands_log"]
    outputs = self.runtime.as_dict()

    if self.plan.expected_screenshots > 0:
      required.append("screenshots_dir")

    missing: list[str] = []
    for key in required:
      path = outputs[key]
      if not Path(path).exists():
        missing.append(path)

    return missing

  def build_step_outputs(self) -> int:
    results_by_identity = {str(item["step_result_id"]): item for item in self.step_results}
    passed: int = 0

    for step in self.plan.steps.values():
      step_id = step.step_id
      result = results_by_identity.get(step_id, None)
      if result is None:
        result = RawStepResult(
            step_result_id=step_id,
            status="failed",
            screenshot_paths=[],
        )
      step_fields: dict[str, Any] = step.as_dict()
      row: QAStepOutput = QAStepOutput(
          step_id=step_id,
          capability=step.capability,
          status=result["status"],
          emulator=step.emulator,
          step_args={
              key: value
              for key, value in step_fields.items() if key.strip() not in {"emulator"}
          },
          screenshot_ctx=ScreenshotsContext(
              expected=step.expected_screenshots,
              captured=step.captured_screenshots,
              paths=result["screenshot_paths"],
          ),
      )
      passed += (row.status == "passed")
      self.step_outputs.append(row)

    return passed

  def finalize(self) -> int:
    # Sort outputs by step_number
    self.step_results.sort(key=lambda step_result: step_result["step_number"])
    # Create outputs
    passed_steps = self.build_step_outputs()

    _passed = (
        (passed_steps == self.plan.step_count) and (self.plan.step_count == len(self.step_outputs))
    )

    if _passed:
      _passed = not self._missing_outputs() and \
        (self.plan.expected_screenshots == self.plan.captured_screenshots)

    outputs = ConsolidatedQARunOutputs(
        plan=self.plan.name,
        output_folder=self.runtime.output_root,
        step_count=self.plan.step_count,
        step_outputs=self.step_outputs,
        status="passed" if _passed else "failed",
        started_at=self.runtime.started_at,
        run_outputs=self.runtime.as_dict(),
        resolved={
            "expected_screenshots": self.plan.expected_screenshots,
            "captured_screenshots": self.plan.captured_screenshots,
        },
        inform_operator=lambda msg: self.inform_operator(message=msg)
    )
    outputs.emit_reports(
        self.runtime.commands_log_path,
        self.runtime.report_json_path,
        self.runtime.report_md_path,
    )
    return int(not _passed) # exit_status is the inverse of True False in zsh


@dataclass
class ToProcess(ABC):
  type: str = field(init=False)


@dataclass
class ConcurrentStepResult(ToProcess):
  result: RawStepResult
  logs: list[str] = field(default_factory=list)
  type: str = field(default="Concurrent Step Result", init=False)


@dataclass
class QAHarnessUpdate(ToProcess):
  message: str
  type: str = field(default="Pebble Log", init=False)


@dataclass
class NoMoreResults(ToProcess):
  type: str = field(default="Done", init=False)


@dataclass
class EmulatorExecutionState:
  emulator: str
  pbw_path: Path
  output_root: Path
  screenshots_dir: Path
  steps: list[PlanStep]
  results_q: Queue[ToProcess]
  pebble: PebbleAdapter


def capture_screenshot(
    emu_state: EmulatorExecutionState,
    connection: PebbleEmulatorConnection,
    step_output: RawStepResult,
) -> None:
  step_result_id = step_output["step_result_id"]
  screenshot_number = len(step_output["screenshot_paths"]) + 1
  suffix = f"-{screenshot_number}" if screenshot_number > 1 else ""
  filename = (f"{step_result_id}{suffix}.png")
  screenshot_file = emu_state.screenshots_dir / filename
  connection.screenshot(screenshot_file)
  step_output["screenshot_paths"].append(str(screenshot_file))


def _execute_emulator_step(
    emu_state: EmulatorExecutionState,
    step: PlanStep,
) -> ConcurrentStepResult:
  logs: list[str] = []
  step_result: RawStepResult = {
      "step_result_id": step.step_id,
      "status": "running",
      "screenshot_paths": [],
      "step_number": step.step_number,
  }
  divider = "=" * 80
  logs.append(
      f"{divider}\n-- Attempting {step.step_number}/'{step.step_id}', "
      f"Emulator: '{step.emulator}'\nType: '{step.capability}'\nArguments: '{step.as_dict()}'"
  )
  logs.append(divider)

  restart_required = False
  try:
    if emu_state.emulator != step.emulator:
      raise ValueError(
          f"Step: '{step.step_id}' targeting emulator: '{step.emulator}' "
          f"routed to emulator: {emu_state.emulator}"
      )

    with emu_state.pebble.create_connection(step.emulator, logs) as connection:
      restart_required = step.run(
          connection,
          lambda: capture_screenshot(emu_state, connection, step_result),
      )
    if restart_required:
      emu_state.pebble.restart_emulator(step.emulator)
  except Exception as err: # noqa: BLE001
    # Mark this step as failed but don't propagate the error.
    # Handling the error here continues plan execution.
    step_result["status"] = "failed"
    logs.append(f"Encountered issue: '{err}'")
    step.captured_screenshots = len(step_result["screenshot_paths"])
  else:
    # There was no exception
    step.captured_screenshots = len(step_result["screenshot_paths"])
    if (not step.capture_screenshots) or (step.expected_screenshots == step.captured_screenshots):
      step_result["status"] = "passed"
  finally:
    # We got here without setting any status means the step failed for an uncaught reason
    if step_result["status"] == "running":
      step_result["status"] = "failed"

  # Output result during processing to associate colors with status
  return ConcurrentStepResult(
      result=step_result,
      logs=logs,
  )


def execute_emulator_steps(emu_state: EmulatorExecutionState) -> None:
  for step in emu_state.steps:
    emu_state.results_q.put(_execute_emulator_step(emu_state, step))


def process_step_results(plan_state: PlanExecutionState, results_q: Queue[ToProcess]) -> None:
  while True:
    result = results_q.get()
    try:
      match result:
        case NoMoreResults():
          break
        case QAHarnessUpdate():
          plan_state.inform_operator(
              fill(cast(QAHarnessUpdate, result).message, width=80, subsequent_indent=" ")
          )
        case ConcurrentStepResult():
          # Process the result
          # 1. Increment captured_screenshots
          csresult = cast(ConcurrentStepResult, result)
          plan_state.plan.captured_screenshots += len(csresult.result["screenshot_paths"])
          # 2. Output logs
          plan_state.inform_operator(
              "\n".join(fill(log, width=80, subsequent_indent=" ") for log in csresult.logs),
          )
          # 3. Output result
          result = csresult.result
          color = ANSI_GREEN if result["status"] == "passed" else ANSI_RED
          plan_state.inform_operator(
              message=fill(
                  f"Outcome: '{result["status"]}'",
                  width=80,
                  subsequent_indent=" ",
              ),
              log_only=True,
              text_color=color
          )
          # 4. Save the step's result to be processed during finalize
          plan_state.step_results.append(result)
    finally:
      results_q.task_done()


def stratify_steps_by_emulator(steps: dict[str, PlanStep]) -> dict[str, list[PlanStep]]:
  emulator_queues: dict[str, list[PlanStep]] = {}
  for index, step in enumerate(steps.values(), start=1):
    step.step_number = index
    emulator = step.emulator
    if emulator not in emulator_queues:
      emulator_queues[emulator] = []
    emulator_queues[step.emulator].append(step)
  return emulator_queues


def execute_plan_concurrently(plan: PlanDefinition) -> int:
  # If there are no steps to execute
  if not plan.steps:
    return 0

  from datetime import datetime
  emulator_queues = stratify_steps_by_emulator(plan.steps)
  from qaharnessconfig import REPO_ROOT
  # Create Queue to receive ConcurrentResult objects from steps
  results_q: Queue[ToProcess] = Queue()
  TERMINUS = NoMoreResults()

  runtime = create_harness_runtime(plan.expected_screenshots > 0)
  divider = "=" * 80
  plan_state = PlanExecutionState(
      runtime=runtime,
      plan=plan,
      results_q=results_q,
  )

  # Create thread that processes results queued to the results_q
  results_thread = Thread(
      target=process_step_results,
      args=(plan_state, results_q),
      name="results-processor",
  )
  results_thread.start()

  # Create Threads
  # Pass each thread the right context and execution function
  threads: list[Thread] = []
  pbw_path = REPO_ROOT / "build" / "at-a-glance.pbw"
  for emulator, steps in emulator_queues.items():
    plan_state.pebble.install_emulator(emulator, pbw_path)
    emu_state = EmulatorExecutionState(
        emulator=emulator,
        pbw_path=pbw_path,
        output_root=runtime.output_root,
        screenshots_dir=runtime.screenshots_dir,
        steps=steps,
        results_q=results_q,
        pebble=plan_state.pebble,
    )
    thread = Thread(
        target=execute_emulator_steps,
        args=(emu_state, ),
        name=f"{emulator}-executor",
    )
    if thread:
      threads.append(thread)
      thread.start()
    else:
      raise RuntimeError(f"Thread couldn't be created to execute steps for '{emulator}'")

  # Join all threads before you exit / return; also wait for all logs to be written
  for thread in threads:
    thread.join()

  # Inform the results processor that there are no more results to process
  results_q.put(TERMINUS)

  # Wait for the results queue and thread to terminate gracefully
  results_q.join()
  results_thread.join()
  plan_state.inform_operator(message=divider)
  plan.captured_screenshots = sum(step.captured_screenshots for step in plan.steps.values())
  end = datetime.now().astimezone()
  start =datetime.strptime(plan_state.runtime.started_at, "%Y%m%dT%H%M%S").astimezone()
  minutes, seconds = divmod(int((end - start).total_seconds()), 60)
  plan_state.inform_operator(
    message=(
    f"Ended at: {end.strftime('%Y%m%dT%H%M%S')}"
    f"Time elapsed: {minutes}:{seconds:02d}"
    ),
    text_color=ANSI_CYAN,
  )
  plan_state.inform_operator(
    message=divider,
    text_color=ANSI_CYAN,
  )
  PebbleAdapter.kill_emulators(emulator_queues)
  return plan_state.finalize()


def resolve_and_execute_plan_concurrently(action: str, plan_name: str) -> int:
  from qaplanresolver import load_and_validate_plan

  plan = load_and_validate_plan(plan_name)
  if plan:
    if action == "run-scenario" or plan.discarded:
      print("Execute this plan? [y/N] ", end="", flush=True)
      reply = input().strip().lower()
      if reply in {"n", "no"}:
        print("Plan execution cancelled.")
        return 1

    return execute_plan_concurrently(plan)
  else:
    print("Plan validation failed.")
    return 1
