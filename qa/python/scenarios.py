from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path


SUPPORTED_CAPABILITIES = {"weather", "battery", "health"}
SCREENSHOTS_POLICIES = {"yes", "no"}
CAPABILITY_FIELD_ORDER = {
    "weather": ("emulator", "display", "temp", "code", "is_day"),
    "battery": ("emulator", "display", "level", "charging"),
    "health": ("emulator", "display", "bpm", "steps"),
}


@dataclass(frozen=True)
class ScenarioStep:
    capability: str
    fields: dict[str, str]
    artifact_identity: str

    def as_dict(self) -> dict[str, str | dict[str, str]]:
        return {
            "capability": self.capability,
            "fields": dict(self.fields),
            "artifact_identity": self.artifact_identity,
        }

    @classmethod
    def from_dict(cls, payload: dict[str, object]) -> "ScenarioStep":
        fields = payload.get("fields", {})
        if not isinstance(fields, dict):
            fields = {}
        return cls(
            capability=str(payload.get("capability", "")),
            fields={str(key): str(value) for key, value in fields.items()},
            artifact_identity=str(payload.get("artifact_identity", "")),
        )


@dataclass(frozen=True)
class ScenarioDefinition:
    name: str
    path: Path
    screenshots_policy: str
    steps: list[ScenarioStep]


@dataclass(frozen=True)
class IncludeEntry:
    scenario_name: str


ExecutionEntry = ScenarioStep | IncludeEntry


@dataclass(frozen=True)
class ParsedScenarioFile:
    name: str
    screenshots_policy: str
    entries: list[ExecutionEntry]


def _validate_slug(value: str, label: str) -> None:
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


def _artifact_identity(capability: str, fields: dict[str, str]) -> str:
    if capability == "weather":
        return "__".join(
            [
                "weather",
                _artifact_value(fields["emulator"]),
                _artifact_value(fields["display"]),
                f"temp-{_artifact_value(fields['temp'])}",
                f"code-{_artifact_value(fields['code'])}",
                f"day-{_artifact_value(fields['is_day'])}",
            ]
        )

    if capability == "battery":
        return "__".join(
            [
                "battery",
                _artifact_value(fields["emulator"]),
                _artifact_value(fields["display"]),
                f"level-{_artifact_value(fields['level'])}",
                f"charging-{_artifact_value(fields['charging'])}",
            ]
        )

    if capability == "health":
        return "__".join(
            [
                "health",
                _artifact_value(fields["emulator"]),
                _artifact_value(fields["display"]),
                f"bpm-{_artifact_value(fields['bpm'])}",
                f"steps-{_artifact_value(fields['steps'])}",
            ]
        )

    raise ValueError(f"Unsupported STEP capability '{capability}'")


def _parse_step(tokens: list[str]) -> ScenarioStep:
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
        fields[key] = value

    expected_fields = set(CAPABILITY_FIELD_ORDER[capability])
    actual_fields = set(fields)
    missing = sorted(expected_fields - actual_fields)
    if missing:
        raise ValueError(f"STEP {capability} is missing field '{missing[0]}'")
    unknown = sorted(actual_fields - expected_fields)
    if unknown:
        raise ValueError(f"Unsupported STEP {capability} field '{unknown[0]}'")

    ordered_fields = {key: fields[key] for key in CAPABILITY_FIELD_ORDER[capability]}
    return ScenarioStep(
        capability=capability,
        fields=ordered_fields,
        artifact_identity=_artifact_identity(capability, ordered_fields),
    )


