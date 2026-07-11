## QA System Design

The QA System is a **hybrid-evidence, local-first, CI-ready validation framework**
that supports the following validation outcomes:

1. Run repeatable validation flows.
2. Keep QA stage boundaries explicit.
3. Exercise the same runtime-message contract as the watch face.
4. Capture evidence that a reviewer can inspect and compare.
5. Generate a summary report that displays results, and is human-readable
   and immediately actionable
6. Visual signoff is explicitly manual.

These are explicit non-goals of the QA System:

1. Building API and hooks for future CI-style execution.
2. Releasing the validation layer as a stand-alone product.
3. Building frameworks and tooling beyond needs of current scenarios.
4. Managing the lifecycle of generated artifacts.

## Current Implementation Layout

The current implementation shape is:

```text
qa/
  README.md
  runner.py
  lib/
    common.sh
    runtime.sh
    validate.sh
    pebble.sh
    appmessage.sh
  stages/
    reset.sh
    build.sh
    compile_db.sh
    install.sh
  tests/
    weather.sh
    battery.sh
    health.sh
    smoke.sh
  data/
    all_qa_vectors.sh
  scenarios/
    dev-smoke.scenario
    release-core.scenario
    release-full.scenario
```

## QA System Primitives

1. build
2. install
3. reset
4. scenario execution
5. artifact capture
6. report emission

## Scenarios: Putting the QA Pieces Together

**Scenarios** encapsulate workflows that connect system primitives to validate
watch face functionality or automate the contributor / operator experience.

Examples are:

- IDE bootstrap: `build`, `compile database generation`
- Phone install: `build`, `phone install`
- Weather validation: `install`, `weather automation`
- Smoke testing: `build`, `install`, `sequence a series of tests`

### Out of the Box Scenarios

1. `dev-smoke`
  - purpose: fast daily-use confidence
  - required emulator set: `emery`
  - required coverage: smoke test type only
  - screenshots: optional
  - reports: required once report generation exists

2. `release-core`
  - purpose: representative pre-release confidence
  - required emulator set: `emery`, `flint`, `chalk`, `aplite`
  - required coverage: curated smoke, weather, battery, and health checkpoints
  - screenshots: mandatory
  - reports: mandatory
  - manual signoff: mandatory

3. `release-full`
  - purpose: full supported-emulator confidence
  - required emulator set: `aplite`, `basalt`, `chalk`, `diorite`, `emery`,
    `flint`, `gabbro`
  - required coverage: full scenario matrix
  - screenshots: mandatory
  - reports: mandatory
  - manual signoff: mandatory

4. `canary`
  - purpose: tiny proof scenario used to validate one harness contract at a time

Config page validation and hardware install remain explicit manual or
operator-assisted checks.

### Creating New Scenarios

Scenario files live under `qa/scenarios/`. They are plain-text files for
watch-face validation, not a generic test DSL.

#### Scenario Grammar

The grammar has two blocks:

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

**Notes**

- `#` starts a comment anywhere in the file.
- `PREAMBLE` owns metadata only.
- `EXECUTE` owns ordered work only.
- `INCLUDE` concatenates another scenario's `EXECUTE` entries at the include
  site.
- Included scenario `PREAMBLE` metadata does not override the parent scenario.
- Include cycles are invalid.

#### Step

A `STEP` is one concrete validation action.

```text
STEP <capability> <field=value>...
```

Supported capabilities:

- `weather`
- `battery`
- `health`

**Notes**

- A step is not a matrix, preset, alias, or mini-scenario.
- A step contains concrete execution values.
- Artifact identity is generated from the normalized step values.
- Scenario names are not part of artifact identity.

Examples:

```text
STEP weather emulator=emery display=white temp=539 code=65 is_day=1
STEP battery emulator=chalk display=black level=25 charging=0
```

#### Artifact Identity

The engine derives artifact identity from the concrete step.

**Notes**

- Do not ask users to provide checkpoint ids, artifact ids, or primary keys.
- Do not include dates, run ids, scenario names, or free-form labels.
- Use fixed field order per capability so the same step has the same artifact
  key across runs.

Examples:

```text
weather__emery__white__temp-539__code-65__day-1
battery__chalk__black__level-25__charging-0
```

#### Include

`INCLUDE` composes scenarios without nesting execution semantics.

```text
INCLUDE <scenario-name>
```

