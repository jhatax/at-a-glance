# Architecture Ledger

This ledger is the starting contract for future At A Glance watchface work.
Use it at the start of each coding conversation, update it when decisions
change, and audit source against it before claiming a slice is complete.

## Working Cycle

Every non-trivial change follows this cycle:

1. Audit first.
   Inspect current symbols, call sites, includes, ownership, platform guards,
   state, and docs before proposing code.
2. Measure twice.
   Identify ramifications, similar patterns elsewhere, and any public API or
   header impact. If fixing a broken pattern, search for other occurrences and
   address the pattern coherently.
3. State patch shape.
   Name files to add/edit/remove, functions to move/change, and behavior that
   must not change.
4. Review before editing.
   For architecture-sensitive changes, pause after the proposed patch shape
   and wait for agreement.
5. Edit narrowly.
   Keep the slice coherent but local. Do not mix refactors, visual changes,
   AppMessage changes, and platform enablement unless explicitly approved.
6. Validate.
   Run `git diff --check`; run `pebble build` for code changes unless the user
   explicitly pauses validation. Use emulator screenshots for visual slices.
7. Reconcile.
   Confirm each invariant and acceptance check as implemented, violated,
   intentionally deferred, or not applicable.
8. Commit cleanly.
   Stage only intended files. Do not commit build artifacts, screenshots, or
   unrelated scratch files.

## Development Guardrails

- Mature codebase rule: do not overreach. Prefer local, deliberate changes.
- Do not create helper functions when the real logic is clearer inline,
  especially for fewer than roughly 10 lines of straightforward code.
- Public APIs are declared in headers and implemented as external functions.
  Public APIs must not be `static` or `inline`.
- File-local private helpers may be `static`.
- Avoid header pollution. Keep public headers limited to concepts callers
  legitimately need.
- If a value only exists to support one `.c` file, make it file-local.
- If a concept is internal to watchface, do not expose it to `main.c`.
- If a concept is internal to layout, do not expose it to feature modules.
- Do not move Pebble layer ownership out of feature modules.
- Do not introduce generic dynamic strata arrays; strata are fixed and known.
- Do not pass or return large watchface structs by value.
- Do not change AppMessage keys, platform support, visual geometry, palette,
  or glyph behavior inside unrelated refactor slices.
- Preserve user changes. Never revert unrelated dirty work.

## Current Architecture Decisions

### Product Constants

Decision: `src/c/ataglance.h` owns product constants and compile-time product
decisions.

Invariants:

- It may define debug flags, design dimensions, sizing constants, font keys,
  string limits, and persisted-settings toggles.
- It must not own Pebble layer helpers, module lifecycle APIs, or runtime
  watchface orchestration.

Acceptance checks:

- `DEBUG_ATAGLANCE` is defined here when local debug support is enabled.
- Module APIs are not declared here.

### Component Model

Decision: `src/modules/watchface_components.h` owns shared display component
types.

Invariants:

- It defines `ColorPalette`, font/color roles, substrata, strata,
  `WatchfaceSurfaceStyle`, and `WatchfaceSurface`.
- It is type-focused, not a dumping ground for settings, renderer, layout, or
  module lifecycle APIs.
- `WatchfaceSurface` carries final substratum frames, not intermediate layout
  math such as row gaps or content widths.

Acceptance checks:

- No layout-private metrics are exposed in `WatchfaceSurface`.
- No renderer functions are declared in `watchface_components.h`.

### Layout Facade

Decision: `src/modules/layout.c/.h` is the public layout facade.

Invariants:

- `layout.h` exposes only calculated-surface APIs:
  `layout_calculate_surface()` and `layout_update_surface_style()`.
- Only layout implementation files include private layout helpers such as
  `layout_rect.h` and `layout_stylist.h`.
- Layout calculates data; it does not create Pebble layers.

Acceptance checks:

- Feature modules include `layout.h` only if they need layout APIs; otherwise
  they use `watchface_components.h` and/or `substratum_renderer.h`.
- `layout.h` does not expose text-layer creation, icon scaling, or color-role
  lookup.

### Layout Stylist

Decision: `src/modules/layout_stylist.c/.h` privately owns visual style
resolution for layout.

Invariants:

- Palette selection, compact/full display classification, font-role to system
  font mapping, and custom font resource decisions happen here.
- Modules may load/unload custom fonts as lifecycle owners, but they do not
  decide which font resource should be used.
- Compact classification is based on face dimensions:
  rectangular compact below `200x228`, round compact below `260x260`.

Acceptance checks:

- Time module reads `surface->style.custom_font_resource_ids`; it does not
  decide resource IDs from face dimensions.
- Current implemented palette is the Shinkansen palette documented in
  `palette-options.md`.

### Layout Architect

Decision: `src/modules/layout_rect.c/.h` privately owns rectangular geometry.
Future `layout_round.c/.h` will privately own round geometry.

Invariants:

- Architects assign final `x`, `y`, `w`, and `h` to each substratum.
- Architects may use file-local metric structs to improve readability.
- Layout is not a generic row engine; product strata remain fixed and known.
- Round support must not be enabled until a real round architect exists.

Acceptance checks:

- Rectangle geometry does not leak private metrics into
  `WatchfaceSurface`.
- No round platform support is added in a rectangle-only slice.

### Substratum Renderer

Decision: `src/modules/substratum_renderer.c/.h` owns common Pebble rendering
helpers for calculated substrata.

