from __future__ import annotations

from abc import ABC
from dataclasses import dataclass, field
from pathlib import Path
from queue import Queue
from textwrap import fill
from threading import Thread
from typing import TYPE_CHECKING, Final, TypedDict

from qaharnessruntime import ANSI_CYAN, ANSI_RESET, HarnessRuntimeContext, StepResult, finalize
from qaplanresolver import PlanDefinition, PlanStep

if TYPE_CHECKING:
  from pebbleadapter import PebbleAdapter

SCREENSHOT_DELAY_SECONDS: Final[int] = 1


@dataclass
class PlanExecutionState:
  context: HarnessRuntimeContext
  plan: PlanDefinition
  step_results: list[StepResult] = field(default_factory=list)
  pebble: PebbleAdapter = field(init=False)
  results_q: Queue[CompletionItem]

  def __post_init__(self) -> None:
    try:
      from pebbleadapter import PebbleAdapter
    except ImportError as exc:
      raise ValueError(
          "Pebble QA adapter unavailable: install or repair the Pebble Tool/libpebble2 environment."
      ) from exc

    self.pebble = PebbleAdapter(self.inform_operator)

  def inform_operator(
      self,
      line: str,
      log_only: bool = False,
      terminal_color: str = "",
  ) -> None:
    message = f"{line}\n"
    if not log_only:
      print(f"{terminal_color}{message}{ANSI_RESET}" if terminal_color else message)
    with self.context.commands_log_path.open("a", encoding="utf-8") as handle:
      handle.write(message)


def stratify_steps_by_emulator(steps: dict[str, PlanStep]) -> dict[str, list[PlanStep]]:
  emulator_queues: dict[str, list[PlanStep]] = {}
  for index, step in enumerate(steps.values(), start=1):
    step.step_number = index
    emulator = step.emulator
    if emulator not in emulator_queues:
      emulator_queues[emulator] = []
    emulator_queues[step.emulator].append(step)
  return emulator_queues


@dataclass
class CompletionItem(ABC):
  type: str = field(init=False)


@dataclass
class ConcurrentStepResult(CompletionItem):
  step: PlanStep
  result: StepResult
  logs: list[str]
  type: str = field(default="Concurrent", init=False)


@dataclass
class NoMoreResults(CompletionItem):
  type: str = field(default="Done", init=False)


class EmulatorExecutionState(TypedDict):
  emulator: str
  pbw_path: Path
  output_root: Path
  screenshots_dir: Path
  steps: list[PlanStep]
  results: Queue[ConcurrentStepResult]
  pebble: PebbleAdapter


def capture_screenshot(
    state: EmulatorExecutionState,
    step_result: StepResult,
) -> None:

  step_result_id = step_result["step_result_id"]
  screenshot_number = len(step_result["screenshot_paths"]) + 1
  filename = (
      f"{step_result_id}.png"
      if screenshot_number == 1 else f"{step_result_id}-{screenshot_number}.png"
  )
  screenshot_file = state["screenshots_dir"] / filename
  state["pebble"].screenshot(state["emulator"], screenshot_file)
  step_result["screenshot_paths"].append(str(screenshot_file))


def _execute_emulator_step(state: EmulatorExecutionState, step: PlanStep) -> ConcurrentStepResult:
  logs: list[str] = []
  result: StepResult = {
      "step_result_id": step.step_id,
      "status": "running",
      "screenshot_paths": [],
  }
  divider = "=" * 80
  logs.append(divider)
  logs.append(f"--- {step.step_number}/'{step.step_id}' on emulator: '{step.emulator}'")
  logs.append("Type: '{step.capability}'")
  logs.append(divider)

  restart_required = False
  try:
    if state["emulator"] != step.emulator:
      raise ValueError(
          f"Step: '{step.step_id}' targeting emulator: '{step.emulator}' "
          f"routed to emulator: {state['emulator']}"
      )

    with state["pebble"].create_connection(step.emulator) as connection:
      restart_required = step.run(
          state["pebble"],
          connection,
          lambda emulator: capture_screenshot(state, result),
      )
    if restart_required:
      state["pebble"].restart_emulator_after_bluetooth_disconnect(step.emulator)
  except Exception as err: # noqa: BLE001
    # Mark this step as failed but don't propagate the error.
    # Handling the error here continues plan execution.
    result["status"] = "failed"
    logs.append(f"Encountered issue: '{err}'")
  else:
    # There was no exception
    step.captured_screenshots = len(result["screenshot_paths"])
    if (not step.capture_screenshots) or (step.expected_screenshots == step.captured_screenshots):
      result["status"] = "passed"
  finally:
    # We got here without setting any status means the step failed for an uncaught reason
    if result["status"] == "running":
      result["status"] = "failed"
    logs.append(fill(f"'{step.as_dict()}'", width=80, subsequent_indent=" "))
    logs.append(fill(f"'{result}'", width=80, subsequent_indent=" "))
  return ConcurrentStepResult(
      step=step,
      result=result,
      logs=logs,
  )


def execute_emulator_steps(state: EmulatorExecutionState) -> None:
  try:
    for step in state["steps"]:
      state["results"].put(_execute_emulator_step(state, step))
  finally:
    # Need to build results objects
    state["pebble"].close()


def process_step_results(state: PlanExecutionState) -> None:
  results_q = state.results_q
  while True:
    result = results_q.get()
    try:
      if isinstance(result, NoMoreResults):
        break

      print("Got one")
      # Process the result
    finally:
      results_q.task_done()


def execute_plan_concurrently(plan: PlanDefinition) -> int:
  emulator_queues = stratify_steps_by_emulator(plan.steps)
  from qaharnessconfig import REPO_ROOT
  from qaharnessruntime import create_harness_context
  # Create Queue to receive ConcurrentResult objects from steps
  results_q: Queue[StepResult] = Queue()
  TERMINUS = NoMoreResults()

  state = PlanExecutionState(
      create_harness_context(plan.expected_screenshots > 0),
      plan=plan,
      results_q=results_q,
  )
  exit_status = 0
  divider = "=" * 80
  state.inform_operator(f"{divider}\nQA Plan to execute:\n{plan.as_dict()}\n", True)

  # Create Threads
  # Pass each thread the right context and execution function
  threads: list[Thread] = []
  pbw_path = REPO_ROOT / "build" / "at-a-glance.pbw"
  for emulator, steps in emulator_queues.items():
    state.pebble.install(emulator, pbw_path)
    em_state = EmulatorExecutionState(
        emulator=emulator,
        pbw_path=pbw_path,
        output_root=state.context.output_root,
        screenshots_dir=state.context.screenshots_dir,
        steps=steps,
        results=results_q,
        pebble=state.pebble,
    )
    thread = Thread(
        target=execute_emulator_steps,
        args=(em_state),
        name=f"{emulator}-execution",
    )
    if thread:
      threads.append(thread)
      thread.start()
    else:
      raise RuntimeError(f"Thread couldn't be created to execute steps for '{emulator}'")
  state.inform_operator(divider)
  # Join all threads before you exit / return; also wait for all logs to be written

  for thread in threads:
    thread.join()

  results_q.put(TERMINUS)

  results_q.join()
  # Compute exit_status
  state.inform_operator(f"{divider}")
  plan.captured_screenshots = sum(step.captured_screenshots for step in plan.steps.values())
  state.pebble.close()

  return finalize(
      context=state.context,
      plan=state.plan,
      exit_status=exit_status,
      step_results=state.step_results,
  )


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

    return execute_plan_concurrently(plan)
  else:
    print("Plan validation failed.")
    return 1
