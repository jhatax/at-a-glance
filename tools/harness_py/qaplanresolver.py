from __future__ import annotations

from abc import ABC, abstractmethod
from dataclasses import dataclass, field
from pathlib import Path
from textwrap import fill
from typing import Any, Callable
from qaplangrammar import (
    DiscardedItem,
    MemberDiscard,
    ParseDiscard,
    ParsedStep,
    QAPlanGrammar,
    validate_slug,
)
from qaplanparser import parse_matrix_files, parse_scenario, parse_steps, parse_suite
from qaharnessconfig import PLANS_ROOT
from qaharnessconfig import DISPLAY_MODE_VALUES

QA_MSG_TEMPERATURE = 10002
QA_MSG_WEATHER_CONDITION = 10003
QA_MSG_IS_DAY = 10004
QA_MSG_DISPLAY_MODE = 10006
QA_MSG_MAYBE_CURRENT_LOCATION = 10011
QA_MSG_ONESHOT_BPM = 10020
QA_MSG_ONESHOT_STEPS = 10021


@dataclass(frozen=True)
class ResolutionDiscard(DiscardedItem):
  capability: str
  execution_config: tuple[str, str]
  reason: str

  def describe(self) -> str:
    emulator, display = self.execution_config
    target = f"Emulator '{emulator}', display '{display}'"
    if self.capability:
      target += f", capability='{self.capability}'"
    return f"{target}: {self.reason}"


@dataclass
class PlanDefinition:
  name: str
  path: Path
  steps: dict[str, PlanStep]
  execution_configs: list[tuple[str, str]]
  discarded: list[ParseDiscard | ResolutionDiscard | MemberDiscard] = field(default_factory=list)
  expected_screenshots: int = field(init=False, default=0)
  captured_screenshots: int = field(init=False, default=0)

  @property
  def step_count(self) -> int:
    return len(self.steps)

  def as_dict(self) -> dict[str, Any]:
    from dataclasses import asdict
    d: dict[str, Any] = asdict(self)
    d["path"] = str(self.path)
    return d


@dataclass
class PlanStep(ABC):
  capability: str = field(init=False)
  emulator: str = field(kw_only=True)
  display: str = field(kw_only=True)
  capture_screenshots: bool = field(kw_only=True)
  expected_screenshots: int = field(kw_only=True)
  captured_screenshots: int = field(kw_only=True, default=0)
  _step_id: str = field(init=False, default="")

  @abstractmethod
  def run(
      self,
      pebble: Any,
      connection: Any,
      capture_screenshot: Callable[[str], None],
  ) -> None:
    pass

  @property
  @abstractmethod
  def step_id(self) -> str:
    pass

  def as_dict(self) -> dict[str, Any]:
    from dataclasses import asdict
    d: dict[str, Any] = asdict(self)
    for key in (
        "_step_id",
        "capability",
        "capture_screenshots",
        "expected_screenshots",
        "captured_screenshots",
    ):
      d.pop(key, None)
    return d


@dataclass
class WeatherStep(PlanStep):
  temp: int
  code: int
  is_day: int
  capability: str = field(default="weather", init=False)

  def __post_init__(self) -> None:
    if (not all([self.capability, self.emulator])) or (not 0 <= self.is_day <= 1):
      raise ValueError(f"All fields must have a valid, non-empty value. Provided:\n{self}")

    self._step_id = "_".join(
        filter(
            None, [
                self.capability,
                self.emulator,
                self.display,
                str(self.temp),
                str(self.code),
                str(self.is_day),
                "shots" if self.capture_screenshots else "",
            ]
        )
    ).lower()

  def run(
      self,
      pebble: Any,
      connection: Any,
      capture_screenshot: Callable[[str], None],
  ) -> None:
    pebble.send_app_message(
        connection,
        self.emulator,
        {
            QA_MSG_TEMPERATURE: self.temp,
            QA_MSG_WEATHER_CONDITION: self.code,
            QA_MSG_IS_DAY: self.is_day,
            QA_MSG_DISPLAY_MODE: int(DISPLAY_MODE_VALUES[self.display]),
        },
    )
    if self.capture_screenshots:
      capture_screenshot(self.emulator)

  @property
  def step_id(self) -> str:
    return self._step_id


@dataclass
class LocationStep(PlanStep):
  location: str
  capability: str = field(default="location", init=False)

  def __post_init__(self) -> None:
    if (not all([self.capability, self.emulator, self.location])):
      raise ValueError(f"All fields must have a valid, non-empty value. Provided:\n{self}")

    self._step_id = "_".join(
        filter(
            None, [
                self.capability,
                self.emulator,
                self.display,
                self.location,
                "shots" if self.capture_screenshots else "",
            ]
        )
    ).lower()

  def run(
      self,
      pebble: Any,
      connection: Any,
      capture_screenshot: Callable[[str], None],
  ) -> None:
    pebble.send_app_message(
        connection,
        self.emulator,
        {
            QA_MSG_MAYBE_CURRENT_LOCATION: self.location,
            QA_MSG_DISPLAY_MODE: int(DISPLAY_MODE_VALUES[self.display]),
        },
    )
    if self.capture_screenshots:
      capture_screenshot(self.emulator)

  @property
  def step_id(self) -> str:
    return self._step_id


