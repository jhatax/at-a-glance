# Validation

This document is the contributor-facing entry point for validating At A Glance.
It explains what validation is for, which validation path to use, and what
evidence a change should produce.

Use [../qa/README.md](../qa/README.md) for how the validation system has been architected.
Use [../qa/QAScenarioGrammar.md](../qa/QAScenarioGrammar.md) for how validation steps and scenarios can be defined.

## Required Reading

- [Build.md](Build.md) for build, install, and editor-tooling support.
- [ArchitectureLedger.md](ArchitectureLedger.md) for runtime ownership and
  lifecycle boundaries.
- [UserInterface.md](UserInterface.md) for visual implementation evidence.
- [Settings_and_Configuration.md](Settings_and_Configuration.md) for settings,
  message-key, transport, and persistence contracts.

## Validation Purpose

The validation layer helps you confirm that watch face changes continue to satisfy architecture and visual invariants.

Validation exists to:

1. prove that the product still builds
2. exercise runtime paths affected by a change
3. capture evidence for inspection
4. make release checks repeatable
5. keep visual signoff explicit and human-owned

Validation makes evidence visible to exercise product judgment.

## Validation Invariants

- Maintain clean separation between its runtime and that of the watch face.
- Exercise the same runtime-message contract as the watch face.
- Scenario evidence must be repeatable and comparable across runs.
- Validation tooling doesn't attempt to interpret test reports, leaving confirmation & sign-off to the operator.
- Generated validation artifacts are not saved to the repository.

## Choose A Validation Path

Use the narrowest path that proves the change.

| Change type | Minimum validation |
| --- | --- |
| Documentation only | Read the touched docs for contradictions and run Markdown/link checks when practical. |
| Build tooling | Run the affected build or compile-database path. |
| Runtime C change | Run `pebble build` and a scenario or manual emulator path that exercises the changed behavior. |
| Settings, AppMessage, or persistence | Validate defaults, invalid values, applied behavior, and restart behavior where applicable. |
| UI layout, palette, glyph, or visual state | Capture screenshots or run a screenshot-producing scenario and perform manual visual review. |
| Release candidate | Run the release scenario path and complete manual signoff. |

If a validation path cannot be run, state the gap explicitly in the closeout.

## Operator Evidence Review Expectations

The operator should validate the following before committing a change:

- what changed
- which validation path ran
- which scenario or manual commands were used
- where the report or screenshot evidence lives
- what remains unchecked

## Release Validation

Before release, validation should cover:

- successful build
- supported emulator behavior
- settings defaults and applied values
- weather, battery, health, and display-mode behavior
- screenshot evidence for visual review
- manual config-page review
- manual hardware-install check when release confidence requires it
- known limitations or unchecked items

`release-core` is the normal pre-release scenario. Use `release-full` when the
change or release risk justifies broader supported-emulator coverage.

```sh
#exercise validation scenarios
./aag-build-qa.sh --scenario release-core
./aag-build-qa.sh --scenario release-full
```

## Diving deeper into Validation Options

### 1. Validation using QA Scenarios

Out-of-the-box scenarios:

- `canary`: Minimal proof that scenario loading and step execution work.
- `dev-smoke`: Fast daily confidence on `emery`.
- `release-core`: Representative pre-release validation with screenshot
  evidence and manual signoff.
- `release-full`: Broad supported-emulator validation with screenshot evidence
  and manual signoff.

Scenario grammar, concrete step fields, include behavior, and authoring rules
live in [../qa/QAScenarioGrammar.md](../qa/QAScenarioGrammar.md).

Use the `aag-build-qa.sh` entry point to validate scenarios.

```sh
#exercise validation scenarios
./aag-build-qa.sh --scenario canary
./aag-build-qa.sh --scenario dev-smoke
./aag-build-qa.sh --scenario release-core

#view validation reports and outputs, starting with the last-10 runs
./aag-build-qa.sh --runs
./aag-build-qa.sh --view <run-id-or-path>
./aag-build-qa.sh --compare <run-a> [run-b] [run-c]
./aag-build-qa.sh --validate-scenario <name-or-path>
```

Use `--dry-run` with a scenario to inspect the resolved execution plan without
creating a run:

```sh
./aag-build-qa.sh --scenario dev-smoke --dry-run
```

Use `--force` only after reviewing the resolved execution plan.

Scenario runs write evidence under: `qa/qa-runs/<run-id>/`

Typical evidence includes:

- `report.md`
- `report.json`
- `run.json`
- `commands.log`
- `scenario_steps.json`
- `screenshots/`, when screenshots are enabled
- `logs/`

To view evidence after a scenario run:

```sh
# to view a summary of the last 10-runs
./aag-build-qa.sh --runs
# to view evidence generated during a specific scenario
`./aag-build-qa.sh --view <run-id>` to locate a run report after execution.
```

### 2. Validation using the `pebble` emulator

Use manual emulator commands for focused diagnosis or when a scenario would be
too broad for the change under review.

#### Battery

```sh
pebble emu-battery --emulator emery --percent 19
pebble emu-battery --emulator emery --percent 75 --charging
```

#### Weather And Display Mode

Weather and display-mode validation use AppMessage. The operational command
examples below intentionally use numeric keys because they exercise the same
watch ingress used by runtime validation.

```sh
pebble send-app-message --emulator emery --int 10002=700 10003=95 10004=0
pebble send-app-message --emulator emery --int 10002=700 10003=95 10004=1
pebble send-app-message --emulator emery --int 10002=-32768 10003=-1 10004=0
pebble send-app-message --emulator emery --int 10005=15
pebble send-app-message --emulator emery --int 10006=0
pebble send-app-message --emulator emery --int 10006=2
```

Use [Settings_and_Configuration.md](Settings_and_Configuration.md) for the
message-key catalog and transport contract.

#### One-Shot Health

One-shot health keys are QA-oriented runtime overrides. They affect the next
health refresh only.

```sh
pebble send-app-message --emulator emery --int 10020=72
pebble send-app-message --emulator emery --int 10021=8500
pebble send-app-message --emulator emery --int 10020=0
pebble send-app-message --emulator emery --int 10021=0
```

#### Config Page

```sh
pebble emu-app-config
```

### 3. Visual Validation using Debug Builds

`ATAGLANCE_DEBUG` is an optional build-time gate for visual and runtime
diagnosis.

Activation point:

```text
wscript
  -> ctx.env.append_value('DEFINES', ['ATAGLANCE_DEBUG=1'])
```

Use this path when layer bounds, glyph bounds, or debug-only runtime evidence
would materially improve review. Re-comment the define before normal release
builds unless the debug behavior is intentionally being inspected.

## Further Reading

- [../qa/README.md](../qa/README.md) for QA System implementation details.
- [../qa/QAScenarioGrammar.md](../qa/QAScenarioGrammar.md) for scenario and step
  authoring.
- [Build.md](Build.md) for build, install, and editor-tooling support.
- [Contributing.md](Contributing.md) for contributor workflow and review
  discipline.
- [Settings_and_Configuration.md](Settings_and_Configuration.md) for settings,
  message-key, and transport obligations.
