# Current Handoff

## Project

- Workspace: `/Users/manomeht/Code/life-at-a-glance-emery-wf`
- Product: Pebble SDK watchface targeting Pebble Time 2 / `emery`
- Current focus: runtime fallback icon mode for testing icon fallbacks

## User Preferences

- Measure twice before editing or committing.
- Keep changes focused and approval-gated for meaningful code changes.
- Keep programming lines near 72 characters.
- Avoid expensive work in Pebble update procs.
- Remove or gate noisy logs before commit unless they are intentional test logs.

## Recent Commits

- `aef7fb6` refactored `main_window_load` into inline layer helpers and a
  shared text-layer initializer.
- `69eecf3` added compile-time BPM icon fallback mode.

## Current Uncommitted Work

Runtime fallback mode work is still uncommitted.

Changed files expected:

- `package.json`
- `src/pkjs/config.json`
- `src/c/ataglance.h`
- `src/c/main.c`

There may also be `pebble-log.log` in the repo root from emulator logging. Do
not commit it unless explicitly requested. Consider adding it to `.gitignore`
if log capture will be repeated.

## Current Runtime Behavior

Hidden AppMessage key:

- `BPM_ICON_MODE`
- Current generated key id: `10004`
- `0` means normal/resource icon mode.
- `1` means fallback mode.

Commands used during runtime testing:

```sh
pebble send-app-message --emulator emery --int 10004=0
pebble send-app-message --emulator emery --int 10004=1
```

Internal C state now uses generic fallback naming:

- `s_settings.fallback_mode`
- `FALLBACK_MODE_DISABLED`
- `FALLBACK_MODE_ENABLED`
- `FALLBACK_MODE`
- `FALLBACK_MODE_VALID(...)`

The hidden config key is still named `BPM_ICON_MODE`. This was kept to avoid
message-key churn during testing, but it may deserve discussion because the
mode is intended to become a generic fallback mode for all icons.

## Asset Lifecycle

Current intended model:

- Allocate each icon asset lazily.
- Keep both resource icon and fallback icon resident if both are created.
- Never allocate or unload assets inside the paint/update proc.
- Destroy both assets only in `main_window_unload()`.

Important functions in `src/c/main.c`:

- `load_bpm_icon_assets()`
- `unload_bpm_icon_assets()`
- `draw_bpm_icon_with_color()`
- `create_bpm_fallback_icon()`
- `apply_mode_visual_cue()`

## Visual Cue

Fallback mode changes the watchface background color:

- fallback enabled: `GColorDarkGray`
- fallback disabled: `GColorBlack`

This lets fallback mode be visually verified from launch.

## Logging

Fallback draw logging is throttled with a function-local static variable in
`draw_bpm_icon_with_color()`.

Known useful log:

```text
Fallback icon draw is active (mode=1, image=1)
```

That means:

- fallback mode is enabled
- the resource icon is also loaded
- draw path correctly chooses fallback because the runtime mode says so

Before committing, review log noise. Existing noisy logs such as update and
handler traces may need to be removed or gated.

## Validation Completed

`pebble build` passed after the current uncommitted changes.

Emulator testing confirmed:

- `10004=0` switched to normal mode.
- `10004=1` switched to fallback mode.
- Fallback drawing worked while resource icon remained loaded:

```text
Fallback icon draw is active (mode=1, image=1)
```

The emulator and log capture were stopped with:

```sh
pebble kill --force
```

## Commands

Build:

```sh
pebble build
```

Install:

```sh
pebble install --emulator emery
```

Logs:

```sh
pebble logs --emulator emery
```

Write logs to a file:

```sh
pebble logs --emulator emery | tee pebble-log.log
```

Stop emulator:

```sh
pebble kill --force
```

## Before Commit

Recommended next-session checklist:

1. Run `git status --short`.
2. Review `git diff`.
3. Decide whether hidden key should remain `BPM_ICON_MODE` or become
   `FALLBACK_MODE`.
4. Check 72-character line width in changed C code.
5. Remove or gate noisy logs that are not needed after testing.
6. Ensure `pebble-log.log` is not committed.
7. Run `pebble build`.
8. Stage and commit only the intended fallback runtime changes.
