from __future__ import annotations

from pathlib import Path
from collections.abc import Iterator
from shlex import split

from qaplangrammar import (
    AcceptedMember,
    MemberDiscard,
    QAPlanGrammar,
    ParseDiscard,
    ParsedStep,
    validate_slug,
)


def _line_feed(path: Path) -> Iterator[tuple[int, str]]:
  for line_number, raw_line in enumerate(path.read_text(encoding="utf-8").splitlines(), start=1):
    line = raw_line.strip()
    if line and "#" not in line:
      yield line_number, line


def _discard(line_number: int, source: str, reason: str) -> ParseDiscard:
  return ParseDiscard(line_number=line_number, source=source, reason=reason)


def parse_step(
    tokens: list[str],
    line_number: int,
    source: str,
) -> ParsedStep | ParseDiscard:
  if len(tokens) < 3:
    return _discard(line_number, source, "STEP requires capability and field=value entries")

  capability = tokens[1]
  if not QAPlanGrammar.is_valid_capability(capability):
    return _discard(line_number, source, f"Unsupported STEP capability '{capability}'")

  fields: dict[str, str] = {}
  for item in tokens[2:]:
    if "=" not in item:
      return _discard(line_number, source, f"Invalid STEP field '{item}'")
    key, value = item.split("=", 1)
    if not key or not value:
      return _discard(line_number, source, f"Invalid STEP field '{item}'")
    if key in fields:
      return _discard(line_number, source, f"Duplicate STEP field '{key}'")
    fields[key] = value

  expected_fields = set(QAPlanGrammar.CAPABILITY_FIELDS[capability])
  actual_fields = set(fields)
  missing = sorted(expected_fields - actual_fields)
  if missing:
    return _discard(line_number, source, f"STEP {capability} is missing field '{missing[0]}'")
  unknown = sorted(actual_fields - expected_fields)
  if unknown:
    return _discard(line_number, source, f"Unsupported STEP {capability} field '{unknown[0]}'")

  return ParsedStep(
      capability=capability,
      fields={key: fields[key]
              for key in QAPlanGrammar.CAPABILITY_FIELDS[capability]},
  )


def _parse_preamble(
    lines: Iterator[tuple[int, str]],
    path: Path,
) -> tuple[str, list[str], list[str], list[ParseDiscard], bool]:
  screenshots_policy = ""
  emulators: list[str] = []
  displays: list[str] = []
  seen_displays = False
  discarded: list[ParseDiscard] = []
  for line_number, line in lines:
    if line.split(maxsplit=1)[0] not in QAPlanGrammar.GRAMMAR_VOCABULARY:
      continue
    try:
      tokens = split(line)
    except ValueError as exc:
      raise ValueError(f"Malformed plan line at {path}:{line_number}: {exc}") from exc
    directive = tokens[0]
    if directive == "PREAMBLE":
      if len(tokens) != 1:
        raise ValueError(f"PREAMBLE takes no values at {path}:{line_number}")
      break
    continue

  else:
    raise ValueError("Plan is missing PREAMBLE")

  for line_number, line in lines:
    if line.split(maxsplit=1)[0] not in QAPlanGrammar.GRAMMAR_VOCABULARY:
      continue
    try:
      tokens = split(line)
    except ValueError as exc:
      raise ValueError(f"Malformed plan line at {path}:{line_number}: {exc}") from exc
    directive = tokens[0]
    if directive == "END":
      if len(tokens) != 1:
        raise ValueError(f"END takes no values at {path}:{line_number}")
      return screenshots_policy, emulators, displays, discarded, seen_displays
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
        if not QAPlanGrammar.is_valid_emulator(emulator):
          raise ValueError(f"Unsupported emulator '{emulator}'")
      continue
    if directive == "DISPLAYS":
      if len(tokens) < 2:
        raise ValueError("DISPLAYS requires at least one display")
      if seen_displays:
        raise ValueError("Plan has duplicate DISPLAYS")
      seen_displays = True
      for display in tokens[1:]:
        if not QAPlanGrammar.is_valid_display(display):
          discarded.append(_discard(line_number, line, f"Unknown display '{display}'"))
          continue
        if display in displays:
          discarded.append(_discard(line_number, line, f"Duplicate display '{display}'"))
          continue
        displays.append(display)
      continue
  raise ValueError("Plan PREAMBLE is missing END")


