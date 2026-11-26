# Validation

Use this document to choose a validation path for a change and identify the evidence to inspect before committing or releasing it.

## Adjacent

- [ProductInvariants](ProductInvariants.md) for the product properties under review.
- [Contributing](Contributing.md) for contributor workflow and review discipline.
- [BuildandInstall](BuildandInstall.md) describes build and install flows and associated commands.

## Validation Goals

Exercise the changed path, capture repeatable evidence, and inspect the result. A build, packet send, or passed harness step confirms only what that check observes. The operator owns visual and release signoff.

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
| Runtime C change | Run the relevant native C cases, then `pebble build` and a scenario or manual emulator path that exercises the changed behavior. |
| PKJS climate/location change | Run the JavaScript tests, then exercise the changed path with PKJS and the emulator or phone companion. |
| Settings, AppMessage, or persistence | Run the relevant native C and JavaScript cases. Validate defaults, invalid values, applied behavior, and restart behavior where applicable. |
| UI layout, palette, glyph, or visual state | For layout calculations, run the native layout suite across the full matrix. Capture screenshots or run a screenshot-producing scenario and perform manual visual review. For display-mode changes, confirm the mode changes and the watch repaints. |
| Release candidate | Run the release scenario path and complete manual signoff. |

If a validation path cannot be run, state the gap explicitly in the closeout.

## Automated Tests

Run all automated suites from the repository root:

```sh
npm test
```

This runs the native C, JavaScript, and Python harness suites in sequence. It does not build the watchface or run an emulator scenario. [Contributing](Contributing.md) lists prerequisites and individual suite commands.

## Native C Tests

The native suite runs production C against mocked Pebble dependencies on the developer's computer. Six suites cover settings persistence and migration, message parsing, runtime dispatch, communications, shared helpers, and calculated layout geometry. It requires Clang, Python 3, and a POSIX shell; it needs no Pebble SDK or emulator.

```sh
npm run test:native
npm run test:native -- --suite layout --matrix full --bounds --keep-build
```

The default runs all suites on Emery, Chalk, and Flint with health enabled, plus Aplite without health. `npm test` and both Git hooks use this matrix. Each case runs in a separate process, so a crash does not stop later cases. Failures identify the case and include a replay command; assertion diagnostics can also include input context and captured calls.

The layout command checks all supported geometries with both health configurations and battery orientations. Its bounds output lists each item's origin, width, height, and bottom-right coordinate in Y/X order. `--keep-build` retains per-case logs under `build/native-tests/<platform>-<variant>/`. Circle-intersection observations identify text or image frames that need visual inspection. The operator or developer owns visual validation.

The [Native C tests guide](../tests/native/README.md) explains suite and case selection, result interpretation, retained logs, sanitizers, architecture, and authoring. Use it to narrow a failure or add coverage; use the emulator paths below to check runtime integration and rendered behavior.

## JavaScript Tests

Run the PKJS tests from the repository root with Node.js installed:

```sh
npm run test:js
```

To run only the climate/location tests:

```sh
node --test tests/js/weather_location.test.js
```

The tests use Node's built-in test runner and one shared harness in `tests/js/harness.js`. Each test gets an isolated environment running the production entry point, refresh controller, and message queues. The harness supplies Pebble events, storage, geolocation, network responses, message sends, and timers; Clay is stubbed. Tests cover startup SYNC, saved intervals, PKJS cadence timers, PING-gated acquisition, fresh climate fetches, location reuse, changed-field delivery, acknowledged caches, failures, timeouts, and late responses. Native C tests cover the watch's startup reply and retry decisions. Use the emulator or phone companion to validate actual AppMessage delivery.

## Operator Evidence Review Expectations

Report the following with a change:

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
- Clay display-mode changes and repaint behavior
- weather, battery, battery orientation, health, and display-mode behavior
- screenshot evidence for visual review
- manual config-page review
- manual hardware-install check when release confidence requires it
- known limitations or unchecked items

`pre-release-gate` is the canonical pre-release QA plan for representative runtime states. The operator also checks the settings page, restart behavior, and phone-driven weather/location flow.

```sh
./aag-build-qa.sh --exec pre-release-gate
```

## Validation of Watch Face Functionality

### 1. Using QA Scenarios, Matrices, and Suites

Out-of-the-box targets:

- `canary`: Two screenshot-enabled integrated `all` steps on Emery, covering both battery orientations and connected/disconnected states.
- `qa-smoke`: Matrix-driven six-surface integrated smoke coverage for Emery, Chalk, and Flint in white and black display modes.
- `visual-refresh`: Matrix-driven screenshot-producing visual checks across the same six surfaces, including charging, connectivity, day/night, sleet, and clear-weather states.
- `pre-push-gate`: Suite aggregation of the `visual-refresh` Matrix and `smoke-bluetooth` scenario.
- `pre-release-gate`: Matrix-driven representative pre-release validation across every supported emulator/display tuple, with screenshot evidence and manual signoff.

Use `aag-build-qa.sh` to validate and execute plans, then inspect or compare results.

```sh
./aag-build-qa.sh --exec canary
./aag-build-qa.sh --exec qa-smoke
./aag-build-qa.sh --exec pre-push-gate --force
./aag-build-qa.sh --exec pre-release-gate

./aag-build-qa.sh --runs
./aag-build-qa.sh --view <run-id-or-path>
./aag-build-qa.sh --compare <run-a> [run-b ...]
./aag-build-qa.sh --plans
./aag-build-qa.sh --validate <name-or-path of plan>
./aag-build-qa.sh --list-steps <name-or-path of plan>
```

Use `--dry-run` with a qa plan to inspect the resolved execution plan without creating a run:

```sh
./aag-build-qa.sh --exec pre-push-gate --dry-run
```

Use `--force` only after reviewing the resolved execution plan. If validation discards any plan items, the harness requires confirmation and ignores `--force`.

Runs write evidence under `qa/qa-runs/<run-id>/`. Inspect the canonical `report.json`, derived `summary.md`, `commands.log`, and requested screenshots. Bluetooth and `all` steps capture before and after the connection change. [QA README](../qa/README.md) owns artifact and inspection details.

### 2. Using the `pebble` emulator

Use manual emulator commands for focused diagnosis or when a qa plan would be too broad for the change under review.

#### Battery

```sh
pebble emu-battery --emulator emery --percent 19
pebble emu-battery --emulator emery --percent 75 --charging
```

#### Battery Orientation

Battery orientation uses AppMessage key `10013`. Exercise horizontal and vertical values on rectangular and round targets.

```sh
pebble send-app-message --emulator emery --int 10013=0
pebble send-app-message --emulator gabbro --int 10013=1
```

Confirm horizontal default on rectangular displays, vertical default on round displays, live relayout without watchface teardown, and persistence after restart. Apply battery percentages and charging state in both orientations.

#### Weather And Display Mode

These commands send test data directly to the watch using numeric message keys:

```sh
pebble send-app-message --emulator emery --int 10002=700 10003=95 10004=0
pebble send-app-message --emulator emery --int 10002=700 10003=95 10004=1
pebble send-app-message --emulator emery --int 10002=-32768 10003=-1 10004=0
pebble send-app-message --emulator emery --int 10005=15
pebble send-app-message --emulator emery --int 10006=0
pebble send-app-message --emulator emery --int 10006=2
```

Use [SettingsandConfiguration](SettingsandConfiguration.md) for the message-key catalog and transport contract.

Key `10005` changes the watch’s persisted interval only. Change the interval through Clay to exercise PKJS scheduling, then restart the phone companion to check restoration from saved Clay settings. Restart the watchface separately to check its persisted copy.

Check startup synchronization with the phone app disconnected: C must skip the outbox and count disconnected checks toward its six-attempt limit. Reconnect during retries and confirm the next attempt sends SYNC. Exhaustion ends that sync attempt.

After synchronization, disconnect before a scheduled PKJS check. Three failed PING sends must skip GPS and network acquisition, preserve visible data, and schedule another check after the selected interval. Reconnect and confirm acquisition resumes.

#### Weather And Location State

```sh
pebble send-app-message --emulator emery --int 10002=265 10003=0 10004=1 --string 10011=OAKLAND
pebble send-app-message --emulator emery --int 10006=1
pebble send-app-message --emulator emery --int 10001=1
pebble send-app-message --emulator emery --int 10002=265
pebble send-app-message --emulator emery --int 10002=0 10003=-1 10004=0 --string '10011='
```

Confirm that palette and unit changes preserve received weather and location. The combined unavailable payload must show unavailable weather and `---` for location. A partial weather packet must invalidate prior weather.

Manual messages exercise C ingress; use the phone companion to check coordinated acquisition, failure, timeout, and late-response handling. At unchanged coordinates, change weather and confirm all three climate fields are sent without the unchanged location. Unchanged results must produce no data message. Failed data delivery must not advance the caches; the next acquisition must be able to resend the changes. Move beyond the coordinate tolerance and confirm a new lookup and location send, even when the returned name is unchanged.

To verify the [refresh-order invariant](SettingsandConfiguration.md#weather-scheduling-and-delivery), delay acknowledgment of an unavailable payload and queue another refresh or cached replay. Confirm the next queued operation waits until that send succeeds or exhausts retries. Repeat with a valid-data send and a cached replay awaiting acknowledgment. Check that an older unavailable update cannot clear data from a later refresh and that the next cadence timeout is scheduled after the acquisition and send outcome settle.

#### One-Shot Health

One-shot health keys are QA-oriented runtime overrides. They affect the next health refresh only.

BPM overrides accept `1` through `220`; steps overrides accept `1` through `100000`. An out-of-range value clears the queued override and refreshes from HealthService. It does not force an unavailable reading.

```sh
pebble send-app-message --emulator emery --int 10020=72
pebble send-app-message --emulator emery --int 10021=8500
pebble send-app-message --emulator emery --int 10020=0
pebble send-app-message --emulator emery --int 10021=0
```

#### Refresh Interval Changes

Change the interval through Clay and confirm a connectivity probe starts immediately. After acquisition settles, confirm the next timeout uses the new interval. Save the same interval again and confirm cached weather and location are resent without replacing the active cadence timeout. Cancel configuration and confirm polling stays unchanged.

#### System Time Format

Select each 12/24-hour format in the watch’s system settings. Confirm the watchface follows that preference at startup and on a time refresh. In 12-hour mode, confirm a single-digit hour has no leading zero.

#### Config Page

```sh
pebble emu-app-config
```

Confirm Clay has no time-format control. Check that health controls follow capability filters and that PKJS omits health settings when its active-watch capability check fails.

### 3. Visually using Debug Builds

`ATAGLANCE_DEBUG` is an optional build-time gate for visual and runtime diagnosis.

Activation point:

```text
wscript
  -> ctx.env.append_value('DEFINES', ['ATAGLANCE_DEBUG=1'])
```

Use this path when layer bounds, glyph bounds, or debug-only runtime evidence would materially improve review. Re-comment the define before normal release builds unless the debug behavior is intentionally being inspected.

## Read Next

- [Native C tests](../tests/native/README.md) for focused runs, geometry diagnostics, debugging, and adding cases or suites.
- [QA README](../qa/README.md) for QA commands, plans, and artifacts.
