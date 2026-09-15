from __future__ import annotations

from dataclasses import dataclass, field
from pathlib import Path
from textwrap import fill
from typing import Any

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
  step_results: list[RawStepResult] = field(default_factory=list)
  step_outputs: list[QAStepOutput] = field(default_factory=list)
  pebble: PebbleAdapter = field(init=False)

  def __post_init__(self) -> None:
    self.pebble = PebbleAdapter(lambda msg: self.inform_operator(message=msg))

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
      passed += 1 if (row.status == "passed") else 0
      self.step_outputs.append(row)

    return passed

  def finalize(self, exit_status: int) -> int:
    # Create outputs
    passed_steps = self.build_step_outputs()

    outputs_match_plan = (
        (passed_steps == self.plan.step_count) and (self.plan.step_count == len(self.step_outputs))
    )

    _passed = (exit_status == 0) and outputs_match_plan
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
    return int(_passed) # boolean False is interpreted as 0


def _capture_screenshot(
    screenshots_dir: Path,
    connection: PebbleEmulatorConnection,
    step_result: RawStepResult,
) -> None:

  step_result_id = str(step_result["step_result_id"])
  screenshot_number = len(step_result["screenshot_paths"]) + 1
  filename = (
      f"{step_result_id}.png"
      if screenshot_number == 1 else f"{step_result_id}-{screenshot_number}.png"
  )
  output_path = screenshots_dir / filename
  connection.screenshot(output_path)
  step_result["screenshot_paths"].append(str(output_path))


def _execute_step(plan_state: PlanExecutionState, step: PlanStep) -> None:
  step_result: RawStepResult = {
      "step_result_id": step.step_id,
      "status": "running",
      "screenshot_paths": [],
      "step_number": step.step_number,
  }
  logs: list[str] = []
  # step execution will write to the log or stdout / stderr
  restart_required = False
  try:
    with plan_state.pebble.create_connection(emulator=step.emulator, logs=logs) as connection:
      restart_required = step.run(
          connection,
          lambda: _capture_screenshot(
              plan_state.runtime.screenshots_dir,
              connection,
              step_result,
          ),
      )
    if restart_required:
      plan_state.pebble.restart_emulator(step.emulator)
  except Exception as err: # noqa: BLE001
    step_result["status"] = "failed"
    plan_state.inform_operator(f"Encountered issue: '{err}'")
    step.captured_screenshots = len(step_result["screenshot_paths"])
    # continue running until all steps have executed
  else:
    step.captured_screenshots = len(step_result["screenshot_paths"])
    if (not step.capture_screenshots) or (step.expected_screenshots == step.captured_screenshots):
      step_result["status"] = "passed"
  finally:
    # Inform the operator of what happened while executing the step
    plan_state.inform_operator(
        "\n".join(fill(log, width=80, subsequent_indent=" ") for log in logs),
    )
    # an exception or error was raised
    if step_result["status"] == "running":
      step_result["status"] = "failed"
    plan_state.step_results.append(step_result)
    color = ANSI_GREEN if step_result["status"] == "passed" else ANSI_RED
    plan_state.inform_operator(
        message=fill(
            f"Result: '{step_result}'",
            width=80,
            subsequent_indent=" ",
        ),
        text_color=color
    )


def execute_plan(plan: PlanDefinition) -> int:
  from datetime import datetime
  plan_state = PlanExecutionState(create_harness_runtime(plan.expected_screenshots > 0), plan=plan)
  exit_status = 0

  divider = "=" * 80
  try:
    plan_state.inform_operator(
        message=f"QA Plan to execute:\n{plan.as_dict()}\n",
        log_only=True,
        text_color=ANSI_CYAN,
    )
    from qaharnessconfig import REPO_ROOT
    emulators = {emulator for emulator, _ in plan.execution_configs}
    plan_state.pebble.install_emulators(emulators, REPO_ROOT / "build" / "at-a-glance.pbw")
    for index, step in enumerate(plan.steps.values(), start=1):
      step.step_number = index
      plan_state.inform_operator(divider)
      plan_state.inform_operator(
          f"-- Attempting {step.step_number}/'{step.step_id}', Emulator: '{step.emulator}'"
      )
      plan_state.inform_operator(f"Type: '{step.capability}'\nArguments: '{step.as_dict()}'")
      plan_state.inform_operator(divider)
      _execute_step(plan_state, step)
  except Exception as exc: # noqa: BLE001
    print(f"Error: {exc!r}")
    exit_status = 1
  finally:
    plan_state.inform_operator(f"{divider}")
    plan.captured_screenshots = sum(step.captured_screenshots for step in plan.steps.values())
  end = datetime.now().astimezone()
  start = datetime.strptime(plan_state.runtime.started_at, "%Y%m%dT%H%M%S").astimezone()
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
  PebbleAdapter.kill_emulators(emulators)
  return plan_state.finalize(exit_status=exit_status)


def resolve_and_execute_plan(action: str, plan_name: str) -> int:
  from qaplanresolver import load_and_validate_plan

  plan = load_and_validate_plan(plan_name)
  if plan:
    if action == "run-scenario" or plan.discarded:
      print("Execute this plan? [y/N] ", end="", flush=True)
      reply = input().strip().lower()
      if reply in {"n", "no"}:
        print("Plan execution cancelled.")
        return 1

    return execute_plan(plan)
  else:
    print("Plan validation failed.")
    return 1
