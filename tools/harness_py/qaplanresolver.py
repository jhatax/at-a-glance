from __future__ import annotations

from abc import ABC, abstractmethod
from dataclasses import asdict, dataclass, field
from pathlib import Path
from typing import Final, Any
from qaplangrammar import ParsedStep, ParsedScenario, QAPlanGrammar, validate_slug
from qaplanparser import parse_scenario, parse_suite
from qaharnessconfig import PLANS_ROOT

INVALID_TEMPERATURE: Final[int] = 32767
INVALID_CLIMATE_VALUE: Final[int] = -1
INVALID_BATTERY_CHARGE: Final[int] = -1
INVALID_HEALTH_METRIC: Final[int] = -2


@dataclass
class PlanDefinition:
  name: str
  path: Path
  emulators: list[str]
  steps: dict[str, PlanStep]
  expected_screenshots: int = field(init=False, default=0)
  captured_screenshots: int = field(init=False, default=0)
  _step_count: int = field(init=False, default=0)

  def __post_init__(self) -> None:
    self._step_count = len(self.steps)

  @property
  def step_count(self) -> int:
    return self._step_count


@dataclass
class PlanStep(ABC):
  capability: str = ""
  emulator: str = ""
  display: str = ""
  capture_screenshots: bool = False
  expected_screenshots: int = 0
  captured_screenshots: int = 0
  _step_id: str = field(init=False, default="")

  @property
  @abstractmethod
  def step_id(self) -> str:
    pass


@dataclass
class WeatherStep(PlanStep):
  temp: int = INVALID_TEMPERATURE
  code: int = INVALID_CLIMATE_VALUE
  is_day: int = INVALID_CLIMATE_VALUE
  capability: str = field(default="weather")

  def __post_init__(self) -> None:
    if (not all([self.capability, self.emulator])) or (self.temp == INVALID_TEMPERATURE) \
        or (self.code == INVALID_CLIMATE_VALUE) or (self.is_day == INVALID_CLIMATE_VALUE):
      raise ValueError("All fields must have a valid, non-empty value")

    self._step_id = "_".join(
        filter(
            None, [
                self.capability, self.emulator, self.display,
                str(self.temp),
                str(self.code),
                str(self.is_day), "shots" if self.capture_screenshots else ""
            ]
        )
    ).lower()

  @property
  def step_id(self) -> str:
    return self._step_id


@dataclass
class BatteryStep(PlanStep):
  level: int = INVALID_BATTERY_CHARGE
  charging: int = 0
  capability: str = field(default="battery")

  def __post_init__(self) -> None:
    if self.level == INVALID_BATTERY_CHARGE:
      raise ValueError("Battery-level must be greater than 0")

    self._step_id = "_".join(
        filter(
            None, [
                self.capability, self.emulator,
                str(self.level),
                str(self.charging), "shots" if self.capture_screenshots else ""
            ]
        )
    ).lower()

  @property
  def step_id(self) -> str:
    return self._step_id


@dataclass
class HealthStep(PlanStep):
  bpm: int = INVALID_HEALTH_METRIC
  steps: int = INVALID_HEALTH_METRIC
  capability: str = field(default="health")

  def __post_init__(self) -> None:
    if (self.bpm == INVALID_HEALTH_METRIC) or (self.steps == INVALID_HEALTH_METRIC):
      raise ValueError(
          f"Heart-rate and steps-walked must be greater than '{INVALID_HEALTH_METRIC}'",
      )
    self._step_id = "_".join(
        filter(
            None, [
                self.capability, self.emulator,
                str(self.steps),
                str(self.bpm), "shots" if self.capture_screenshots else ""
            ]
        )
    ).lower()

  @property
  def step_id(self) -> str:
    return self._step_id


def print_plan(plan: PlanDefinition):
  from textwrap import fill

  divider = "=" * 80
  print(f"{divider}")
  print(f"Plan: {plan.name}")
  print(f"Steps to execute: {plan.step_count}")
  print(f"Expected screenshots: {plan.expected_screenshots}")
  print("Resolved execution plan:")
  for index, (_, step) in enumerate(plan.steps.items(), start=1):
    step_details = fill(f"{index}. Step: {asdict(step)}", width=80, subsequent_indent="  ")
    print(step_details)

  print(f"{divider}")


