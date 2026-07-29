from __future__ import annotations

from dataclasses import asdict, dataclass, field
import time
from typing import TYPE_CHECKING, Final
from qaharnessruntime import HarnessRuntimeContext, StepResult, finalize
from qaplanresolver import BatteryStep, HealthStep, PlanDefinition, PlanStep, WeatherStep
from qaharnessconfig import DISPLAY_MODE_VALUES
from pebbleadapter import PEBBLE_SETTLE_DELAY_SECONDS

if TYPE_CHECKING:
  from pebbleadapter import PebbleAdapter

QA_MSG_TEMPERATURE: Final[int] = 10002
QA_MSG_WEATHER_CONDITION: Final[int] = 10003
QA_MSG_IS_DAY: Final[int] = 10004
QA_MSG_DISPLAY_MODE: Final[int] = 10006
QA_MSG_ONESHOT_BPM: Final[int] = 10020
QA_MSG_ONESHOT_STEPS: Final[int] = 10021
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

  def inform_operator(self, line: str, log_only: bool = False) -> None:
    message = f"{line}\n"
    if not log_only:
      print(message)
    with self.context.commands_log_path.open("a", encoding="utf-8") as handle:
      handle.write(message)


def _capture_screenshot(
    state: ExecutionState,
    step_result: StepResult,
    emulator: str,
) -> None:

  step_result_id = str(step_result["step_result_id"])
  output_path = state.context.screenshots_dir / f"{step_result_id}.png"
  # Sleep before capturing a screenshot as there could be a pebble lag
  time.sleep(SCREENSHOT_DELAY_SECONDS)
  state.pebble.screenshot(emulator, output_path)
  step_result["screenshot_paths"].append(str(output_path))


def _send_display_mode(state: ExecutionState, emulator: str, mode: str) -> None:
  state.pebble.send_app_message(emulator, {QA_MSG_DISPLAY_MODE: int(DISPLAY_MODE_VALUES[mode])})


def _send_weather_message(
    state: ExecutionState,
    emulator: str,
    temperature: int,
    weather_code: int,
    is_day: int,
    mode: str,
) -> None:
  mode_value = int(DISPLAY_MODE_VALUES[mode])
  state.pebble.send_app_message(
      emulator, {
          QA_MSG_TEMPERATURE: temperature,
          QA_MSG_WEATHER_CONDITION: weather_code,
          QA_MSG_IS_DAY: is_day,
          QA_MSG_DISPLAY_MODE: mode_value,
      }
  )


def _send_health_override(state: ExecutionState, emulator: str, bpm: int, steps: int) -> None:
  state.pebble.send_app_message(emulator, {
      QA_MSG_ONESHOT_BPM: bpm,
      QA_MSG_ONESHOT_STEPS: steps,
  })


def _set_battery_state(
    state: ExecutionState, emulator: str, percent: int, charging_state: int
) -> None:
  state.pebble.set_battery(emulator, percent, charging_state)


def _run_weather_step(state: ExecutionState, step: WeatherStep, result: StepResult) -> None:
  _send_weather_message(state, step.emulator, step.temp, step.code, step.is_day, step.display)
  if step.capture_screenshots:
    _capture_screenshot(state, result, step.emulator)
  time.sleep(PEBBLE_SETTLE_DELAY_SECONDS)


def _run_battery_step(state: ExecutionState, step: BatteryStep, result: StepResult) -> None:
  if step.display:
    _send_display_mode(state, step.emulator, step.display)
  time.sleep(PEBBLE_SETTLE_DELAY_SECONDS)
  _set_battery_state(state, step.emulator, step.level, step.charging)
  if step.capture_screenshots:
    _capture_screenshot(state, result, step.emulator)
  time.sleep(PEBBLE_SETTLE_DELAY_SECONDS)


def _run_health_step(state: ExecutionState, step: HealthStep, result: StepResult) -> None:
  if step.display:
    _send_display_mode(state, step.emulator, step.display)
  time.sleep(PEBBLE_SETTLE_DELAY_SECONDS)
  _send_health_override(state, step.emulator, step.bpm, step.steps)
  if step.capture_screenshots:
    _capture_screenshot(state, result, step.emulator)
  time.sleep(PEBBLE_SETTLE_DELAY_SECONDS)


def _execute_step(state: ExecutionState, step: PlanStep) -> None:
  from textwrap import fill

  result: StepResult = {
      "step_result_id": step.step_id,
      "status": "running",
      "screenshot_paths": [],
  }

  divider = "=" * 80
  header = f"--- Attempting step: Type: '{step.capability}' with id '{step.step_id}'"
  pre_step = f"{divider}\n{header}"
  state.inform_operator(pre_step)

  # step execution will write to the log or stdout / stderr
  # by logging the divider and header before we execute, there
  # is clean separation between steps executed
  try:
    match step:
      case WeatherStep():
        _run_weather_step(state, step, result)

      case BatteryStep():
        _run_battery_step(state, step, result)

      case HealthStep():
        _run_health_step(state, step, result)

      case _:
        raise ValueError(f"Unknown step instance type: {type(step)}")
  except (Exception, ValueError):
    result["status"] = "failed"
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
    _step = fill(f"'{asdict(step)}'", width=80, subsequent_indent=" ")
    _result = fill(f"'{result}'", width=80, subsequent_indent=" ")
    state.inform_operator(f"Executed: {_step}\nResult: {_result}\n{divider}")


def run_plan_execution(plan: PlanDefinition) -> int:
  from qaharnessruntime import create_harness_context
  state = ExecutionState(create_harness_context(plan.expected_screenshots > 0), plan=plan)
  exit_status = 0

  try:
    state.inform_operator(f"QA Plan to execute:\n{asdict(plan)}\n", True)
    from qaharnessconfig import REPO_ROOT
    pbw_path = REPO_ROOT / "build" / "at-a-glance.pbw"
    state.pebble.install_emulators(plan.emulators, pbw_path)
    for step in plan.steps.values():
      _execute_step(state, step)
  except (Exception, AssertionError) as exc:
    print(f"Error: {exc!r}")
    exit_status = 1
  finally:
    plan.captured_screenshots = sum(step.captured_screenshots for step in plan.steps.values())
    # this should always be true because this is done in _run_step's finalize:
    #   step.captured_screenshots = len(result["screenshot_paths"])

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
    if action == "run-scenario":
      print("Execute this plan? [y/N] ", end="", flush=True)
      reply = input().strip().lower()
      if reply in {"n", "no"}:
        print("Plan execution cancelled.")
        return 1

    return run_plan_execution(plan)
  else:
    print("Plan validation failed.")
    return 1
