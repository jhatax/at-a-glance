# Current Handoff

## Project State

- Workspace: `/Users/manomeht/Code/life-at-a-glance-emery-wf`
- Product: Pebble SDK watchface, "Life at a Glance".
- Date: June 8, 2026.
- Branch/head at handoff: `main` at
  `d323873 Continue modular watchface ownership cleanup`.
- Working tree at handoff creation: clean before this document update.
- Current target platforms in `package.json`: `aplite`, `basalt`,
  `diorite`, `emery`, and `flint`.
- Last known validation before `d323873`: `git diff --check` passed and
  `pebble build` passed.

Latest relevant commits:

- `d323873` `Continue modular watchface ownership cleanup`
- `5e7f3f1` `Document round text-first layout direction`
- `3ab3229` `Continue code clean-up: enforce text-first creation`
- `4d9ff41` `Continue code clean-up: harden runtime inputs`
- `beab326` `Rename time display module to time`

## Required Reading

Before editing, read these in order:

1. `/Users/manomeht/Code/AGENTS.md`
2. `/Users/manomeht/Code/agent-templates/AGENTS-PEBBLE.md`
3. `AGENTS.md`
4. `current-handoff.md`

The inherited files are intentional. If they cannot be read, say so before
making changes.

## Collaboration Contract

Use the stricter loop before writing code:

1. Inspect current symbols and call paths.
2. Identify existing vocabulary, ownership, platform guards, and state.
3. State the intended patch shape before editing.
4. Edit only after terms and branches line up.
5. Build and review the diff against the stated intent.

Additional project expectations:

- Push back when a proposed change fights Pebble lifecycle, module ownership,
  product semantics, or repository hygiene.
- Reuse existing sentinels, macros, enum names, and product vocabulary.
- Avoid redundant state, adjacent constants, and one-line helper functions.
- Audit every include and caller when changing platform-bound headers,
  especially `bpm.h`, `steps.h`, and `Health*` symbols behind
  `PBL_HEALTH`.
- Do not ignore new refactor source files. Ignore local build/editor artifacts
  only.

## Current Architecture

`main.c` is now intentionally slimmer. It owns:

- app/window lifecycle setup and teardown
- service subscription and unsubscription
- AppMessage callback entry points and dispatch
- settings load/save orchestration
- Pebble app loop entry

`watchface_composer.c` owns screen composition:

- active window and settings references
- layout calculation
- root layer composition
- required and optional module creation
- refresh and tick routing
- temperature and weather routing
- failure exit behavior when required controls cannot initialize

Module ownership after the latest refactor:

- `date.c`: date text layer, buffer, creation, update, destroy.
- `time.c`: time text layer, buffer, creation, tick update, destroy.
- `battery.c`: battery text/icon layers, state, colors, callback update, and
  rendering.
- `bpm.c`: BPM text/icon layers, BPM state, colors, health event update,
  and rendering. Health API use remains behind `PBL_HEALTH`.
- `steps.c`: steps text/icon layers, steps availability, colors, health
  event update, and rendering. Health API use remains behind `PBL_HEALTH`.
- `climate.c`: temperature text, weather icon layer, source state,
  formatting, validation, update orchestration, and layer ownership.
- `climate_glyphs.c`: procedural weather glyph drawing only. It receives
  condition and palette explicitly; it must not reach back into climate
  state.
- `layout.c`: `WatchfaceSurface` calculation, palette selection,
  typography resolution, and shared text-layer update behavior. It should
  not create Pebble layers.
- `settings.c`: persisted user settings defaults, validation, load, save, and
  HR sampling interval mapping.
- `helper.c/.h`: shared helpers such as strict tuple parsing and scaling.
- `ataglance.h`: product-level constants and decisions.

## Creation Semantics

Required controls:

- date text
- time text
- battery text

Optional controls:

- weather text/icon
- health text/icon controls
- all icon layers
- color richness beyond black-and-white fallback
- live phone/weather/health data

Product rule:

- Text is the primary glance surface.
- Icons are secondary.
- A text-only metric is acceptable.
- An icon-only metric is not useful.
- If a metric's text layer cannot be created, do not create or depend on that
  metric's icon.
- If an icon layer cannot be created, keep updating the corresponding text.
- Log real creation failures once where creation happens. Avoid refresh-time
  noise for known optional missing layers.

Health-specific rule:

- Health creation succeeds if either BPM text or steps text exists.
- This replaced the accidental all-or-nothing behavior.

## Pebble Lifecycle Decision

Do not fight Pebble lifecycle.

The accepted model is:

1. `init()` creates the window and installs handlers.
2. `window_stack_push()` lets Pebble call the window load handler.
3. The load handler creates the watchface through `watchface_composer_create()`.
4. If required controls cannot be created, the composer destroys partial state
   and pops the window stack.
5. With no useful window left, `app_event_loop()` returns.
6. `deinit()` unwinds services and destroys the window safely.

Do not reintroduce a post-`window_stack_push()` synchronous success flag or a
separate lifecycle control flag unless Pebble docs and code paths prove it is
needed.

## Weather Contract

There is intentionally no offline weather cache and no weather timestamp model.

PebbleKit JS sends:

- `TEMPERATURE`: Celsius tenths as a raw integer.
- `WEATHER_CONDITION`: raw Open-Meteo weather code.

Unavailable weather uses existing vocabulary:

- `WEATHER_TEMP_INVALID`
- `WEATHER_CONDITION_UNKNOWN`

C validates incoming untrusted data at the receiving boundary. JS should avoid
drift by sending the documented sentinel values when fetch/parse fails.

Weather glyph drawing is split out so climate state and procedural glyph
rendering stay independently auditable. Keep the split clean:

- `climate.c` owns state and layer/update orchestration.
- `climate_glyphs.c` maps/draws glyphs from explicit render inputs.
- `draw_climate_icon()` should receive condition and palette explicitly.

## Palette And Legibility

The fallback visual decision is black-and-white display, not blocking the
watchface.

Current boundary:

- `layout_update_surface_style()` owns palette selection.
- palette storage in `layout.c` should remain file-local unless there is a
  clear external contract.
- renderers that already have palette/background context should use Pebble's
  legibility helpers directly, such as `gcolor_legible_over()`.
- low-level display code should not reach into `main.c` or composer state.

## AppMessage And Settings

Current AppMessage decisions:

- Watch does not need a meaningful outbound payload for product behavior.
- Inbox/outbox sizes were reduced from maximum sizes toward current payload
  needs.
- `app_message_open()` result is checked.
- setting tuple parsing moved to shared strict helper logic.

Persisted settings are user preferences only:

- time format
- temperature unit
- HR sample interval
- display mode

Do not reintroduce persisted temperature or weather snapshot state unless the
product decision changes.

## Remaining Work

### 1. Style Drift Cleanup

The latest broad refactor intentionally prioritized boundaries. A focused style
pass is still warranted.

Check for:

- source lines materially over the new 80-character guidance
- one-line function declarations or definitions where existing style expects
  multi-line formatting
- closing parenthesis/brace placement on multi-line calls, definitions,
  declarations, and returns
- comments that merely restate code
- no-argument C functions that should be declared as `(void)` if that is the
  prevailing pattern

Keep this as a style-only commit if possible.

### 2. Continue Main Ownership Reduction

`main.c` is smaller, but keep pushing it toward lifecycle and dispatch only.

Candidate audits:

- AppMessage receive flow: identify what can move into owner modules without
  hiding message contracts.
- Settings application: keep persistence in `settings.c`, but avoid making
  `main.c` own module-specific behavior.
- Service callbacks: dispatch to modules/composer; do not let `main.c` regain
  formatting or render state.

Do not move behavior merely for symmetry. Preserve behavior and use clear owner
contracts.

### 3. Round Watchface Planning

Round support is still planning-only.

Do not add `chalk` or `gabbro` until the plan accounts for:

- circular clipping at each row's y-position
- row-specific available widths
- date/time extremes
- health and bottom-row differences
- icon legibility near curved edges
- color and monochrome variants
- platform capabilities and missing sensors

Use `180x180` as the hard base target first, then scale toward larger round
screens such as `gabbro`.

### 4. README And Docs Drift

`AGENTS.md` was updated with the latest ownership lessons. README may still
lag behind implementation details.

Audit docs against:

- `src/c/main.c`
- `src/c/ataglance.h`
- `src/modules/*.h`
- `src/pkjs/config.json`
- `package.json`

Pay attention to stale references to layout/rule behavior, glyph semantics,
weather persistence, and module ownership.

### 5. Publish Prep

Before first private GitHub publication:

- run `pebble build`
- run a secret/private-data audit
- review `.gitignore`
- review SDK/platform metadata
- review README
- create screenshots only if useful and approved
- make a clean initial private GitHub commit when requested

## Validation Checklist

Before any commit:

1. Run `git status --short`.
2. Inspect staged and unstaged diffs.
3. Run `git diff --check`.
4. Run `pebble build` for code changes.
5. Confirm untracked files are intentional source/docs, not screenshots,
   scratch buffers, compile databases, or build artifacts.
6. Use a commit message with a concise subject and a body that captures broad
   behavior/ownership impact for non-trivial changes.