@dataclass
class BluetoothStep(PlanStep):
  connected: int
  capability: str = field(default="bluetooth", init=False)

  def __post_init__(self) -> None:
    if not (all([self.capability, self.emulator]) and (0 <= self.connected <= 1)):
      raise ValueError(f"All fields must have a valid, non-empty value. Provided:\n{self}")

    self._step_id = "_".join(
        filter(
            None, [
                self.capability,
                self.emulator,
                self.display,
                str(self.connected),
                "shots" if self.capture_screenshots else "",
            ]
        )
    ).lower()

  def run(
      self,
      pebble: Any,
      connection: Any,
      capture_screenshot: Callable[[str], None],
  ) -> None:
    if self.display:
      pebble.send_app_message(
          connection,
          self.emulator,
          {QA_MSG_DISPLAY_MODE: int(DISPLAY_MODE_VALUES[self.display])},
      )
    pebble.set_bluetooth(
        connection,
        self.emulator,
        self.connected,
        capture_screenshot if self.capture_screenshots else None,
    )

  @property
  def step_id(self) -> str:
    return self._step_id


@dataclass
class BatteryStep(PlanStep):
  level: int
  charging: int
  capability: str = field(default="battery", init=False)

  def __post_init__(self) -> None:
    if not ((0 <= self.level <= 100) and (0 <= self.charging <= 1)):
      raise ValueError(
          f"Battery inputs are invalid, level: '{self.level}', charging: '{self.charging}'",
      )

    self._step_id = "_".join(
        filter(
            None, [
                self.capability,
                self.emulator,
                self.display,
                str(self.level),
                str(self.charging),
                "shots" if self.capture_screenshots else "",
            ]
        )
    ).lower()

  def run(
      self,
      pebble: Any,
      connection: Any,
      capture_screenshot: Callable[[str], None],
  ) -> None:
    if self.display:
      pebble.send_app_message(
          connection,
          self.emulator,
          {QA_MSG_DISPLAY_MODE: int(DISPLAY_MODE_VALUES[self.display])},
      )
    pebble.set_battery(connection, self.emulator, self.level, self.charging)
    if self.capture_screenshots:
      capture_screenshot(self.emulator)

  @property
  def step_id(self) -> str:
    return self._step_id


@dataclass
class HealthStep(PlanStep):
  bpm: int
  steps: int
  capability: str = field(default="health", init=False)

  def __post_init__(self) -> None:
    if not all([self.capability, self.emulator]):
      raise ValueError(f"All fields must have a valid, non-empty value. Provided:\n{self}")
    self._step_id = "_".join(
        filter(
            None, [
                self.capability,
                self.emulator,
                self.display,
                str(self.steps),
                str(self.bpm),
                "shots" if self.capture_screenshots else "",
            ]
        )
    ).lower()

  def run(
      self,
      pebble: Any,
      connection: Any,
      capture_screenshot: Callable[[str], None],
  ) -> None:
    pebble.send_app_message(
        connection,
        self.emulator,
        {
            QA_MSG_ONESHOT_BPM: self.bpm,
            QA_MSG_ONESHOT_STEPS: self.steps,
            QA_MSG_DISPLAY_MODE: int(DISPLAY_MODE_VALUES[self.display]),
        },
    )
    if self.capture_screenshots:
      capture_screenshot(self.emulator)

  @property
  def step_id(self) -> str:
    return self._step_id


