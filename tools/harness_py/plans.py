from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from types import MappingProxyType
from typing import Final

SUPPORTED_CAPABILITIES: Final = frozenset({"weather", "battery", "health"})
SCREENSHOTS_POLICIES: Final = frozenset({"yes", "no"})
SUPPORTED_EMULATORS: Final = frozenset({
    "aplite",
    "basalt",
    "chalk",
    "diorite",
    "emery",
    "flint",
    "gabbro",
})
COLOR_EMULATORS: Final = frozenset({"chalk", "emery", "gabbro"})
ACCEPTED_PLAN_TYPES: Final = MappingProxyType({
    ".scenario": "scenario",
    ".suite": "suite",
})
DISPLAY_MODE_VALUES: Final = MappingProxyType({
    "white": "0",
    "black": "1",
    "celeste": "2",
    "oxford": "3",
})
DISPLAY_MODES_BY_EMULATOR: Final = MappingProxyType({
    "color": frozenset(DISPLAY_MODE_VALUES),
    "monochrome": frozenset({"white", "black"}),
})
CAPABILITY_FIELD_ORDER: Final = MappingProxyType({
    "weather": ("display", "temp", "code", "is_day"),
    "battery": ("display", "level", "charging"),
    "health": ("display", "bpm", "steps"),
})


def expected_screenshot_count(capability: str, capture_screenshots: bool) -> int:
  return int(capture_screenshots)


@dataclass
class PlanStep:
  capability: str
  fields: dict[str, str]
  artifact_identity: str


@dataclass
class PlanDefinition:
  name: str
  path: Path
  screenshots_policy: str
  emulators: list[str]
  steps: dict[str, PlanStep]


@dataclass
class StepTemplate:
  capability: str
  fields: dict[str, str]


@dataclass
class ParsedPlanFile:
  name: str
  screenshots_policy: str
  emulators: list[str]
  steps: list[StepTemplate]


@dataclass
class ParsedSuiteFile:
  name: str
  members: list[tuple[str, str]]


def validate_slug(value: str, label: str) -> None:
  if not value or not value[0].isalnum():
    raise ValueError(f"Invalid {label} '{value}'")
  for char in value:
    if char.islower() or char.isdigit() or char == "-":
      continue
    raise ValueError(f"Invalid {label} '{value}'")


def _artifact_value(value: str) -> str:
  if not value:
    raise ValueError("Artifact identity cannot use empty values")
  for char in value:
    if char.islower() or char.isdigit() or char == "-":
      continue
    raise ValueError(f"Invalid artifact identity value '{value}'")
  return value


def _artifact_identity(capability: str, emulator: str, fields: dict[str, str]) -> str:
  if capability == "weather":
    return "_".join([
        "weather",
        _artifact_value(emulator),
        _artifact_value(fields["display"]),
        f"temp-{_artifact_value(fields['temp'])}",
        f"code-{_artifact_value(fields['code'])}",
        f"day-{_artifact_value(fields['is_day'])}",
    ])

  if capability == "battery":
    return "_".join([
        "battery",
        _artifact_value(emulator),
        _artifact_value(fields["display"]),
        f"level-{_artifact_value(fields['level'])}",
        f"charging-{_artifact_value(fields['charging'])}",
    ])

  if capability == "health":
    return "_".join([
        "health",
        _artifact_value(emulator),
        _artifact_value(fields["display"]),
        f"bpm-{_artifact_value(fields['bpm'])}",
        f"steps-{_artifact_value(fields['steps'])}",
    ])

  raise ValueError(f"Unsupported STEP capability '{capability}'")


