# QA System

This directory contains the implementation of the At A Glance QA System used by
`../aag-build-qa.sh`.

Use this document to understand how the harness is built. Use
[../docs/Validation.md](../docs/Validation.md) to decide which validation path a
contributor should run and what evidence a change needs.

## Public Entrypoint

The public entrypoint is:

```sh
./aag-build-qa.sh
```

The entrypoint owns CLI parsing and top-level orchestration. It delegates
scenario parsing, run bootstrap, report generation, stage execution, Pebble CLI
interaction, and capability dispatch to the implementation files in `qa/`.

## Core Boundary

Shell owns:

- CLI argument parsing in `../aag-build-qa.sh`
- request validation through `lib/validate.sh`
- stage execution through `stages/`
- Pebble CLI calls through `lib/pebble.sh`
- AppMessage helpers through `lib/appmessage.sh`
- scenario-step dispatch through `tests/`
- run closeout through `lib/runtime.sh`

Python owns:

- scenario parsing and include expansion
- artifact identity generation
- run bootstrap state
- report generation
- run listing
- report lookup
- run comparison

Each layer has distinct responsibilities, and any overlap between layers has been recorded as an explicit exception / decision.

## Command Classes

### 1. Read-only coomands
These do not create a new `qa/qa-runs/<run-id>/` directory.

- `--help`
- `--runs`
- `--view <run-id-or-path>`
- `--compare <run-id-or-path> [run-id-or-path ...]`
- `--validate-scenario <name-or-path>`
- `--dry-run` with `--scenario <name>`

### 2. Direct commands
These do not bootstrap a run, do not create `qa/qa-runs/<run-id>/`, do not
emit a report, and write command output directly to stdout.

- `--build`
- `--build-clean`
- `--install`
- `--phone <ip>`
- `--wipe`
- `--nuclear`

### 3. Scenario-run commands
These bootstrap a run, create artifacts, execute selected stages, and emit closeout evidence.

- `--scenario <name>`

## Scenario Execution

Scenario files live in `scenarios/`.

Scenario execution uses this flow:

```text
aag-build-qa.sh --scenario <name>
  -> Python loads and validates qa/scenarios/<name>.scenario
  -> Python expands INCLUDE entries into a flat ordered STEP list
  -> harness prints the resolved execution plan
  -> operator confirms execution unless --force is present
  -> Python bootstraps run state and writes scenario_steps.json
  -> shell reads one concrete step at a time
  -> shell dispatches to qa/tests/<capability>.sh
  -> shell records stage, step, assertion, command, and screenshot evidence
  -> Python finalizes report.json and report.md
```

Read [QAScenarioGrammar.md](QAScenarioGrammar.md) for the scenario grammar, step
shape, artifact identity rules, and scenario-authoring guide.

## Artifact Model

Run artifacts live under ./qa so validation evidence survives clean builds
and remains comparable across runs:

```text
qa/qa-runs/<run-id>/
```

Typical run artifacts are:

- `run.json`: requested and resolved run state
- `scenario_steps.json`: concrete steps after scenario expansion
- `commands.log`: commands executed by the harness
- `report.json`: machine-readable report
- `report.md`: human-readable report
- `screenshots/index.tsv`: screenshot index
- `screenshots/*.png`: captured screenshots when enabled
- `logs/*.log`: command output logs

Comparison artifacts are written under `qa/comparisons/`.

## Report Model

Summary reports are generated during finalization. to facilitate evidence inspction, include screenshots, and validation activities.

## Implementation Layout

- `runner.py`: Python CLI used by the shell entrypoint.
- `python/scenarios.py`: Scenario parsing, include expansion, validation, and
  artifact identity generation.
- `python/state.py`: Bootstrap state intake from the shell entrypoint.
- `python/runtime.py`: Run bootstrap, finalization, report emission, run listing,
  report lookup, and comparison output.
- `lib/common.sh`: Shared shell helpers.
- `lib/state.sh`: Shell-side global state defaults and step-state loading.
- `lib/runtime.sh`: Shell-side lifecycle, artifact paths, command logging,
  screenshot indexing, assertion recording, scenario dispatch, and closeout.
- `lib/validate.sh`: Request validation.
- `lib/pebble.sh`: Pebble CLI wrappers.
- `lib/appmessage.sh`: AppMessage transport helpers.
- `stages/reset.sh`: Emulator reset and cleanup actions.
- `stages/build.sh`: Pebble build stage.
- `stages/compile_db.sh`: Compile database generation.
- `stages/install.sh`: Emulator and phone install stage.
- `tests/weather.sh`: Weather step dispatcher.
- `tests/battery.sh`: Battery step dispatcher.
- `tests/health.sh`: Health step dispatcher.
- `data/all_qa_vectors.sh`: Static supported emulator and vector data.
- `scenarios/`: Named scenario files.
- `fixtures/`: Invalid scenario inputs used to prove grammar rejection.
- `qa-runs/`: Run artifacts.
- `comparisons/`: Comparison reports.

## Further Reading

- [QAScenarioGrammar.md](QAScenarioGrammar.md) for scenario grammar, step shape,
  artifact identity, and authoring guidance.
- [../docs/Validation.md](../docs/Validation.md) for the contributor-facing
  validation contract.
- [../docs/Build.md](../docs/Build.md) for build, install, and editor-tooling
  support.
- [../docs/Contributing.md](../docs/Contributing.md) for contributor workflow and
  review discipline.
