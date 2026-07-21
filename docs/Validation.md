# Validation

This document is the contributor-facing entry point for validating *At A Glance*. It explains what validation is for, which validation path to use, and what evidence a change should produce.

## Adjacent

- [ProductInvariants](ProductInvariants.md) for the product properties under review.
- [Contributing](Contributing.md) for contributor workflow and review discipline.
- [BuildandInstall](BuildandInstall.md) describes build and install flows and associated commands.

## Validation Goals

The validation layer helps you confirm that watch face changes continue to satisfy architecture and visual invariants. Core capabilities:

1. exercise runtime paths affected by a change
2. capture evidence for inspection
3. make release checks repeatable
4. keep visual signoff explicit and human-owned

## Validation Invariants

- Maintain clean separation between QA and watch face runtimes.
- Exercise the same runtime-message contract as the watch face.
- Scenario evidence must be repeatable and comparable across runs.
- Generated validation artifacts are not saved to the repository.
- The operator reviews evidence and owns signoff.

## Choose A Validation Path

Use the narrowest path that proves the change.

| Change type | Minimum validation |
| --- | --- |
| Documentation only | Read the touched docs for contradictions and run Markdown/link checks when practical. |
| Build tooling | Run the affected build or compile-database path. |
| Runtime C change | Run `pebble build` and a scenario or manual emulator path that exercises the changed behavior. |
| Settings, AppMessage, or persistence | Validate defaults, invalid values, applied behavior, and restart behavior where applicable. |
| UI layout, palette, glyph, or visual state | Capture screenshots or run a screenshot-producing scenario and perform manual visual review. For display-mode changes, confirm the mode changes and the watch repaints. |
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
- three-click Back-button display-mode changes and repaint behavior
- weather, battery, health, and display-mode behavior
- screenshot evidence for visual review
- manual config-page review
- manual hardware-install check when release confidence requires it
- known limitations or unchecked items

`pre-release-gate` is the canonical pre-release QA plan that comprehensively validates watch face code paths.

```
./aag-build-qa.sh --qaplan pre-release-gate
```

## Validation of Watch Face Functionality

### 1. Using QA Scenarios and Suites

Out-of-the-box targets:

- `canary`: Minimal proof that qa plan loading and step execution work.
- `dev-smoke`: Fast daily confidence on the `qa-emery` suite.
- `pre-release-gate`: Representative pre-release validation with screenshot evidence and manual signoff across the emulator QA suites.

Use the `aag-build-qa.sh` entry point to execute defined qa plans, review and compare past qa runs, and validate qa plans for correctness.

```sh
./aag-build-qa.sh --qaplan canary
./aag-build-qa.sh --qaplan dev-smoke
./aag-build-qa.sh --qaplan pre-release-gate

./aag-build-qa.sh --runs
./aag-build-qa.sh --view <run-id-or-path>
./aag-build-qa.sh --compare <run-a> [run-b ...]
./aag-build-qa.sh --validate <name-or-path>
```

Use `--dry-run` with a qa plan to inspect the resolved execution plan without creating a run:

```sh
./aag-build-qa.sh --qaplan dev-smoke --dry-run
```

Use `--force` only after reviewing the resolved execution plan.

QA runs write evidence under: `qa/qa-runs/<run-id>/`

Typical evidence includes:

- `summary.md`
- `report.json`
- `commands.log`
- `screenshots/`, when screenshots are enabled

To view prior qa plan execution results:

```s
./aag-build-qa.sh --runs
./aag-build-qa.sh --view <run-id>
```

### 2. Using the `pebble` emulator

Use manual emulator commands for focused diagnosis or when a qa plan would be too broad for the change under review.

#### Battery

```sh
pebble emu-battery --emulator emery --percent 19
pebble emu-battery --emulator emery --percent 75 --charging
```

#### Weather And Display Mode

Weather and display-mode validation use AppMessage. The operational command examples below intentionally use numeric keys because they exercise the same watch ingress used by runtime validation.

```sh
pebble send-app-message --emulator emery --int 10002=700 10003=95 10004=0
pebble send-app-message --emulator emery --int 10002=700 10003=95 10004=1
pebble send-app-message --emulator emery --int 10002=-32768 10003=-1 10004=0
pebble send-app-message --emulator emery --int 10005=15
pebble send-app-message --emulator emery --int 10006=0
pebble send-app-message --emulator emery --int 10006=2
```

Use [SettingsandConfiguration](SettingsandConfiguration.md) for the message-key catalog and transport contract.

#### One-Shot Health

One-shot health keys are QA-oriented runtime overrides. They affect the next health refresh only.

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

### 3. Visually using Debug Builds

`ATAGLANCE_DEBUG` is an optional build-time gate for visual and runtime diagnosis.

Activation point:

```text
wscript
  -> ctx.env.append_value('DEFINES', ['ATAGLANCE_DEBUG=1'])
```

Use this path when layer bounds, glyph bounds, or debug-only runtime evidence would materially improve review. Re-comment the define before normal release builds unless the debug behavior is intentionally being inspected.

## Read Next

- [QA_Readme](../qa/README.md) for QA commands, plans, and artifacts.
