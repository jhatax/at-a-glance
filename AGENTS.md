# Agent Instructions

This repo is for Pebble SDK 4+ watchfaces and apps. The current rectangular
targets are `aplite`, `basalt`, `diorite`, `emery`, and `flint`; round
platforms still require a separate layout plan before support is added.

## Working Style

- Spend most effort on architecture, design, testability, constraints, and look-and-feel before coding.
- Aim before acting: understand the goal, constraints, platform target, and likely failure modes before changing files.
- Measure twice, cut once: verify assumptions and inspect diffs before commits, publishing steps, or broad refactors.
- Think like an architect before editing. Slow down, identify the real
  boundary, and resist broad helper extraction that only hides simple
  arithmetic or spreads context across files.
- Review your own code with a code-review mindset before and after patches:
  prioritize bugs, missing state updates, declaration-definition drift,
  platform regressions, and missing validation.
- Use an inspect, identify, audit, confirm, execute, validate cycle for
  substantive changes. Before proposing or implementing changes, inspect the
  relevant code paths, identify affected state and layers, audit all
  dependents, confirm the intended behavior, execute the smallest coherent
  patch, and validate with build/runtime checks.
- Use progressive disclosure: discover only the files needed, identify entry points, suggest changes, then edit.
- Do not guess Pebble behavior. Verify SDK 4+ APIs, generated resources, AppMessage keys, emulator behavior, and build output when they matter.
- Keep changes focused. Suggest cleanups and stylistic changes before making them unless they are required for the task.
- Keep slices small: audit one coherent change, patch it, inspect the diff,
  build when code changed, commit it, then move to the next slice.
- After meaningful work, critique the result: call out misses, uncertain assumptions, platform risks, verification gaps, and what could get dropped in later iterations.

## Project Facts

- Native C: `src/c`
- PebbleKit JS and Clay config: `src/pkjs`
- Metadata, target platforms, resources, capabilities, and message keys: `package.json`
- Build entry: `wscript`
- Current design reference: `DESIGN.md`
- Current product: "Life at a Glance", a Swiss-Rail-inspired glanceable watchface.

## Current Watchface Implementation Snapshot

Keep this section aligned with `src/c/main.c`, `src/c/ataglance.h`, `src/pkjs/config.json`, and `package.json`.

- Target platforms: `aplite`, `basalt`, `diorite`, `emery`, and `flint`.
  Round platforms such as `chalk` and `gabbro` still need a separate layout
  plan before support is added.
- Pebble manifest capabilities currently required:
  - `configurable` (Clay settings page)
  - `location` (weather via geolocation)
  - `health` (BPM + steps)
- AppMessage keys currently required:
  - `TIME_FORMAT`, `TEMP_UNIT`, `TEMPERATURE`, `WEATHER_CONDITION`
  - `HR_SAMPLE_MINUTES`, `DISPLAY_MODE`
- Persisted settings defaults:
  - time format: `24h`
  - temperature unit: `°F`
  - HR sampling: `10` minutes
  - display mode: dark

Rectangular `PBL_RECT` layout geometry (current frames/anchors on Emery):

- derived spacing: `PBL_DISPLAY_HEIGHT / 28`, currently `8`
- date: `GRect(8, 8, 184, 20)` (`FONT_KEY_GOTHIC_18_BOLD`,
  `GColorRichBrilliantLavender`)
- time: `GRect(8, 58, 184, 48)`
  (`FONT_KEY_BITHAM_42_BOLD`, `GColorSunsetOrange`)
- rule: line from `(8, 114)` to `(192, 114)`
- heart icon: `GRect(8, 118, 28, 28)` (procedural heart)
- bpm text: `GRect(38, 122, 28, 20)` (`FONT_KEY_GOTHIC_18`)
- steps icon: `GRect(122, 118, 28, 28)` (custom drawn paw glyph)
- steps text: `GRect(152, 122, 40, 20)` (`FONT_KEY_GOTHIC_18`)
- weather icon: `GRect(8, 196, 28, 28)` (procedural weather glyph)
- temp text: `GRect(38, 200, 40, 20)` (`FONT_KEY_GOTHIC_18`)
- battery icon: `GRect(128, 196, 28, 28)` (custom horizontal battery)
- battery text: `GRect(158, 200, 34, 20)`
  (`FONT_KEY_GOTHIC_18_BOLD`)

Color/semantic rules that must remain documented when changed:

- unavailable text token is `---`.
- dark mode uses black background, light gray primary text, Windsor Tan
  unavailable text on color displays, Rich Brilliant Lavender date,
  Sunset Orange time, Light Gray rule, and Chrome Yellow steps icon.
- light mode uses white background, black primary text, Light Gray
  unavailable text on color displays, Imperial Purple date, Sunset Orange
  time, Light Gray rule, and Chrome Yellow steps icon.
- black-and-white displays use inverted unavailable backgrounds so missing
  values remain legible without color.
- unavailable icon backgrounds should match unavailable text backgrounds for
  symmetry unless an explicit product decision says otherwise.
- BPM color zones:
  - `<=0` or unavailable: current mode unavailable color
  - `1-99`: Jaeger Green
  - `100-120`: Magenta
  - `>120`: Red
- battery text/icon colors:
  - charging: Jaeger Green
  - not charging and `>50`: Cobalt Blue
  - not charging and `21-50`: Yellow
  - not charging and `<=20`: Red
- weather state:
  - PebbleKit JS sends raw Open-Meteo `weather_code`
  - C maps codes into private weather glyph buckets in
    `src/modules/weather.c`
  - procedural glyphs are used instead of Carbon/IcoMoon or PDC weather
    assets for now

