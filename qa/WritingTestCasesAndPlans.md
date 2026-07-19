# QA Scenario Grammar

This document is the source of truth for QA scenario composition, step shape,
artifact identity, and scenario authoring.

Use [README.md](README.md) for the QA System implementation architecture. Use
[../docs/Validation.md](../docs/Validation.md) to decide which validation path a
contributor should run.

## Purpose

Scenarios are plain-text compositions of concrete steps. They are not a generic
test DSL, a matrix system, or a place for implied defaults.

The grammar exists to make validation actions explicit, repeatable, and
comparable across runs.

## File Shape

```text
PREAMBLE
  SCENARIO <name>
  SCREENSHOTS <yes|no>
END

EXECUTE
  STEP <capability> <field=value>...
  INCLUDE <scenario-name>
END
```

## Rules

- `#` starts a comment anywhere in the file.
- `PREAMBLE` owns metadata only.
- `EXECUTE` owns ordered work only.
- `SCENARIO` names the scenario file.
- `SCREENSHOTS` records whether the scenario expects screenshots.
- `STEP` declares one concrete validation action.
- `INCLUDE` composes another scenario's `EXECUTE` entries at the include site.
- Included scenario `PREAMBLE` metadata does not override the parent scenario.
- Include cycles are invalid.
- Scenario names are not part of step identity.
- Scenario names use lowercase letters, digits, and hyphens.

## Step Grammar

```text
STEP <capability> <field=value>...
```

Supported capabilities:

- `weather`
- `battery`
- `health`

## Step Rules

- A step contains concrete execution values.
- A step is not a matrix, preset, alias, or mini-scenario.
- Field order is fixed per capability so the same step produces the same
  artifact identity across runs.
- Scenario composition does not change the step's concrete values.
- Every supported field is required.
- Unknown fields are rejected.
- Duplicate fields are rejected.
- Empty field values are rejected.
- Artifact identity values use lowercase letters, digits, and hyphens.

## Capability Fields

### Weather

```text
STEP weather emulator=<target> display=<mode> temp=<celsius-tenths> code=<weather-code> is_day=<0|1>
```

Fields:

- `emulator`: Pebble emulator target.
- `display`: Display mode token used by the QA step.
- `temp`: Temperature in Celsius tenths.
- `code`: Weather condition code.
- `is_day`: `1` for day glyph behavior, `0` for night glyph behavior.

### Battery

```text
STEP battery emulator=<target> display=<mode> level=<percent> charging=<0|1>
```

Fields:

- `emulator`: Pebble emulator target.
- `display`: Display mode token used by the QA step.
- `level`: Battery percentage.
- `charging`: `1` when plugged in or charging, `0` when unplugged.

### Health

```text
STEP health emulator=<target> display=<mode> bpm=<value> steps=<value>
```

Fields:

- `emulator`: Pebble emulator target.
- `display`: Display mode token used by the QA step.
- `bpm`: One-shot heart-rate value.
- `steps`: One-shot steps value.

## Artifact Identity

The engine derives artifact identity from the normalized step values.

Examples:

```text
weather__emery__white__temp-539__code-65__day-1
battery__chalk__black__level-25__charging-0
```

## Include Rules

- `INCLUDE` is legal only inside `EXECUTE`.
- Include expansion produces one flat ordered step list.
- Included steps keep artifact identity based on their concrete values.
- Included steps are executed at the include site.
- Included scenario names do not appear in artifact identity.

## Build A Step

To add a step to an existing scenario:

1. Choose the capability that matches the product behavior under validation.
2. Provide every required field for that capability.
3. Use concrete values only.
4. Keep the field names exactly as defined in this document.
5. Run scenario validation before running the scenario.

Example:

```text
STEP weather emulator=emery display=white temp=539 code=65 is_day=1
```

This is a good step because it is concrete. It identifies one emulator, one
display mode, one weather state, and one day/night state.

## Build A Scenario

To create a scenario:

1. Create `qa/scenarios/<name>.scenario`.
2. Add a `PREAMBLE` block.
3. Set `SCENARIO <name>` to match the filename without `.scenario`.
4. Set `SCREENSHOTS yes` when screenshot evidence is required.
5. Add an `EXECUTE` block.
6. Add ordered `STEP` and `INCLUDE` entries.
7. Validate the scenario.

Validation command:

```sh
./aag-build-qa.sh --validate-scenario <name>
```

Dry-run command:

```sh
./aag-build-qa.sh --scenario <name> --dry-run
```

Execution command:

```sh
./aag-build-qa.sh --scenario <name>
```

Use `--force` only when the resolved execution plan has already been reviewed.

## Change The Grammar

Changing the grammar requires updating all affected owners in one slice:

- `python/scenarios.py`
- `QAScenarioGrammar.md`
- capability dispatch in `lib/runtime.sh`
- capability implementation under `tests/`, when a new capability is added
- report expectations in `python/runtime.py`, when artifact shape changes
- scenario files under `scenarios/`, when existing scenarios need new fields

## Example: Single Step

```text
PREAMBLE
  SCENARIO weather-clear-day
  SCREENSHOTS yes
END

EXECUTE
  STEP weather emulator=emery display=white temp=539 code=0 is_day=1
END
```

## Example: Composed Scenario

```text
PREAMBLE
  SCENARIO run-them-all
  SCREENSHOTS yes
END

EXECUTE
  INCLUDE weather
  INCLUDE battery
END
```

## Current Scenario Set

- `canary`: minimal parser and harness proof.
- `dev-smoke`: fast daily confidence on `emery`.
- `release-core`: representative pre-release scenario with screenshot evidence.
- `release-full`: broad supported-emulator scenario with screenshot evidence.