**Notes**

- `INCLUDE` is legal only inside `EXECUTE`.
- Include expansion produces one flat ordered step list.
- Included steps keep artifact identity based on their concrete values.

Examples:

```text
INCLUDE weather
INCLUDE battery
```

#### Full Examples

```text
# One concrete weather artifact.
PREAMBLE
  SCENARIO weather-clear-day
  SCREENSHOTS yes
END

EXECUTE
  STEP weather emulator=emery display=white temp=539 code=0 is_day=1
END
```

```text
# Compose existing scenario execution blocks.
PREAMBLE
  SCENARIO run-them-all
  SCREENSHOTS yes
END

EXECUTE
  INCLUDE weather
  INCLUDE battery
END
```

#### Scenario Execution

Use the harness entrypoint to execute scenarios:

```sh
./aag-build-qa.sh --scenario canary
./aag-build-qa.sh --scenario dev-smoke
./aag-build-qa.sh --scenario release-core
```

Before execution, the harness must print the resolved execution plan and ask
for confirmation. The resolved plan is the flat ordered list produced after
`INCLUDE` expansion.

**Notes**

- `-n` / `--dry-run` prints the resolved execution plan and exits without
  executing.
- `--force` skips the confirmation prompt and executes.
- `--nuclear` remains explicit and has no short flag.

#### Scenario Validation

Scenario validation is a harness responsibility. Do not make user-facing docs
depend on the implementation language of the scenario engine.

**Notes**

- The current implementation validates scenarios through `qa/runner.py`, but
  that is an internal implementation detail.
- The stable user-facing ABI is `./aag-build-qa.sh`.

### Anatomy of a Scenario

Each scenario runs through some or all of these stages:

1. preflight
2. reset
3. optional build
4. optional compile database generation
5. optional install
6. scenario execution
7. cleanup
8. artifact capture
9. report emission

## Test runs

**Canonical** invocation syntax:

```sh
./aag-build-qa.sh --scenario canary
```

A test run executes specified scenarios and has the following characteristics:

- Every run has one unique `run-id`
- A report is generated if an emulator is launched.
  - Report and supporting artifact(s) are stored in an identifiable folder.
  - Multiple supporting artifacts could be created.
- Screenshots, when generated, must be associated with a test-run.
- Reports must be clear, human-readable, and actionable with minimal cognitive load.
- Failed runs must emit partial evidence and a failure summary.

### Evidence model

Selected target remains:

- Hybrid Evidence
- Local-First, CI-Ready
- No data interpretation

**Meaning**

- Automation proves stage success, scenario completion, artifact completeness,
  and reproducibility
- Curated visual checkpoints remain human-reviewed
- The summary report shows images in their captured resolution
- Report and screenshot validation is solely the responsibility of the human operator

### Typical Test Run Outputs

Outputs from test-runs support the evidence model and are stored locally in:

- `qa/qa-runs/<run-id>/`

Typical outputs:

- `report.md`, `report.json`
- `run.json`
- `commands.log`
- `report.json`
- `screenshots/`
- `logs/` when captured
- `comparison.md`, `comparison.json`

#### Screenshot identity contract

Every captured screenshot must be identifiable by:

- capability
- emulator
- display mode
- concrete state fields

### Viewing Test Run Reports

The system supports reviewing test-run reports post execution to facilitate
operator-owned validation of the following:

  - palette, layout, and glyph tuning
  - text display
  - state changes

**Out of the box capabilities**

1. `--runs`
Displays the `run-id`, `scenario`, and `status` of the last 10-runs.

2. `--view <run-id-or-path>`
Provides the path to a human-readable `report.md` for the test-run. `run-id`
can be conveniently retrieved from `--runs`.

In addition to identifiable data, logs, and links to supporting information,
the report displays screenshots to facilitate visual validation.

This summary report is supported by:
- `report.json`
  - machine-readable stage results
  - scenario completion results
  - artifact inventory summary
  - failure summary

3. `--compare <run-a> [run-b] [run-c]`
Creates a `comparison.md` with the familiar table flow from the single test-run report.

## Further Reading

- [../docs/Validation.md](../docs/Validation.md) for the validation contract.
- [../docs/QA_Harness_Decision_Ledger.md](../docs/QA_Harness_Decision_Ledger.md)
  for accepted QA system decisions.
