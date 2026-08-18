from __future__ import annotations

from dataclasses import dataclass, field
from typing import TYPE_CHECKING, Final
from qaharnessruntime import ANSI_CYAN, ANSI_RESET, HarnessRuntimeContext, StepResult, finalize
from qaplanresolver import PlanDefinition, PlanStep
from textwrap import fill

if TYPE_CHECKING:
  from pebbleadapter import PebbleAdapter

SCREENSHOT_DELAY_SECONDS: Final[int] = 1


@dataclass
class ExecutionState:
  context: HarnessRuntimeContext
  plan: PlanDefinition
  step_results: list[StepResult] = field(default_factory=list)
  pebble: PebbleAdapter = field(init=False)

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


def _capture_screenshot(
    state: ExecutionState,
    step_result: StepResult,
    emulator: str,
) -> None:

  step_result_id = str(step_result["step_result_id"])
  screenshot_number = len(step_result["screenshot_paths"]) + 1
  filename = (
      f"{step_result_id}.png"
      if screenshot_number == 1 else f"{step_result_id}-{screenshot_number}.png"
  )
  output_path = state.context.screenshots_dir / filename
  state.pebble.screenshot(emulator, output_path)
  step_result["screenshot_paths"].append(str(output_path))


def _execute_step(state: ExecutionState, step: PlanStep) -> None:
  result: StepResult = {
      "step_result_id": step.step_id,
      "status": "running",
      "screenshot_paths": [],
  }

  # step execution will write to the log or stdout / stderr
  restart_required = False
  try:
    with state.pebble.create_connection(step.emulator) as connection:
      restart_required = step.run(
          state.pebble,
          connection,
          lambda emulator: _capture_screenshot(state, result, emulator),
      )
    if restart_required:
      state.pebble.restart_emulator_after_bluetooth_disconnect(step.emulator)
  except Exception as err:
    result["status"] = "failed"
    state.inform_operator(f"Encountered issue: '{err}'")
    # continue running until all steps have executed
  else:
    step.captured_screenshots = len(result["screenshot_paths"])
    if (not step.capture_screenshots) or (step.expected_screenshots == step.captured_screenshots):
      result["status"] = "passed"
  finally:
    # an exception or error was raised
    if result["status"] == "running":
      result["status"] = "failed"
    state.step_results.append(result)
    _step = fill(f"'{step.as_dict()}'", width=80, subsequent_indent=" ")
    _result = fill(f"'{result}'", width=80, subsequent_indent=" ")
    state.inform_operator(f"Executed: {_step}\nResult: {_result}\n")


def run_plan_execution(plan: PlanDefinition) -> int:
  from qaharnessruntime import create_harness_context
  state = ExecutionState(create_harness_context(plan.expected_screenshots > 0), plan=plan)
  exit_status = 0

  divider = "=" * 80
  try:
    state.inform_operator(f"QA Plan to execute:\n{plan.as_dict()}\n", True)
    from qaharnessconfig import REPO_ROOT
    pbw_path = REPO_ROOT / "build" / "at-a-glance.pbw"
    emulators = {emulator for emulator, _display in plan.execution_configs}
    state.pebble.install_emulators(emulators, pbw_path)
    for index, step in enumerate(plan.steps.values(), start=1):
      header = f"--- Attempting step# {index}: Type: '{step.capability}' with id '{step.step_id}'"
      pre_step = f"{divider}\n{header}"
      state.inform_operator(pre_step, terminal_color=ANSI_CYAN)
      _execute_step(state, step)
  except Exception as exc:
    print(f"Error: {exc!r}")
    exit_status = 1
  finally:
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

    return run_plan_execution(plan)
  else:
    print("Plan validation failed.")
    return 1
