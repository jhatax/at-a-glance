# QA Tooling

This directory contains the modular build and validation implementation used by
`../ataglance_build_test_harness.sh`.

Layout:

- `lib/`: shared helpers for logging, validation, Pebble CLI wrappers, and
  AppMessage helpers
- `stages/`: build, compile database, reset, and install stages
- `tests/`: validation families such as weather, battery, health, and smoke
- `data/`: static vectors used by the validation families

Public entrypoint:

- `../ataglance_build_test_harness.sh`

The top-level harness owns CLI parsing and stage orchestration. The files in
this directory own the implementation details of those stages and validation
flows.

## Test Descriptions

- `weather`: exercises weather-condition, day/night, and display-mode sweeps
- `battery`: exercises battery-level and charging-state sweeps across display modes
- `health`: exercises BPM and steps overrides, including unavailable-state coverage
- `smoke`: exercises a subset of scenarios across battery, weather, and health

## Further Reading

- `../docs/ArchitectureLedger.md` for the runtime architecture.
- `../docs/Contributing.md` for contributor workflow, validation, and review discipline.
- `../docs/Build and Validation.md` for build, install, editor-tooling, and validation architecture.