def _parse_step(tokens: list[str]) -> StepTemplate:
  if len(tokens) < 3:
    raise ValueError("STEP requires capability and field=value entries")

  capability = tokens[1]
  if capability not in SUPPORTED_CAPABILITIES:
    raise ValueError(f"Unsupported STEP capability '{capability}'")

  fields: dict[str, str] = {}
  for item in tokens[2:]:
    if "=" not in item:
      raise ValueError(f"Invalid STEP field '{item}'")
    key, value = item.split("=", 1)
    if not key or not value:
      raise ValueError(f"Invalid STEP field '{item}'")
    if key in fields:
      raise ValueError(f"Duplicate STEP field '{key}'")
    if key == "display" and value not in DISPLAY_MODE_VALUES:
      raise ValueError(f"Unsupported display-mode '{value}'")
    fields[key] = value

  expected_fields = set(CAPABILITY_FIELD_ORDER[capability])
  actual_fields = set(fields)
  missing = sorted(expected_fields - actual_fields)
  if missing:
    raise ValueError(f"STEP {capability} is missing field '{missing[0]}'")
  unknown = sorted(actual_fields - expected_fields)
  if unknown:
    raise ValueError(f"Unsupported STEP {capability} field '{unknown[0]}'")

  return StepTemplate(
      capability=capability,
      fields={key: fields[key]
              for key in CAPABILITY_FIELD_ORDER[capability]},
  )


def _validate_display_modes(steps: list[StepTemplate], emulators: list[str]) -> None:
  for step in steps:
    display = step.fields.get("display")
    if display is None:
      continue
    for emulator in emulators:
      device_type = "color" if emulator in COLOR_EMULATORS else "monochrome"
      if display not in DISPLAY_MODES_BY_EMULATOR[device_type]:
        raise ValueError(f"Display-mode '{display}' is unsupported on emulator '{emulator}'")


def _expand_step_template(template: StepTemplate, emulators: list[str]) -> dict[str, PlanStep]:
  steps: dict[str, PlanStep] = {}
  for emulator in emulators:
    id: str = _artifact_identity(template.capability, emulator, template.fields)
    fields = {"emulator": emulator, **template.fields}
    steps[id] = PlanStep(template.capability, fields, id)
  return steps


# Contract: Only returns "scenario" or "suite" along with the path
def _resolve_target_path(target_name: str, root_dir: Path) -> tuple[Path, str]:
  to_check = Path(target_name)

  if not to_check.is_file():
    explicit_path = root_dir / target_name
    if explicit_path.is_file():
      to_check = explicit_path
    else:
      matches = [
          root_dir / f"{target_name}{suffix}" for suffix in ACCEPTED_PLAN_TYPES
          if (root_dir / f"{target_name}{suffix}").is_file()
      ]
      if not matches:
        raise ValueError(f"Unknown plan or suite '{target_name}'")
      if len(matches) > 1:
        raise ValueError(f"Ambiguous plan name '{target_name}'; use a .scenario or .suite path")
      to_check = matches[0]

  suffix = to_check.suffix
  if suffix not in ACCEPTED_PLAN_TYPES:
    raise ValueError(f"Unknown plan type '{target_name}'")

  return to_check, ACCEPTED_PLAN_TYPES[suffix]


