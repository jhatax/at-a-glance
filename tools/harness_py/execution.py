from __future__ import annotations

from dataclasses import dataclass, field
import sys
import time
from typing import TYPE_CHECKING, Final

from runtime import ExecutionContext, StepResult, finalize
from plans import DISPLAY_MODE_VALUES, PlanDefinition, PlanStep

if TYPE_CHECKING:
  from pebble import PebbleAdapter

QA_MSG_TEMPERATURE: Final[int] = 10002
QA_MSG_WEATHER_CONDITION: Final[int] = 10003
QA_MSG_IS_DAY: Final[int] = 10004
QA_MSG_DISPLAY_MODE: Final[int] = 10006
QA_MSG_ONESHOT_BPM: Final[int] = 10020
QA_MSG_ONESHOT_STEPS: Final[int] = 10021
SCREENSHOT_DELAY_SECONDS: Final[int] = 1


@dataclass
class ExecutionState:
  context: ExecutionContext
  plan: PlanDefinition
  step_results: list[StepResult] = field(default_factory=list)
  pebble: PebbleAdapter = field(init=False)

  def __post_init__(self) -> None:
    try:
      from pebble import PebbleAdapter
    except ImportError as exc:
      raise ValueError(
          "Pebble QA adapter unavailable: install or repair the Pebble Tool/libpebble2 environment."
      ) from exc

    self.pebble = PebbleAdapter(self._log)

  def _log(self, line: str) -> None:
    with self.context.commands_log_path.open("a", encoding="utf-8") as handle:
      handle.write(line + "\n")


def _capture_screenshot(
    state: ExecutionState,
    step_result: StepResult,
    emulator: str,
    phase: str = "",
) -> None:

  artifact_identity = str(step_result["name"])
  suffix = f"-{phase}" if phase else ""
  output_path = state.context.screenshots_dir / f"{artifact_identity}{suffix}.png"
  # Sleep before capturing a screenshot as there could be a pebble lag
  time.sleep(SCREENSHOT_DELAY_SECONDS)
  state.pebble.screenshot(emulator, output_path)
  step_result["screenshot_paths"].append(str(output_path))


def _send_display_mode(state: ExecutionState, emulator: str, mode: str) -> None:
  mode_value = int(DISPLAY_MODE_VALUES[mode])
  state.pebble.send_app_message(emulator, {QA_MSG_DISPLAY_MODE: int(mode_value)})


def _send_weather_message(
    state: ExecutionState,
    emulator: str,
    temperature: str,
    weather_code: str,
    is_day: str,
    mode: str,
) -> None:
  mode_value = int(DISPLAY_MODE_VALUES[mode])
  state.pebble.send_app_message(
      emulator, {
          QA_MSG_TEMPERATURE: int(temperature),
          QA_MSG_WEATHER_CONDITION: int(weather_code),
          QA_MSG_IS_DAY: int(is_day),
          QA_MSG_DISPLAY_MODE: mode_value,
      })


def _send_health_override(state: ExecutionState, emulator: str, bpm: str, steps: str) -> None:
  state.pebble.send_app_message(emulator, {
      QA_MSG_ONESHOT_BPM: int(bpm),
      QA_MSG_ONESHOT_STEPS: int(steps),
  })


def _set_battery_state(state: ExecutionState, emulator: str, percent: str,
                       charging_state: str) -> None:
  state.pebble.set_battery(emulator, int(percent), charging_state == "1")


def _run_weather_step(state: ExecutionState, step: PlanStep, result: StepResult) -> None:
  emulator = step.fields["emulator"]
  display = step.fields["display"]
  temp = step.fields["temp"]
  code = step.fields["code"]
  is_day = step.fields["is_day"]

  _send_weather_message(state, emulator, temp, code, is_day, display)
  if state.context.capture_screenshots:
    _capture_screenshot(state, result, emulator)


def _run_battery_step(state: ExecutionState, step: PlanStep, result: StepResult) -> None:
  emulator = step.fields["emulator"]
  display = step.fields["display"]
  level = step.fields["level"]
  charging = step.fields["charging"]

  _send_display_mode(state, emulator, display)
  time.sleep(2)
  _set_battery_state(state, emulator, level, charging)
  if state.context.capture_screenshots:
    _capture_screenshot(state, result, emulator)
  time.sleep(2)


def _run_health_step(state: ExecutionState, step: PlanStep, result: StepResult) -> None:
  emulator = step.fields["emulator"]
  display = step.fields["display"]
  bpm = step.fields["bpm"]
  steps = step.fields["steps"]

  _send_display_mode(state, emulator, display)
  time.sleep(2)
  _send_health_override(state, emulator, bpm, steps)
  if state.context.capture_screenshots:
    _capture_screenshot(state, result, emulator)
  time.sleep(2)


def _run_step(state: ExecutionState, step: PlanStep) -> None:
  result: StepResult = {
      "name": step.artifact_identity,
      "status": "running",
      "screenshot_paths": [],
  }
  state.step_results.append(result)
  try:
    match step.capability:
      case "weather":
        _run_weather_step(state, step, result)
      case "battery":
        _run_battery_step(state, step, result)
      case "health":
        _run_health_step(state, step, result)
      case _:
        raise ValueError(f"Unsupported step capability {step.capability}")
  except Exception:
    result["status"] = "failed"
    raise
  result["status"] = "passed"


def run_plan_execution(context: ExecutionContext, plan: PlanDefinition) -> int:
  state = ExecutionState(context=context, plan=plan)
  exit_status = 0

  try:
    pbw_path = context.artifact_root.parents[2] / "build" / "at-a-glance.pbw"
    state.pebble.install_emulators(plan.emulators, pbw_path)
    for step in plan.steps.values():
      _run_step(state, step)
  except Exception as exc:
    print(f"Error: {exc!r}", file=sys.stderr)
    exit_status = 1

  try:
    return finalize(
        context=state.context,
        plan=state.plan,
        exit_status=exit_status,
        step_results=state.step_results,
    )
  finally:
    state.pebble.close()