@dataclass
class AllForOneStep(PlanStep):
  bpm: int
  steps: int
  level: int
  charging: int
  temp: int
  code: int
  is_day: int
  location: str
  connected: int
  capability: str = field(default="all", init=False)

  def __post_init__(self) -> None:
    if not ((0 <= self.level <= 100) and (0 <= self.charging <= 1)) or \
    (not all([self.capability, self.emulator])) or \
    (not 0 <= self.is_day <= 1) or (not 0 <= self.connected <= 1):
      raise ValueError(f"Invalid input for step: '{self}'")
    self._step_id = "_".join(
        filter(
            None, [
                self.capability,
                self.emulator,
                self.display,
                str(self.temp),
                str(self.code),
                str(self.is_day),
                str(self.level),
                str(self.charging),
                str(self.steps),
                str(self.bpm),
                self.location,
                str(self.connected),
                "shots" if self.capture_screenshots else "",
            ]
        )
    ).lower()

  def run(
      self,
      pebble: Any,
      connection: Any,
      capture_screenshot: Callable[[str], None],
  ) -> None:
    pebble.send_app_message(
        connection,
        self.emulator,
        {
            QA_MSG_ONESHOT_BPM: self.bpm,
            QA_MSG_ONESHOT_STEPS: self.steps,
            QA_MSG_DISPLAY_MODE: int(DISPLAY_MODE_VALUES[self.display]),
            QA_MSG_TEMPERATURE: self.temp,
            QA_MSG_WEATHER_CONDITION: self.code,
            QA_MSG_IS_DAY: self.is_day,
            QA_MSG_MAYBE_CURRENT_LOCATION: self.location,
        },
    )
    pebble.set_battery(connection, self.emulator, self.level, self.charging)
    pebble.set_bluetooth(connection, self.emulator, self.connected, self.capture_screenshots)
    if self.capture_screenshots:
      capture_screenshot(self.emulator)

  @property
  def step_id(self) -> str:
    return self._step_id


def print_plan(plan: PlanDefinition):
  divider = "=" * 80
  print(f"{divider}")
  print(f"Plan: {plan.name}")
  print(f"Steps to execute: {plan.step_count}")
  print(f"Expected screenshots: {plan.expected_screenshots}")
  print("Resolved execution plan:")
  for index, step in enumerate(plan.steps.values(), start=1):
    step_details = fill(f"{index}. Step: {step}", width=80, subsequent_indent="  ")
    print(step_details)

  if plan.discarded:
    print("Discarded plan items:")
    for index, discarded in enumerate(plan.discarded, start=1):
      details = f"{index}. {discarded.describe()}"
      print(fill(details, width=80, subsequent_indent="  "))

  print(f"{divider}")


def create_step_for_capability(step_type: str, capture_screen: bool, **kwargs: Any) -> PlanStep:
  _capability = step_type.lower()
  expected = QAPlanGrammar.EXPECTED_SCREENSHOTS[_capability] if capture_screen else 0

  def required(name: str) -> Any:
    if name not in kwargs:
      raise ValueError(f"Missing required field '{name}' for capability '{_capability}'")
    return kwargs[name]

  shared: dict[str, Any] = {
      "emulator": str(required("emulator")),
      "display": str(required("display")),
      "capture_screenshots": capture_screen,
      "expected_screenshots": expected,
  }
  match _capability:
    case "weather":
      return WeatherStep(
          **shared,
          temp=int(required("temp")),
          code=int(required("code")),
          is_day=int(required("is_day")),
      )

    case "location":
      return LocationStep(
          **shared,
          location=str(required("location")),
      )

    case "bluetooth":
      return BluetoothStep(
          **shared,
          connected=int(required("connected")),
      )

    case "battery":
      return BatteryStep(
          **shared,
          level=int(required("level")),
          charging=int(required("charging")),
      )

    case "health":
      return HealthStep(
          **shared,
          bpm=int(required("bpm")),
          steps=int(required("steps")),
      )

    case "all":
      return AllForOneStep(
          **shared,
          temp=int(required("temp")),
          code=int(required("code")),
          is_day=int(required("is_day")),
          level=int(required("level")),
          charging=int(required("charging")),
          bpm=int(required("bpm")),
          steps=int(required("steps")),
          location=str(required("location")),
          connected=int(required("connected")),
      )

    case _:
      raise ValueError(f"Unsupported step type: '{step_type}'")


def _expand_step_template(
    template: ParsedStep,
    execution_configs: list[tuple[str, str]],
    screenshots_policy: str,
) -> tuple[dict[str, PlanStep], list[ResolutionDiscard]]:
  steps: dict[str, PlanStep] = {}
  discarded: list[ResolutionDiscard] = []
  for emulator, display in execution_configs:
    if not QAPlanGrammar.is_capability_supported_by_emulator(template.capability, emulator):
      discarded.append(
          ResolutionDiscard(
              capability=template.capability,
              execution_config=(emulator, display),
              reason=(
                  f"Emulator '{emulator}' doesn't support capability "
                  f"'{template.capability}'"
              ),
          )
      )
      continue

    fields: dict[str, Any] = dict(template.fields)
    fields["emulator"] = emulator
    fields["display"] = display
    try:
      step: PlanStep = create_step_for_capability(
          template.capability, screenshots_policy == "yes", **fields
      )
    except (TypeError, ValueError) as exc:
      discarded.append(
          ResolutionDiscard(
              capability=template.capability,
              execution_config=(emulator, display),
              reason=f"Could not create step: {exc}",
          )
      )
      continue
    steps[step.step_id] = step
  return steps, discarded