def _parse_scenario_source(path: Path, expected_name: str | None = None) -> ParsedPlanFile:

  plan_name = ""
  screenshots_policy = ""
  emulators: list[str] = []
  steps: list[StepTemplate] = []
  block = ""
  seen_preamble = False
  seen_execute = False
  preamble_closed = False
  execute_closed = False

  for line_number, raw_line in enumerate(path.read_text(encoding="utf-8").splitlines(), start=1):
    line = raw_line.split("#", 1)[0].strip()
    if not line:
      continue

    tokens = line.split()
    directive = tokens[0]

    if directive in {"PREAMBLE", "EXECUTE"}:
      if len(tokens) != 1:
        raise ValueError(f"{directive} takes no values at {path}:{line_number}")
      if block:
        raise ValueError(f"{directive} cannot start inside {block} at {path}:{line_number}")
      if directive == "PREAMBLE":
        if seen_preamble:
          raise ValueError("Plan has duplicate PREAMBLE block")
        if seen_execute:
          raise ValueError("PREAMBLE must come before EXECUTE")
        seen_preamble = True
      else:
        if not preamble_closed:
          raise ValueError("EXECUTE must come after PREAMBLE END")
        if seen_execute:
          raise ValueError("Plan has duplicate EXECUTE block")
        seen_execute = True
      block = directive
      continue

    if directive == "END":
      if len(tokens) != 1:
        raise ValueError(f"END takes no values at {path}:{line_number}")
      if not block:
        raise ValueError(f"END without block at {path}:{line_number}")
      if block == "PREAMBLE":
        preamble_closed = True
      else:
        execute_closed = True
      block = ""
      continue

    if block == "PREAMBLE":
      if directive == "SCENARIO":
        if len(tokens) != 2:
          raise ValueError("SCENARIO requires exactly one value")
        if plan_name:
          raise ValueError("Plan has duplicate SCENARIO")
        plan_name = tokens[1]
        validate_slug(plan_name, "plan name")
        if expected_name is not None and plan_name != expected_name:
          raise ValueError(f"Plan '{plan_name}' does not match requested plan '{expected_name}'")
        continue

      if directive == "SCREENSHOTS":
        if len(tokens) != 2:
          raise ValueError("SCREENSHOTS requires exactly one policy")
        if screenshots_policy:
          raise ValueError("Plan has duplicate SCREENSHOTS")
        if tokens[1] not in SCREENSHOTS_POLICIES:
          raise ValueError(f"Unsupported SCREENSHOTS policy '{tokens[1]}'")
        screenshots_policy = tokens[1]
        continue

      if directive == "EMULATORS":
        if len(tokens) < 2:
          raise ValueError("EMULATORS requires at least one emulator")
        if emulators:
          raise ValueError("Plan has duplicate EMULATORS")
        emulators = tokens[1:]
        for emulator in emulators:
          validate_slug(emulator, "emulator")
          if emulator not in SUPPORTED_EMULATORS:
            raise ValueError(f"Unsupported emulator '{emulator}'")
        continue

      raise ValueError(f"Unsupported PREAMBLE directive '{directive}'")

    if block == "EXECUTE":
      if directive != "STEP":
        raise ValueError(f"Unsupported EXECUTE directive '{directive}'")
      steps.append(_parse_step(tokens))
      continue

    raise ValueError(f"Directive '{directive}' appears outside PREAMBLE or EXECUTE")

  if block:
    raise ValueError(f"Plan has unclosed {block} block")
  if not seen_preamble:
    raise ValueError("Plan is missing PREAMBLE")
  if not preamble_closed:
    raise ValueError("Plan PREAMBLE is missing END")
  if not plan_name:
    raise ValueError("Plan is missing SCENARIO")
  if not screenshots_policy:
    raise ValueError("Plan is missing SCREENSHOTS")
  if not emulators:
    raise ValueError("Plan is missing EMULATORS")
  if not seen_execute:
    raise ValueError("Plan is missing EXECUTE")
  if not execute_closed:
    raise ValueError("Plan EXECUTE is missing END")
  if not steps:
    raise ValueError("Plan has no STEP directives")
  _validate_display_modes(steps, emulators)

  return ParsedPlanFile(
      name=plan_name,
      screenshots_policy=screenshots_policy,
      emulators=emulators,
      steps=steps,
  )