def _create_step_for_capability(step_type: str, capture_screen: bool, **kwargs: Any) -> PlanStep:
  _capability = step_type.lower()
  expected = 1 if capture_screen else 0
  match _capability:

    case "weather":

      return WeatherStep(
          emulator=str(kwargs.get("emulator", "")),
          capability=_capability,
          temp=int(kwargs.get("temp", INVALID_TEMPERATURE)),
          code=int(kwargs.get("code", INVALID_CLIMATE_VALUE)),
          is_day=int(kwargs.get("is_day", INVALID_CLIMATE_VALUE)),
          display=str(kwargs.get("display", "")),
          capture_screenshots=capture_screen,
          expected_screenshots=expected,
      )

    case "battery":
      return BatteryStep(
          emulator=str(kwargs.get("emulator", "")),
          capability=_capability,
          level=int(kwargs.get("level", INVALID_BATTERY_CHARGE)),
          charging=int(kwargs.get("charging", "0")),
          display=str(kwargs.get("display", "")),
          capture_screenshots=capture_screen,
          expected_screenshots=expected,
      )

    case "health":
      return HealthStep(
          emulator=str(kwargs.get("emulator", "")),
          capability=_capability,
          bpm=int(kwargs.get("bpm", INVALID_HEALTH_METRIC)),
          steps=int(kwargs.get("steps", INVALID_HEALTH_METRIC)),
          display=str(kwargs.get("display", "")),
          capture_screenshots=capture_screen,
          expected_screenshots=expected,
      )

    case _:
      raise ValueError(f"Unsupported step type: '{step_type}'")


def _expand_step_template(
    template: ParsedStep,
    emulators: list[str],
) -> dict[str, PlanStep]:
  steps: dict[str, PlanStep] = {}
  for emulator in emulators:
    fields: dict[str, Any] = dict(template.fields)
    fields["emulator"] = emulator
    step: PlanStep = _create_step_for_capability(
        template.capability, template.needs_screenshot, **fields
    )
    steps[step.step_id] = step
  return steps


def _merge_unique(items: list[str], new_items: list[str]) -> list[str]:
  merged = list(items)
  for item in new_items:
    if item not in merged:
      merged.append(item)
  return merged


def _resolve_scenario_from_parsed(path: Path, parsed: ParsedScenario) -> PlanDefinition:
  steps: dict[str, PlanStep] = {}
  for template in parsed.steps:
    steps.update(_expand_step_template(template, parsed.emulators))

  return PlanDefinition(
      name=parsed.name,
      path=path,
      emulators=parsed.emulators,
      steps=steps,
  )


def _resolve_target_definition(
    path: Path, target_kind: str, expected_name: str, plans_dir: Path
) -> PlanDefinition:
  if target_kind == "scenario":
    return _resolve_scenario_from_parsed(path, parse_scenario(path, expected_name))

  if target_kind == "suite":
    # parse a suite into included scenarios
    _suite = parse_suite(path, plans_dir=plans_dir, expected_name=expected_name)
    # dictionary of identity and planned-step
    steps: dict[str, PlanStep] = {}
    emulators: list[str] = []

    for scenario in _suite.members:
      resolved = _resolve_scenario_from_parsed(path, scenario)
      steps.update(resolved.steps)
      emulators = _merge_unique(emulators, resolved.emulators)

    return PlanDefinition(
        name=_suite.name,
        path=path,
        emulators=emulators,
        steps=steps,
    )
  raise ValueError(f"Unsupported QA Plan type: '{target_kind}'")


def _resolve_target_path(target_name: str, root_dir: Path) -> tuple[Path, str]:
  to_check = Path(target_name)

  if not to_check.is_file():
    explicit_path = root_dir / target_name
    if explicit_path.is_file():
      to_check = explicit_path
    else:
      matches = [
          root_dir / f"{target_name}{suffix}" for suffix in QAPlanGrammar.ACCEPTED_PLAN_TYPES
          if (root_dir / f"{target_name}{suffix}").is_file()
      ]
      if not matches:
        raise ValueError(f"Unknown plan or suite '{target_name}'")
      if len(matches) > 1:
        raise ValueError(f"Ambiguous plan name '{target_name}'; use a .scenario or .suite path")
      to_check = matches[0]

  suffix = to_check.suffix
  if suffix not in QAPlanGrammar.ACCEPTED_PLAN_TYPES:
    raise ValueError(f"Unknown plan type '{target_name}'")
  return to_check, QAPlanGrammar.ACCEPTED_PLAN_TYPES[suffix]


def load_and_validate_plan(target: str, target_parent: Path = PLANS_ROOT) -> PlanDefinition:

  path = Path(target)
  # handle file-path
  if path.is_file():
    target_parent = path.parent

  path, target_kind = _resolve_target_path(target, target_parent)
  validate_slug(path.stem, "scenario or suite name")
  loaded: PlanDefinition = _resolve_target_definition(path, target_kind, path.stem, target_parent)
  loaded.expected_screenshots = sum(step.expected_screenshots for step in loaded.steps.values())
  print_plan(loaded)
  return loaded