def _parse_scenario_source(path: Path, expected_scenario_name: str | None = None) -> ParsedScenarioFile:
    scenario_name = ""
    screenshots_policy = ""
    entries: list[ExecutionEntry] = []
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
                    raise ValueError("Scenario has duplicate PREAMBLE block")
                if seen_execute:
                    raise ValueError("PREAMBLE must come before EXECUTE")
                seen_preamble = True
            if directive == "EXECUTE":
                if not preamble_closed:
                    raise ValueError("EXECUTE must come after PREAMBLE END")
                if seen_execute:
                    raise ValueError("Scenario has duplicate EXECUTE block")
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
            if block == "EXECUTE":
                execute_closed = True
            block = ""
            continue

        if block == "PREAMBLE":
            if directive == "SCENARIO":
                if len(tokens) != 2:
                    raise ValueError("SCENARIO requires exactly one value")
                if scenario_name:
                    raise ValueError("Scenario has duplicate SCENARIO")
                scenario_name = tokens[1]
                _validate_slug(scenario_name, "scenario name")
                if expected_scenario_name is not None and scenario_name != expected_scenario_name:
                    raise ValueError(
                        f"Scenario '{scenario_name}' does not match requested scenario '{expected_scenario_name}'"
                    )
                continue

            if directive == "SCREENSHOTS":
                if len(tokens) != 2:
                    raise ValueError("SCREENSHOTS requires exactly one policy")
                if screenshots_policy:
                    raise ValueError("Scenario has duplicate SCREENSHOTS")
                if tokens[1] not in SCREENSHOTS_POLICIES:
                    raise ValueError(f"Unsupported SCREENSHOTS policy '{tokens[1]}'")
                screenshots_policy = tokens[1]
                continue

            raise ValueError(f"Unsupported PREAMBLE directive '{directive}'")

        if block == "EXECUTE":
            if directive == "STEP":
                entries.append(_parse_step(tokens))
                continue

            if directive == "INCLUDE":
                if len(tokens) != 2:
                    raise ValueError("INCLUDE requires exactly one scenario name")
                _validate_slug(tokens[1], "scenario name")
                entries.append(IncludeEntry(tokens[1]))
                continue

            raise ValueError(f"Unsupported EXECUTE directive '{directive}'")

        raise ValueError(f"Directive '{directive}' appears outside PREAMBLE or EXECUTE")

    if block:
        raise ValueError(f"Scenario has unclosed {block} block")
    if not seen_preamble:
        raise ValueError("Scenario is missing PREAMBLE")
    if not preamble_closed:
        raise ValueError("Scenario PREAMBLE is missing END")
    if not scenario_name:
        raise ValueError("Scenario is missing SCENARIO")
    if not screenshots_policy:
        raise ValueError("Scenario is missing SCREENSHOTS")
    if not seen_execute:
        raise ValueError("Scenario is missing EXECUTE")
    if not execute_closed:
        raise ValueError("Scenario EXECUTE is missing END")
    if not entries:
        raise ValueError("Scenario has no EXECUTE entries")

    return ParsedScenarioFile(name=scenario_name, screenshots_policy=screenshots_policy, entries=entries)


def _expand_entries(
    path: Path,
    parsed: ParsedScenarioFile,
    stack: list[str],
    stack_members: set[str],
) -> list[ScenarioStep]:
    steps: list[ScenarioStep] = []
    scenarios_dir = path.parent

    for entry in parsed.entries:
        if isinstance(entry, ScenarioStep):
            steps.append(entry)
            continue

        include_path = scenarios_dir / f"{entry.scenario_name}.scenario"
        if not include_path.is_file():
            raise ValueError(f"Unknown included scenario '{entry.scenario_name}'")
        if entry.scenario_name in stack_members:
            cycle = " -> ".join([*stack, entry.scenario_name])
            raise ValueError(f"Scenario INCLUDE cycle detected: {cycle}")

        included = _parse_scenario_source(include_path, expected_scenario_name=entry.scenario_name)
        steps.extend(
            _expand_entries(
                include_path,
                included,
                [*stack, entry.scenario_name],
                {*stack_members, entry.scenario_name},
            )
        )

    return steps


def load_scenario_file(path: Path, expected_scenario_name: str | None = None) -> ScenarioDefinition:
    parsed = _parse_scenario_source(path, expected_scenario_name=expected_scenario_name)
    steps = _expand_entries(path, parsed, [parsed.name], {parsed.name})

    if not steps:
        raise ValueError("Scenario has no STEP directives")

    return ScenarioDefinition(
        name=parsed.name,
        path=path,
        screenshots_policy=parsed.screenshots_policy,
        steps=steps,
    )


def load_scenario(scenario_name: str, scenarios_dir: Path) -> ScenarioDefinition:
    _validate_slug(scenario_name, "scenario name")
    path = scenarios_dir / f"{scenario_name}.scenario"
    if not path.is_file():
        raise ValueError(f"Unknown scenario '{scenario_name}'")

    return load_scenario_file(path, expected_scenario_name=scenario_name)


def expected_step_screenshot_count(step: ScenarioStep) -> int:
    if step.capability not in SUPPORTED_CAPABILITIES:
      raise ValueError(f"Unsupported STEP capability '{step.capability}'")
    return 1