Invariants:

- It may create text layers, create icon layers, update text layers, resolve
  static color roles, and scale design icon coordinates.
- It does not own module source state, text formatting, dynamic colors, icon
  update procs, service data, AppMessage data, or glyph behavior decisions.
- Icon update procs are passed into icon creation by the owning module.

Acceptance checks:

- Feature modules call renderer helpers for common TextLayer/Icon layer setup.
- Dynamic BPM and battery colors remain module-owned.

### Watchface Runtime

Decision: `src/modules/watchface.c/.h` is the runtime clearing house.

Invariants:

- `watchface` owns the live `WatchfaceSurface`.
- `watchface` explicitly creates modules and tracks successful creation with
  `s_strata_created_mask`.
- `watchface` destroys only strata that were actually created.
- `watchface_refresh(WatchfaceUpdateMask updates)` is the only public render
  dispatcher.
- Display-mode handling is inline in the `WATCHFACE_UPDATE_DISPLAY_MODE`
  branch of `watchface_refresh()`.
- The strata-only redraw mask is private to `watchface.c`; `main.c` must not
  know about strata.
- Setters mutate source state only. Rendering happens through a later refresh.

Acceptance checks:

- `main.c` includes `watchface.h`, not feature module headers.
- `main.c` calls watchface APIs only.
- No `watchface_update_style`, `watchface_update_palette`, or
  `watchface_update_display_mode` helper exists.
- `WATCHFACE_UPDATE_ALL_STRATA` is not exposed in `watchface.h`.

### Main Runtime

Decision: `src/c/main.c` owns Pebble lifecycle, services, settings, and
AppMessage parsing.

Invariants:

- `main.c` knows update categories, not feature modules.
- AppMessage handling accumulates a local `WatchfaceUpdateMask` and makes one
  final `watchface_refresh()` call for coalesced updates.
- `main.c` may call watchface source-state setters for temperature, weather
  condition, and debug health values.

Acceptance checks:

- No direct calls from `main.c` to battery, climate, BPM, steps, date, or time
  modules.
- No feature-module headers included by `main.c`.

### Feature Modules

Decision: Feature modules own their own Pebble layer lifecycle.

Invariants:

- Modules create their own text/icon layers from calculated substrata.
- Modules own buffers, source state, update procs, refresh behavior, and
  destroy paths.
- Required text-layer creation determines module success for text-bearing
  modules.
- Optional icon creation may fail without making the module fail unless the
  module explicitly defines it as required.
- Modules assign their static `s_surface` only after required layer creation
  succeeds.

Acceptance checks:

- `*_module_create()` returns true only after required layers are created.
- Destroy paths clean up every layer/font the module owns.
- Watchface records creation success only from module create return values.

### Climate And Health Boundaries

Decision: Weather is now the `climate` module; health is split into `bpm` and
`steps`.

Invariants:

- AppMessage key names may remain weather-oriented for compatibility.
- `main.c` does not know climate/BPM/steps modules exist.
- Debug BPM/steps declarations and definitions are gated by both
  `DEBUG_ATAGLANCE` and `PBL_HEALTH`.
- Debug BPM/steps injections are one-shot when consumed by refresh and can be
  explicitly cleared.
- A zero step count is valid when Pebble Health reports step data as
  accessible. In current emulator runs, `0` steps can appear even when no
  useful health data is present; this is treated as an emulator-data quirk, not
  a product bug to special-case in watchface code.

Acceptance checks:

- No `weather.c/.h`, `health.c/.h`, or old display module remains tracked.
- PebbleKit JS comments reference `climate.h`, not `weather.h`.
- Do not infer unavailable steps from `0`; unavailable steps are driven by
  health-service accessibility and invalid readings.

### Background Layer

Decision: The background/rule is its own module-owned layer.

Invariants:

- Background owns its layer and update proc.
- Background color and line color come from `ColorPalette`.
- The rule is part of background substratum data, not ad hoc watchface drawing.

Acceptance checks:

- `watchface.c` does not draw the rule directly.
- `background_module_refresh()` is used when palette/display mode changes.

### Visual Redesign

Decision: The rectangle/round visual redesign is documented but not yet
implemented.

Invariants:

- Do not mix visual redesign with architecture cleanup.
- Rectangle visual redesign is a future visual slice.
- Round visual redesign is a separate future visual slice after rectangle
  redesign is validated.
- Round support requires `layout_round.c/.h` and screenshot review before
  enabling round platforms.

Acceptance checks:

- Current source does not enable Chalk/Gabbro.
- `layout-architect-role-flow.md` documents the proposed visual slices.

## Reconciliation Checklist

Before marking a task complete, answer these explicitly:

- What decisions from this ledger apply?
- Which invariants were touched?
- Which files changed?
- Which call sites were audited?
- Were similar patterns searched and fixed where appropriate?
- Was any public header changed? If yes, why is that exposure necessary?
- Did `main.c` learn module details? It should not.
- Did `watchface_components.h` gain behavior? It should not.
- Did a module lose lifecycle ownership? It should not.
- Did the change introduce tiny helpers that should stay inline?
- Did the change alter visuals, platform support, AppMessage keys, palette, or
  fonts unintentionally?
- What validation was run?
- What remains intentionally deferred?

## Current Validation Baseline

As of this ledger creation:

- `pebble build` passes for the configured rectangular platforms.
- Worktree was clean before this ledger file was added.
- Round support is not implemented or enabled.