def _parse_execute(
    lines: Iterator[tuple[int, str]],
    path: Path,
) -> tuple[list[ParsedStep], list[ParseDiscard]]:
  steps: list[ParsedStep] = []
  discarded: list[ParseDiscard] = []
  for line_number, line in lines:
    if line.split(maxsplit=1)[0] not in QAPlanGrammar.GRAMMAR_VOCABULARY:
      continue
    try:
      tokens = split(line)
    except ValueError as exc:
      raise ValueError(f"Malformed plan line at {path}:{line_number}: {exc}") from exc
    directive = tokens[0]
    if directive == "EXECUTE":
      if len(tokens) != 1:
        raise ValueError(f"EXECUTE takes no values at {path}:{line_number}")
      break
    continue
  else:
    raise ValueError("Plan is missing EXECUTE")

  for line_number, line in lines:
    if line.split(maxsplit=1)[0] not in QAPlanGrammar.GRAMMAR_VOCABULARY:
      continue
    try:
      tokens = split(line)
    except ValueError as exc:
      discarded.append(_discard(line_number, line, f"Malformed STEP: {exc}"))
      continue
    directive = tokens[0]
    if directive == "END":
      if len(tokens) != 1:
        raise ValueError(f"END takes no values at {path}:{line_number}")
      if not steps:
        raise ValueError("Plan has no valid STEP directives")
      return steps, discarded
    if directive != "STEP":
      continue
    parsed_step = parse_step(tokens, line_number, line)
    if isinstance(parsed_step, ParseDiscard):
      discarded.append(parsed_step)
    else:
      steps.append(parsed_step)
  raise ValueError("Plan EXECUTE is missing END")


def parse_scenario(
    path: Path,
) -> tuple[str, list[str], list[str], list[ParsedStep], list[ParseDiscard]]:
  lines = _line_feed(path)
  screenshots_policy, emulators, displays, discarded, seen_displays = _parse_preamble(lines, path)
  steps, execute_discards = _parse_execute(lines, path)
  discarded.extend(execute_discards)

  if not screenshots_policy:
    raise ValueError("Plan is missing SCREENSHOTS")
  if not emulators:
    raise ValueError("Plan is missing EMULATORS")
  if not seen_displays:
    raise ValueError("Plan is missing DISPLAYS")
  return screenshots_policy, emulators, displays, steps, discarded


def parse_steps(path: Path) -> tuple[list[ParsedStep], list[ParseDiscard]]:
  steps, discarded = _parse_execute(_line_feed(path), path)
  return steps, discarded