Clay configuration UI (must stay in sync across Clay, JS, C, docs):

```text
At A Glance: Configuration
  - Time format: 24-hour, 12-hour
  - Temperature unit: Fahrenheit, Celsius
  - Display mode: Dark mode, Light mode
  - HR Sampling Frequency: 10, 15, 30, 60, 120 minutes
  - Submit: Save Settings
```

Visual baseline from emulator snapshot (June 1, 2026, 14:50 sample):

- date and hero time are fitting at sampled values (`MON · 01 JUN`, `14:50`).
- bottom row sample (`90°F`, `80%`) fit still needs visual
  revalidation after the rectangular spacing update.
- unavailable health values render as `---`.
- current implementation does not draw a vertical rail; it uses a
  horizontal rule at `y=114`.

## Pebble Platform Rules

- Use only Pebble SDK 4+ APIs. If an API is platform-specific, guard it and document the fallback.
- Treat Pebble as an embedded environment: small memory, limited CPU, limited battery, small display, and platform-specific capabilities.
- Keep rendering callbacks lightweight. Do not allocate, fetch, parse JSON, or do expensive formatting in layer update procs.
- Treat health, heart rate, color, screen shape, resources, and phone data as optional by platform unless verified.
- Before broadening platform support, maintain a small platform matrix: bounds, shape, color capability, sensors, resources, and SDK constraints.
- Separate platform geometry from product logic. Prefer named constants and values derived from layer bounds over magic numbers.
- Avoid passing large structs by value in C and C++ paths added to this
  project. Prefer scalar fields or pointers to caller-owned storage when the
  function needs to fill or inspect larger state.

## Coding Style

- Align naming, formatting, and structure with Pebble C conventions. This is important.
- Stick to standard C supported by the Pebble toolchain. Avoid unsupported extensions and desktop-only assumptions.
- Use spaces, not tabs. Remove trailing spaces.
- Keep source lines at 72 characters max across all programming files.
- Use braces and new lines for all `if`, `else`, `switch`, `case`, `for`, `while`, and similar blocks, even for one statement.
- Keep variables in the narrowest practical local scope.
- Prefer readability over unnecessary optimization.
- Document succinctly: explain intent, contracts, non-obvious platform constraints, and manual test notes. Avoid comments that restate the line of code.
- Preserve fixed-size buffer discipline. Use `snprintf`; do not introduce unsafe string handling.
- Avoid heap allocation unless the Pebble API requires it. Pair allocations with clear destroy paths.
- Use `APP_LOG` sparingly. Remove or gate noisy logs before commit.

## JavaScript Bridge

- PebbleKit JS must remain compatible with the Pebble/Rebble JS runtime.
- Write readable JS with clean argument passing and semicolons.
- Use braces and new lines for all conditionals, loops, and switch cases.
- Keep AppMessage payloads compact, typed, and validated on both JS and C sides.
- Weather payloads should stay numeric. Prefer passing raw or enum-like
  integer weather codes over random strings.
- AppMessage changes must update `package.json`, Clay config, JS send/receive code, C tuple handling, defaults, and docs together.
- Handle network errors, timeouts, malformed JSON, missing fields, and fallback data explicitly.

## Build Discipline

- Build with the strictest settings available for the Pebble SDK/toolchain. Do not add flags that the SDK cannot support.
- Use this command for normal builds:

```sh
pebble build
```

- If the build fails, stop and report the failure before moving toward commit or publish.

## Run And Debug

- Test with the Pebble emulator for visual or runtime changes.
- Confirm install command:

```sh
pebble install --emulator emery
```

- Keep logs visible in a shell session during emulator debugging:

```sh
pebble logs --emulator emery
```

- Use logs to debug real behavior, then remove noisy diagnostic logging before commit.

## Design And Docs

- Optimize for a one-second glance on a small watch display.
- Preserve hierarchy: date, hero time, rule, complications, bottom metrics.
- Validate visual changes against actual screen bounds, not only desktop intuition.
- Text must not clip at likely extremes such as `WED · 30 SEP`, `12:59`, `100`, `99999`, `---F`, and `100%`.
- Document entry points and user-facing behavior in `README.md`.
- Keep `README.md` simple, direct, and Markdown-native. Include screenshots when visual behavior changes.
- Avoid AI-generated slop: vague filler, overexplaining, contradiction phrases, double-dash constructions, and inflated claims.

## GitHub Publishing

- Everything here should be treated as eventually publishable to GitHub.
- New projects start in a private GitHub repo unless the user says otherwise.
- Never commit secrets, tokens, API keys, credentials, private device addresses, personal location data, signing artifacts, or machine-specific config.
- Before committing, inspect diffs, untracked files, ignored files, generated output, and docs.
- When a commit includes new functions or non-trivial behavior, use a detailed
  commit message body with a bulleted list instead of prose.
- If something will not build, is wrong for GitHub, contains private data, has broken metadata, or is misleading, tell the user and fix it before commit.
- Do not publish generated build artifacts unless intentionally releasing them and the user agrees.
- This folder is not yet on GitHub. The next repo task should prep the initial private GitHub commit: build check, secret audit, `.gitignore`, SDK/platform metadata, README, screenshots if available, and a clear initial commit.

## Final Checklist

- Current target platform set is explicit.
- SDK 4+ API availability is verified or uncertainty is stated.
- Platform-specific behavior has a fallback.
- Resource filenames and generated IDs match.
- AppMessage keys match across package metadata, Clay, JS, and C.
- Defaults match across C, Clay, README, and persisted settings.
- `pebble build` was run, or the reason it could not run is reported.
- Emulator install/log verification was run for runtime or visual changes, or the gap is reported.
