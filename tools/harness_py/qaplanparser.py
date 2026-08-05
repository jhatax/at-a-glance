from __future__ import annotations

from pathlib import Path
from qaharnessconfig import DISPLAY_MODE_VALUES, PLANS_ROOT
from qaplangrammar import QAPlanGrammar, ParsedStep, ParsedSuite, ParsedScenario, validate_slug


def parse_step(needs_screenshot: str, tokens: list[str]) -> ParsedStep:
  if len(tokens) < 3:
    raise ValueError("STEP requires capability and field=value entries")

  capability = tokens[1]
  if not QAPlanGrammar.is_valid_capability(capability):
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

  expected_fields = set(QAPlanGrammar.CAPABILITY_FIELDS[capability])
  actual_fields = set(fields)
  missing = sorted(expected_fields - actual_fields)
  if missing:
    raise ValueError(f"STEP {capability} is missing field '{missing[0]}'")
  unknown = sorted(actual_fields - expected_fields)
  if unknown:
    raise ValueError(f"Unsupported STEP {capability} field '{unknown[0]}'")

  return ParsedStep(
      capability=capability,
      needs_screenshot=True if needs_screenshot == "yes" else False,
      fields={key: fields[key]
              for key in QAPlanGrammar.CAPABILITY_FIELDS[capability]},
  )


def _validate_display_modes(steps: list[ParsedStep], emulators: list[str]) -> None:
  for step in steps:
    display = step.fields.get("display")
    if display is None:
      continue
    for emulator in emulators:
      if not QAPlanGrammar.is_valid_display_for_emulator(display, emulator):
        raise ValueError(f"Display-mode '{display}' is unsupported on emulator '{emulator}'")


def parse_scenario(path: Path, expected_name: str | None = None) -> ParsedScenario:
  plan_name = ""
  screenshots_policy = ""
  emulators: list[str] = []
  steps: list[ParsedStep] = []
  block = ""
  seen_preamble = False
  seen_execute = False
  preamble_closed = False
  execute_closed = False

  from shlex import split
  for line_number, raw_line in enumerate(path.read_text(encoding="utf-8").splitlines(), start=1):
    line = raw_line.split("#", 1)[0].strip()
    if not line:
      continue
    tokens = split(line)
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
        if tokens[1] not in QAPlanGrammar.SCREENSHOTS_POLICIES:
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
          if emulator not in QAPlanGrammar.SUPPORTED_EMULATORS:
            raise ValueError(f"Unsupported emulator '{emulator}'")
        continue
      raise ValueError(f"Unsupported PREAMBLE directive '{directive}'")

    if block == "EXECUTE":
      if directive != "STEP":
        raise ValueError(f"Unsupported EXECUTE directive '{directive}'")
      steps.append(parse_step(screenshots_policy, tokens))
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
  return ParsedScenario(plan_name, screenshots_policy, emulators, steps)


def _parse_suite_source(
    path: Path,
    expected_name: str | None = None,
) -> tuple[str, list[tuple[str, str]]]:
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
  return suite_name, members


def parse_suite(
    path: Path,
    plans_dir: Path | None = None,
    expected_name: str | None = None,
    stack: tuple[str, ...] = (),
) -> ParsedSuite:
  suite_name, members = _parse_suite_source(path, expected_name=expected_name)
  if plans_dir is None:
    plans_dir = PLANS_ROOT
  if suite_name in stack:
    cycle = " -> ".join([*stack, suite_name])
    raise ValueError(f"Suite INCLUDE cycle detected: {cycle}")

  scenarios: list[ParsedScenario] = []
  for member_kind, member_name in members:
    member_path = plans_dir / f"{member_name}.{member_kind}"
    if not member_path.is_file():
      raise ValueError(f"Unknown included plan '{member_name}'")
    if member_kind == "scenario":
      scenarios.append(parse_scenario(member_path, expected_name=member_name))
      continue
    nested = parse_suite(
        member_path, plans_dir, expected_name=member_name, stack=(*stack, suite_name)
    )
    scenarios.extend(nested.members)
  return ParsedSuite(suite_name, scenarios)