def _create_execution_configs(
    emulators: list[str], displays: list[str]
) -> tuple[list[tuple[str, str]], list[ResolutionDiscard]]:
  configs: list[tuple[str, str]] = []
  discarded: list[ResolutionDiscard] = []
  for emulator in emulators:
    for display in displays:
      if not QAPlanGrammar.is_valid_display_for_emulator(display, emulator):
        discarded.append(
            ResolutionDiscard(
                capability="",
                execution_config=(emulator, display),
                reason=f"Display '{display}' is unsupported on emulator '{emulator}'",
            )
        )
        continue
      configs.append((emulator, display))
  return configs, discarded


def _resolve_plan_inputs(
    name: str,
    path: Path,
    screenshots_policy: str,
    emulators: list[str],
    displays: list[str],
    templates: list[ParsedStep],
    parse_discards: list[ParseDiscard],
) -> PlanDefinition:
  steps: dict[str, PlanStep] = {}
  execution_configs, resolution_discards = _create_execution_configs(emulators, displays)
  discarded: list[ParseDiscard | ResolutionDiscard | MemberDiscard] = list(parse_discards)
  discarded.extend(resolution_discards)
  if not execution_configs:
    raise ValueError(f"No supported emulator/display configurations remain for plan '{name}'")
  for template in templates:
    expanded, step_discards = _expand_step_template(template, execution_configs, screenshots_policy)
    steps.update(expanded)
    discarded.extend(step_discards)

  if not steps:
    raise ValueError(
        fill(
            f"No executable steps remain for plan '{name}' after filtering",
            width=80,
            subsequent_indent=" "
        )
    )

  return PlanDefinition(
      name=name,
      path=path,
      steps=steps,
      execution_configs=execution_configs,
      discarded=discarded,
  )


def _expand_suite_members(path: Path, ) -> tuple[dict[tuple[str, str], Path], list[MemberDiscard]]:
  discovered: dict[tuple[str, str], Path] = {
      ("suite", path.stem): path,
  }
  member_types: list[tuple[str, str]] = [("suite", path.stem)]
  discarded: list[MemberDiscard] = []
  index = 0
  while index < len(member_types):
    member_kind, member_name = member_types[index]
    index += 1
    if member_kind != "suite":
      continue
    member = (member_kind, member_name)
    repeated_members, new_members = parse_suite(discovered[member], discovered)
    discarded.extend(repeated_members)
    for new_member in new_members:
      member = (new_member.kind, new_member.name)
      member_path = discovered[member]
      if member_path.is_file():
        member_types.append(member)
        continue
      del discovered[member]
      discarded.append(
          MemberDiscard(
              kind=new_member.kind,
              name=new_member.name,
              reason=f"Unknown included plan '{new_member.name}'",
          )
      )
  return discovered, discarded


def _resolve_target_definition(
    path: Path,
    target_kind: str,
    plans_dir: Path,
) -> PlanDefinition:
  if target_kind == "scenario":
    screenshots_policy, emulators, displays, templates, parse_discards = parse_scenario(path)
    return _resolve_plan_inputs(
        path.stem,
        path,
        screenshots_policy,
        emulators,
        displays,
        templates,
        parse_discards,
    )

  if target_kind == "suite":
    discovered, member_discards = _expand_suite_members(path)

    steps: dict[str, PlanStep] = {}
    execution_configs: list[tuple[str, str]] = []
    discarded: list[ParseDiscard | ResolutionDiscard | MemberDiscard] = list(member_discards)

    for (member_kind, _), member_path in discovered.items():
      if member_kind == "suite":
        continue
      resolved = _resolve_target_definition(
          member_path,
          member_kind,
          plans_dir,
      )
      steps.update(resolved.steps)
      for config in resolved.execution_configs:
        if config not in execution_configs:
          execution_configs.append(config)
      discarded.extend(resolved.discarded)

    if not steps:
      raise ValueError(f"No executable steps remain for suite '{path.stem}' after filtering")

    return PlanDefinition(
        name=path.stem,
        path=path,
        steps=steps,
        execution_configs=execution_configs,
        discarded=discarded,
    )
  if target_kind == "matrix":
    screenshots_policy, emulators, displays, step_files, parse_discards = parse_matrix_files(path)
    templates: list[ParsedStep] = []
    for step_file in step_files:
      step_path = path.parent / step_file
      if not step_path.is_file():
        raise ValueError(f"Unknown steps file '{step_file}'")
      step_templates, step_discards = parse_steps(step_path)
      templates.extend(step_templates)
      parse_discards.extend(step_discards)
    return _resolve_plan_inputs(
        path.stem,
        path,
        screenshots_policy,
        emulators,
        displays,
        templates,
        parse_discards,
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
  loaded: PlanDefinition = _resolve_target_definition(path, target_kind, target_parent)
  loaded.expected_screenshots = sum(step.expected_screenshots for step in loaded.steps.values())
  print_plan(loaded)
  return loaded