def parse_matrix_files(
    path: Path,
) -> tuple[str, list[str], list[str], list[str], list[ParseDiscard]]:
  lines = _line_feed(path)
  screenshots_policy, emulators, displays, preamble_discards, seen_displays = _parse_preamble(
      lines, path
  )
  if not screenshots_policy:
    raise ValueError("Matrix is missing SCREENSHOTS")
  if not emulators:
    raise ValueError("Matrix is missing EMULATORS")
  if not seen_displays:
    raise ValueError("Matrix is missing DISPLAYS")

  for line_number, line in lines:
    if line.split(maxsplit=1)[0] not in QAPlanGrammar.GRAMMAR_VOCABULARY:
      continue
    try:
      tokens = split(line)
    except ValueError as exc:
      raise ValueError(f"Malformed STEP line at {path}:{line_number}: {exc}") from exc
    if tokens[0] != "STEPS":
      continue
    if len(tokens) != 1:
      raise ValueError(f"STEPS takes no values at {path}:{line_number}")
    break
  else:
    raise ValueError("Matrix is missing STEPS")

  step_files: list[str] = []
  for line_number, line in lines:
    try:
      tokens = split(line)
    except ValueError as exc:
      raise ValueError(f"Malformed STEP line at {path}:{line_number}: {exc}") from exc
    if tokens[0] == "END":
      if len(tokens) != 1:
        raise ValueError(f"END takes no values at {path}:{line_number}")
      if not step_files:
        raise ValueError("Matrix STEPS has no file entries")
      return screenshots_policy, emulators, displays, step_files, preamble_discards
    if len(tokens) != 1:
      raise ValueError(f"STEPS file entry takes one value at {path}:{line_number}")
    stem = tokens[0]
    if not Path(stem).suffix == QAPlanGrammar.ACCEPTED_STEPS_SUFFIX:
      print(f"Invalid STEPS file entry '{stem}'")
      continue
    step_files.append(tokens[0])
  raise ValueError("Matrix STEPS is missing END")


def _parse_members(
    lines: Iterator[tuple[int, str]],
    path: Path,
) -> list[tuple[str, str]]:
  members: list[tuple[str, str]] = []
  for line_number, line in lines:
    if line.split(maxsplit=1)[0] not in QAPlanGrammar.GRAMMAR_VOCABULARY:
      continue
    try:
      tokens = split(line)
    except ValueError as exc:
      raise ValueError(f"Malformed MEMBERS line at {path}:{line_number}: {exc}") from exc
    if tokens[0] != "MEMBERS":
      continue
    if len(tokens) != 1:
      raise ValueError(f"MEMBERS takes no values at {path}:{line_number}")
    break
  else:
    raise ValueError("Suite is missing MEMBERS")

  for line_number, line in lines:
    if line.split(maxsplit=1)[0] not in QAPlanGrammar.GRAMMAR_VOCABULARY:
      continue
    try:
      tokens = split(line)
    except ValueError as exc:
      raise ValueError(f"Malformed MEMBER line at {path}:{line_number}: {exc}") from exc
    directive = tokens[0]
    if directive == "END":
      if len(tokens) != 1:
        raise ValueError(f"END takes no values at {path}:{line_number}")
      if not members:
        raise ValueError("Suite has no MEMBERS entries")
      return members
    if directive != "INCLUDE" or len(tokens) != 3:
      continue
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
    if member_kind == "MATRIX":
      validate_slug(member_name, "matrix name")
      members.append(("matrix", member_name))
      continue
    raise ValueError(f"Unsupported INCLUDE kind '{member_kind}'")
  raise ValueError("Suite MEMBERS is missing END")


def _parse_suite_source(path: Path, ) -> tuple[str, list[tuple[str, str]]]:
  lines = _line_feed(path)
  return path.stem, _parse_members(lines, path)


def parse_suite(
    path: Path,
    discovered: dict[tuple[str, str], Path] | None = None,
) -> tuple[list[MemberDiscard], list[AcceptedMember]]:
  _suite_name, members = _parse_suite_source(path)
  if discovered is None:
    return [], [
        AcceptedMember(
            kind=member_kind,
            name=member_name,
            path=path.parent / f"{member_name}.{member_kind}",
        ) for member_kind, member_name in members
    ]

  discarded: list[MemberDiscard] = []
  discovered_members: list[AcceptedMember] = []
  for member_kind, member_name in members:
    member = (member_kind, member_name)
    if member in discovered:
      discarded.append(
          MemberDiscard(
              kind=member_kind,
              name=member_name,
              reason="Member was already discovered; cyclic or duplicate include",
          )
      )
      continue
    member_path = path.parent / f"{member_name}.{member_kind}"
    discovered[member] = member_path
    discovered_members.append(AcceptedMember(kind=member_kind, name=member_name, path=member_path))
  return discarded, discovered_members