def _parse_suite_source(path: Path, expected_name: str | None = None) -> ParsedSuiteFile:
  suite_name = ""
  members: list[tuple[str, str]] = []
  block = ""
  seen_preamble = False
  seen_members = False
  preamble_closed = False
  members_closed = False

  for line_number, raw_line in enumerate(path.read_text(encoding="utf-8").splitlines(), start=1):
    line = raw_line.split("#", 1)[0].strip()
    if not line:
      continue

    tokens = line.split()
    directive = tokens[0]

    if directive in {"PREAMBLE", "MEMBERS"}:
      if len(tokens) != 1:
        raise ValueError(f"{directive} takes no values at {path}:{line_number}")
      if block:
        raise ValueError(f"{directive} cannot start inside {block} at {path}:{line_number}")
      if directive == "PREAMBLE":
        if seen_preamble:
          raise ValueError("Suite has duplicate PREAMBLE block")
        if seen_members:
          raise ValueError("PREAMBLE must come before MEMBERS")
        seen_preamble = True
      else:
        if not preamble_closed:
          raise ValueError("MEMBERS must come after PREAMBLE END")
        if seen_members:
          raise ValueError("Suite has duplicate MEMBERS block")
        seen_members = True
      block = directive
      continue

    if directive == "END":
      if len(tokens) != 1:
        raise ValueError(f"END takes no values at {path}:{line_number}")
      if not block:
        raise ValueError(f"END without block at {path}:{line_number}")
      if block == "PREAMBLE":
        preamble_closed = True
      else:
        members_closed = True
      block = ""
      continue

    if block == "PREAMBLE":
      if directive != "SUITE":
        raise ValueError(f"Unsupported PREAMBLE directive '{directive}'")
      if len(tokens) != 2:
        raise ValueError("SUITE requires exactly one value")
      if suite_name:
        raise ValueError("Suite has duplicate SUITE")
      suite_name = tokens[1]
      validate_slug(suite_name, "suite name")
      if expected_name is not None and suite_name != expected_name:
        raise ValueError(f"Suite '{suite_name}' does not match requested suite '{expected_name}'")
      continue

    if block == "MEMBERS":
      if len(tokens) != 3 or directive != "INCLUDE":
        raise ValueError(f"Unsupported MEMBERS directive '{directive}'")
      member_kind = tokens[1]
      member_name = tokens[2]
      if member_kind == "SCENARIO":
        validate_slug(member_name, "plan name")
        members.append(("scenario", member_name))
        continue
      if member_kind == "SUITE":
        validate_slug(member_name, "suite name")
        members.append(("suite", member_name))
        continue
      raise ValueError(f"Unsupported INCLUDE kind '{member_kind}'")

    raise ValueError(f"Directive '{directive}' appears outside PREAMBLE or MEMBERS")

  if block:
    raise ValueError(f"Suite has unclosed {block} block")
  if not seen_preamble:
    raise ValueError("Suite is missing PREAMBLE")
  if not preamble_closed:
    raise ValueError("Suite PREAMBLE is missing END")
  if not suite_name:
    raise ValueError("Suite is missing SUITE")
  if not seen_members:
    raise ValueError("Suite is missing MEMBERS")
  if not members_closed:
    raise ValueError("Suite MEMBERS is missing END")
  if not members:
    raise ValueError("Suite has no MEMBERS entries")

  return ParsedSuiteFile(name=suite_name, members=members)


def _merge_unique(items: list[str], new_items: list[str]) -> list[str]:
  merged = list(items)
  for item in new_items:
    if item not in merged:
      merged.append(item)
  return merged


# expand steps so that they are applied to all emulators that have been parsed
def _resolve_scenario_from_parsed(path: Path, parsed: ParsedPlanFile) -> PlanDefinition:
  steps: dict[str, PlanStep] = {}
  for template in parsed.steps:
    steps.update(_expand_step_template(template, parsed.emulators))

  return PlanDefinition(
      name=parsed.name,
      path=path,
      screenshots_policy=parsed.screenshots_policy,
      emulators=parsed.emulators,
      steps=steps,
  )


def _resolve_target_definition(
    path: Path,
    target_kind: str,
    expected_name: str,
    stack: list[str],
    stack_members: set[str],
) -> PlanDefinition:

  plans_dir = path.parent

  if target_kind == "scenario":
    parsed = _parse_scenario_source(path, expected_name=expected_name)
    return _resolve_scenario_from_parsed(path, parsed)

  parsed_suite = _parse_suite_source(path, expected_name=expected_name)
  steps: dict[str, PlanStep] = {}
  emulators: list[str] = []
  screenshots_policy = "no"

  for member in parsed_suite.members:
    member_kind, member_name = member
    if member_name in stack_members:
      cycle = " -> ".join([*stack, member_name])
      raise ValueError(f"Suite INCLUDE cycle detected: {cycle}")

    suffix = f".{member_kind}"
    member_path = plans_dir / f"{member_name}{suffix}"
    if not member_path.is_file():
      raise ValueError(f"Unknown included plan '{member_name}'")
    resolved = _resolve_target_definition(
        member_path,
        member_kind,
        member_name,
        [*stack, member_name],
        {*stack_members, member_name},
    )
    steps.update(resolved.steps.items())
    emulators = _merge_unique(emulators, resolved.emulators)
    if resolved.screenshots_policy == "yes":
      screenshots_policy = "yes"

  return PlanDefinition(
      name=parsed_suite.name,
      path=path,
      screenshots_policy=screenshots_policy,
      emulators=emulators,
      steps=steps,
  )


def load_plan(target_name: str, plans_dir: Path) -> PlanDefinition:
  path, target_kind = _resolve_target_path(target_name, plans_dir)
  validate_slug(path.stem, "scenario or suite name")
  return _resolve_target_definition(path, target_kind, path.stem, [target_name], {target_name})
