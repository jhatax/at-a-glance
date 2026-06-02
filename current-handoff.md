# Current Handoff

## Project State

- Workspace: `/Users/manomeht/Code/life-at-a-glance-emery-wf`
- Product: Pebble SDK watchface, currently targeting Pebble Time 2 / `emery`.
- Date: June 2, 2026.
- Last verified build: `pebble build` passed after commit `9fa6850`.
- Working tree at handoff: docs-only changes are pending and not
  committed yet: `AGENTS.md`, `current-handoff.md`, and
  `NEXT_SESSION_REFINEMENTS.md`.

## Current Commit Stack

Recent relevant commits:

- `9fa6850` Rename watchface display refresh path
- `f6cb94b` Clarify visual palette application flow
- `57de6f9` Wire rendering to display palettes
- `c63ce78` Define display color palettes
- `6d0fa4e` Make icon fallback mode global
- `2d9e015` docs: record commit message preference

## Current Runtime Model

Settings and AppMessage keys:

- `TIME_FORMAT`: `0` 24-hour, `1` 12-hour
- `TEMP_UNIT`: `0` Fahrenheit, `1` Celsius
- `TEMPERATURE`: Celsius tenths from PebbleKit JS
- `HR_SAMPLE_MINUTES`: enum-backed sampling interval
- `ICON_FALLBACK_MODE`: hidden, `0` disabled, `1` enabled
- `DISPLAY_MODE`: `0` dark, `1` light

Generated key ids after clean build:

- `ICON_FALLBACK_MODE` is `10004`
- `DISPLAY_MODE` is `10005`

Useful commands:

```sh
pebble send-app-message --emulator emery --int 10004=0
pebble send-app-message --emulator emery --int 10004=1
pebble send-app-message --emulator emery --int 10005=0
pebble send-app-message --emulator emery --int 10005=1
```

## Visual Palette State

Palette definitions live in `src/c/main.c` as `VisualPalette`.

Dark mode:

- Background: `GColorBlack`
- Primary text: `GColorLightGray`
- Unavailable text: `GColorWindsorTan`
- Date: `GColorRichBrilliantLavender`
- Time: `GColorSunsetOrange`
- Rule: `GColorLightGray`
- Steps icon: `GColorChromeYellow`

Light mode:

- Background: `GColorWhite`
- Primary text: `GColorBlack`
- Unavailable text: `GColorLightGray`
- Date: `GColorImperialPurple`
- Time: `GColorSunsetOrange`
- Rule: `GColorLightGray`
- Steps icon: `GColorChromeYellow`

Semantic colors remain mode-independent:

- BPM `1-99`: `GColorJaegerGreen`
- BPM `100-120`: `GColorMagenta`
- BPM `>120`: `GColorRed`
- Battery charging: `GColorJaegerGreen`
- Battery `>50`: `GColorCobaltBlue`
- Battery `21-50`: `GColorYellow`
- Battery `<=20`: `GColorRed`

## Refresh Flow

Current display refresh entry point:

- `refresh_watchface_display()`

It currently:

- Requires `s_window`, logs error and returns if missing.
- Sets the window background from `s_palette`.
- Marks background/rule dirty.
- Marks steps icon dirty because the icon reads `s_palette->steps_icon`.
- Refreshes date, time, steps, BPM, battery, and temp.

Palette selection:

- `settings_load()` validates settings and calls `select_visual_palette()`.
- Display-mode AppMessage updates `s_settings.display_mode`, calls
  `select_visual_palette()`, then `refresh_watchface_display()`.

Important audit rule for tomorrow:

- Before changing refresh or draw behavior, run `rg "s_palette|update_proc|layer_mark_dirty|text_layer_set_text_color" src/c/main.c`.
- Identify every layer and text path affected before editing.

## Known Follow-Ups

See `NEXT_SESSION_REFINEMENTS.md` for the tomorrow backlog.

Highest-value next items:

1. Reconcile `README.md` and `AGENTS.md` with current implementation.
2. Audit `refresh_watchface_display()` end-to-end for all layers and callbacks.
3. Begin a behavior-equivalent layout-state extraction for future form factors.

## Collaboration Runbook

Use this cycle for substantive work:

1. Inspect relevant code paths and repo state.
2. Identify affected state, layers, callbacks, config, docs, and generated keys.
3. Audit all dependent draw/update/refresh paths before suggesting a patch.
4. Confirm intended behavior and failure modes with the user when needed.
5. Execute the smallest coherent patch.
6. Validate with diff review, build, and emulator/log checks when applicable.

Specific lesson from today:

- UI refresh changes require a complete layer audit. Do not assume text updates
  imply icon layers will repaint. Every `Layer` update proc that reads changed
  state must be explicitly considered and dirtied when appropriate.
