# Contributing

This document explains how to change At A Glance safely.

It is a workflow and review guide for contributors. It does not own product rules, visual vocabulary, or runtime architecture decisions.

## Required Reading

- `../README.md`
- `Build and Validation.md`
- `ProductInvariants.md`
- `VisualVocabulary.md`
- `UserInterface.md`
- `ArchitectureLedger.md`

## Scope

- Treat every change as small embedded firmware work.
- Design first. Audit live source, docs, generated files, and dirty tree before editing. Keep one coherent slice. Preserve existing user work.
- Cleanup, visual changes, AppMessage changes, platform work, and glyph tuning are separate unless explicitly approved together.

## C-Coding Decisions

- Use portable C within the Pebble SDK's constrained embedded runtime model.
- Format code with `clang-format`
- Use integer layout and drawing math only.
- Avoid heap allocation unless required.
- Match every acquired resource with a destroy path.
- Use fixed-size buffers and `snprintf`.
- Keep allocation, network, JSON, heavy formatting, and layout calculation out of layer update procs.
- Prefer capability guards.
- Do not pass or return large watchface or layout structs by value.
- Keep module APIs narrow.
- Prefer direct, auditable code.

## Build And Validation

For build, install, editor-tooling setup, `compile_commands.json`, harness
behavior, and validation flow, see `Build and Validation.md`.

Contributor rule:

- If a code change needs build or runtime verification, run the relevant build and validation flow from `Build and Validation.md` or explicitly report the gap.

## Install

### Emulator

Choose a target and install to it:

```sh
pebble install --emulator {emery,flint,chalk,gabbro}
```

To test the config page in an emulator:

```sh
pebble emu-app-config
```

### Hardware

```sh
pebble install --phone YOUR_PHONE_IP
```

`YOUR_PHONE_IP` is the Developer Connection Server IP shown by the Pebble mobile app.

Detailed install workflow, harness flow, and target-validation rules live in
`Build and Validation.md`.

## QA: Logs, Emulators, Debugging

### Logs

Use Pebble's built-in `APP_LOG` with restraint.

- `INFO` logs are temporary messages during feature development, not normal committed behavior.
- Use `APP_LOG_LEVEL_DEBUG` sparingly for temporary diagnostics.
- Use `APP_LOG_LEVEL_WARNING` and `APP_LOG_LEVEL_ERROR` for durable runtime diagnostics when they materially help diagnosis.
- Delete all `APP_LOG(APP_LOG_LEVEL_INFO, ...)` calls before committing code.

### Validating features using the Emulator

- Weather, display mode, and health overrides are sent through AppMessage.
- Battery state is changed through the emulator battery service.

#### Battery State Change: Emulator Messages

```sh
pebble emu-battery --emulator emery --percent 19
pebble emu-battery --emulator emery --percent 75 --charging
```

Use the harness or repeated emulator commands to validate multiple charge levels and charging status when checking bolt visibility and fill behavior.

#### Weather And Display Palette Change: AppMessages

Numeric message keys:

For weather data:
- `10002` = `TEMPERATURE`, for example `10002=539` sends `53.9C`
- `10003` = `WEATHER_CONDITION`, for example `10003=0` exercises a clear-weather glyph
- `10004` = `IS_DAY`, where `1` is day and `0` is night

For weather settings:
- `10005` = `WEATHER_UPDATE_MINUTES`
- supported values are `15`, `30`, `45`, and `60`

For display palettes:
- `10006` = `DISPLAY_MODE`
- supported modes are `0` to `3` on color targets and `0` to `1` on monochrome targets

Examples:

```sh
pebble send-app-message --emulator emery --int 10002=700 10003=95 10004=0 # night or day
pebble send-app-message --emulator emery --int 10002=700 10003=95 10004=1 # day
pebble send-app-message --emulator emery --int 10002=-32768 10003=-1 10004=0 # unavailable condition
pebble send-app-message --emulator emery --int 10005=15 # weather every 15 minutes
pebble send-app-message --emulator emery --int 10006=0 # Black on White palette
pebble send-app-message --emulator emery --int 10006=2 # Clear as Celeste palette on color targets; invalid on monochrome
```

QA notes:
1. Use the same weather code but change IS_DAY value when the goal is to inspect only the day/night glyph delta.
2. Pair display-mode changes with weather or battery checks when validating light/dark behavior.

#### One-shot Health validation: AppMessages

One-shot health ingress exists as one-shot overrides that affect the next health refresh only. Here's the flow:

- `watchface_runtime_boundary.c`
  - accepts one-shot health payloads and converts them into normal health refreshes
- `bpm.c`
  - supports a one-shot BPM override for the next health refresh only
  - falls back to the real HealthService value when no one-shot BPM packet is queued
- `steps.c`
  - supports a one-shot steps override for the next health refresh only
  - falls back to the real HealthService value when no one-shot steps packet is queued
- `watchface.c`
  - clears queued one-shot health state during destroy so stale values do not survive teardown

Current one-shot keys:
- `10020` = One-shot BPM
- `10021` = One-shot steps

Examples:

```sh
pebble send-app-message --emulator emery --int 10020=72 # BPM
pebble send-app-message --emulator emery --int 10021=8500 # Steps
pebble send-app-message --emulator emery --int 10020=0 # unavailable BPM icon
pebble send-app-message --emulator emery --int 10021=0 # unavailable Steps icon
```

### Message Validation Discipline [TODO review]

- Keep AppMessage examples synchronized with live `package.json` key order and current harness commands.
- Recheck manual numeric keys after any key add, remove, or reorder.
- Validate both transport and render behavior:
  - tuple parsed
  - runtime accepted the value
  - module refreshed
  - visible state matches the intended scenario
- When a test is battery-only, use `pebble emu-battery`; do not invent battery AppMessage keys.
- Prefer harness automation for broad sweeps and direct `pebble send-app-message` commands for focused debugging.

### Debugging

`ATAGLANCE_DEBUG` provides narrow, debug-gated rendering behavior that is off by default.

Recommended flow:

1. Enable `ATAGLANCE_DEBUG` in `wscript`.
2. Identify whether the module is driven by:
   - Pebble service callbacks
   - debug-only render instrumentation
3. Trigger only that module's path.
4. Validate the module's visible output.
5. Remove temporary logs or temporary debug drawing before commit.

To toggle debug builds:

1. Open `wscript`.
2. Uncomment or re-comment the line:
   ```py
   # ctx.env.append_value('DEFINES', ['ATAGLANCE_DEBUG=1'])
   ```
3. Rebuild:
   ```sh
   pebble clean && pebble build
   ```

#### Debug-Enabled Modules

Current `ATAGLANCE_DEBUG` behavior is narrow and module-specific:

- `substratum_renderer.c`
  - in debug builds, text layers render with a visible background/text-color inversion so frame and text bounds are obvious
- `climate_glyphs.c`
  - contains a debug-gated icon-bounds rectangle outline in the primary text color

## Review and Commit Discipline

Before committing a change:

- run `git diff --check`
- run `pebble build` for code changes or explicitly report the gap
- run **smoke** coverage across the areas touched by the change
- use emulator screenshots for visual or layout changes when practical
- review API, lifecycle, layout, and platform changes before coding
- update relevant documentation
- review the diff and stage only intended files
- delete temporary screenshots and buffers unless intentionally retained

## Documentation QA

Move content to canonical owners before rewriting it heavily.

Cross-references can expand understanding, but each doc should preserve enough detail that a reader can understand a section without having to inspect code unless they want implementation proof.

Update the documentation set when accepted architectural or product decisions change.
