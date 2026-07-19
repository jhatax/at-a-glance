# Writing Test Cases And Plans

This document is the operator guide for writing QA test cases and plans.

**Key document relationships**

```text
`Validation` describes validation and introduces the QA harness
|
V
`qa/README` dives into executing and creating QA plans for the watch face
  |_ `this` covers QA plan authoring, including a constrained grammar for composing steps and plans into larger suites
  |_ `QA_Harness_Implementation_Flow` details the QA harness's flow and capabilities
```

## Example Test Plans

### Scenario with a Single Step

```text
PREAMBLE
  SCENARIO weather-clear-day
  SCREENSHOTS yes
  EMULATORS emery
END

EXECUTE
  STEP weather display=white temp=539 code=0 is_day=1
END
```

### Suite that composes two scenarios

```text
PREAMBLE
  SUITE run-subset
END

MEMBERS
  INCLUDE SCENARIO weather-clear-day
  INCLUDE SCENARIO battery-charging
END
```
## Test-Plan Anatomy

A QA plan can be visualized as a hierarchy:

- A `step` sends one concrete product state to one capability.
- A `scenario` declares policy: emulators, screenshots, and composes `steps` in order.
- A `suite` composes `scenarios` and `suites` in order.

The selected suite or scenario is the test plan. The `qa-release-gate` suite, by definition, is a composition of ordered suites and covers all watch face capabilities.

## Build A Step

1. Choose the capability {weather, battery, health} that matches the product behavior under validation.
2. Provide every required field for that capability.
3. Use concrete values only.
4. Keep the field names exactly as defined in this document.
5. Run scenario validation before running the scenario.

Example:

```text
STEP weather display=white temp=539 code=65 is_day=1
```

This is a good step because it is concrete. It identifies one display mode, one weather state, and one day/night state within the scenario's emulator set.

## Build A Scenario

**Invariants**

- A scenario specifies `SCREENSHOTS`, `EMULATORS`, and `STEP`.
- Unique `steps` are executed in order.

To create a scenario:

1. Create `qa/scenarios/<name>.scenario`.
2. Add a `PREAMBLE` block.
3. Set `SCENARIO <name>` to match the filename without `.scenario`.
4. Set `SCREENSHOTS yes` when screenshot evidence is required.
5. Declare the scenario emulator set with `EMULATORS`.
6. Add an `EXECUTE` block.
7. Add ordered `STEP` entries.
8. Validate the scenario.

## Build A Suite

**Invariants**

- A suite is ordered composition only.
- A suite has no `SCREENSHOTS`, `EMULATORS`, or `STEP` entries.
- `INCLUDE` is legal only inside `MEMBERS`.
- A suite member must declare whether it includes a `SCENARIO` or a `SUITE`.
- Suite expansion preserves member order.
- Included scenarios keep their own emulator scope.
- Included scenarios keep artifact identity based on their concrete values.

To create a suite:

1. Create `qa/scenarios/<name>.suite`.
2. Add a `PREAMBLE` block.
3. Set `SUITE <name>` to match the filename without `.suite`.
4. Add a `MEMBERS` block.
5. Add ordered `INCLUDE SCENARIO` and `INCLUDE SUITE` entries.
6. Validate the suite.

## Out-of-the-box Validation QA Plans

The following `suites and scenarios` will accelerate your development and release work:

- `canary`: minimal parser and harness proof.
- `dev-smoke`: fast daily confidence on the `qa-emery` suite.
- `qa-<emulator>` suites: one suite per emulator, composed from capability scenarios.
- `pre-release-gate`: top-level pre-release suite composed from the emulator QA suites.

Refer to `qa/scenarios/` for all qa plan specifications.

## Grammar to specify & govern QA plans

Scenarios are plain-text compositions of concrete steps. The grammar exists to make validation actions explicit, repeatable, and comparable across runs.

### File Shapes

```text
PREAMBLE
  SCENARIO <name>
  SCREENSHOTS <yes|no>
  EMULATORS <target>...
END

EXECUTE
  STEP <capability> <field=value>...
END
```

```text
PREAMBLE
  SUITE <name>
END

MEMBERS
  INCLUDE SCENARIO <scenario-name>
  INCLUDE SUITE <suite-name>
END
```

### Rules

- `#` starts a comment anywhere in the file.
- `PREAMBLE` owns metadata only.
- `EXECUTE` owns ordered step work only.
- `MEMBERS` owns ordered suite composition only.
- `SCENARIO` names the scenario file.
- `SUITE` names the suite file.
- `SCREENSHOTS` records whether the scenario expects screenshots.
- `EMULATORS` declares the emulator set for the whole scenario.
- `STEP` declares one concrete validation action.
- `INCLUDE SCENARIO` composes one scenario into a suite.
- `INCLUDE SUITE` composes one suite into another suite.
- `SUITE` and `SCENARIO` include cycles are invalid.
- Scenario names are not part of step identity.
- Scenario, suite names use lowercase letters, digits, and hyphens.
- Emulator names, strictly lower-case, are compared against a fixed set of supported emulators (all pebbles).

The grammar's explicit rules and implied hierarchy (Suite > Scenario > Step) are enforced by the parser.

## Steps

```text
STEP <capability> <field=value>...
```

Supported capabilities: `weather` | `battery` | `health`

## Rules

- A step contains concrete execution values.
- Field order is fixed per capability so the same step produces the same artifact identity across runs.
- Scenario composition does not change the step's concrete values.
- Every supported field is required.
- Unknown, duplicate, empty fields are rejected.

## Supported Capabilities

### 1. Weather

```text
STEP weather display=<mode> temp=<celsius-tenths> code=<weather-code> is_day=<0|1>
```

Fields:

- `display`: Display mode token used by the QA step.
- `temp`: Temperature in Celsius tenths.
- `code`: Weather condition code.
- `is_day`: `1` for day glyph behavior, `0` for night glyph behavior.

### 2. Battery

```text
STEP battery display=<mode> level=<percent> charging=<0|1>
```

Fields:

- `display`: Display mode token used by the QA step.
- `level`: Battery percentage.
- `charging`: `1` when plugged in or charging, `0` when unplugged.

### 3. Health

```text
STEP health display=<mode> bpm=<value> steps=<value>
```

Fields:

- `display`: Display mode token used by the QA step.
- `bpm`: One-shot heart-rate value.
- `steps`: One-shot steps value.

## Change The Grammar

Changing the grammar requires updating all affected owners in one slice:

  - `python/scenarios.py`
  - `WritingTestCasesAndPlans.md`
  - scenario and suite files under `scenarios/`, when existing targets need new fields
  - capability implementation under `tests/`, when a new capability is added
  - report expectations in `python/runtime.py`, when artifact shape changes

## Grammar Fixtures

Fixtures provide accepted and rejected plan files for parser checks:

- `qa/fixtures/scenarios/` contains valid scenarios and suites.
- `qa/fixtures/invalid-scenarios/` contains invalid plans.

Run the executable fixture checks from the repository root:

```sh
python3 -m unittest discover -s qa/tests -p 'test_*.py'
```
