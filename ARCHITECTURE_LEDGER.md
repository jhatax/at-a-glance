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
5. Agree on invariants.
   Explicitly name the contracts, ownership boundaries, lifecycle assumptions,
   and behavior that must remain unchanged.
6. Challenge foundational assumptions.
   For platform/layout work, name the design authority and identify whether
   the slice reuses architecture, geometry, constants, scaling policy, or
   behavior. Record open risks, assumptions, active issues, and decisions in
   `RAID_LOG.md`.
7. Edit narrowly.
   Keep the slice coherent but local. Do not mix refactors, visual changes,
   AppMessage changes, and platform enablement unless explicitly approved.
8. Validate.
   Run `git diff --check`; run `pebble build` for code changes unless the user
   explicitly pauses validation. Use emulator screenshots for visual slices.
9. Document flow before commit.
   For any new API, changed API, changed lifecycle path, or changed caller
   behavior, create a clean before/after flow diagram and a proposed commit
   message. The flow review must cover callers and users of the API, not only
   the implementation diff.
10. Get approval before commit.
   The revised flow(s) and commit message must be approved before staging or
   committing. A successful build and post-hoc diff review are not sufficient
   approval for API, lifecycle, or caller-flow changes.
11. Reconcile.
   Confirm each invariant and acceptance check as implemented, violated,
   intentionally deferred, or not applicable.
12. Commit cleanly.
   Stage only intended files. Do not commit build artifacts, screenshots, or
   unrelated scratch files.
13. Update the ledger.
   After the accepted code flow lands, update this ledger for any changed
   architecture decision, invariant, or acceptance check.

## Development Guardrails

- Mature codebase rule: do not overreach. Prefer local, deliberate changes.
- Do not create helper functions when the real logic is clearer inline,
  especially for fewer than roughly 10 lines of straightforward code.
- Do not introduce private helpers for five or fewer simple logic statements
  unless they are required by a framework callback signature, hide genuinely
  non-obvious domain math, or remove meaningful repeated complexity.
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
- Do not commit a new API, changed API, or changed API caller flow without an
  approved before/after flow diagram and approved commit message.
- Do not let a cleaned-up implementation hide an unresolved product premise.
  If a slice depends on a foundational assumption, state it and either defend
  it, challenge it, or record it in `RAID_LOG.md`.
- Preserve user changes. Never revert unrelated dirty work.

## Current Architecture Decisions

### Product Constants

Decision: product constants are split by ownership. `src/c/ataglance.h` owns
app-level flags and string limits. `src/modules/watchface_components.h` owns
shared display design inputs.

Invariants:

- `ataglance.h` may define debug flags, app-level string limits, and
  persisted-settings toggles.
- `watchface_components.h` may define shared design dimensions, sizing
  constants, font roles, color roles, and surface component types.
- Design face width/height remain canonical product constants.
- Design icon width/height remain canonical product constants because icon
  glyph and renderer coordinate math use them as the icon design-space bounds.
- Product-constants headers must not own Pebble layer helpers, module
  lifecycle APIs, or runtime watchface orchestration.

Acceptance checks:

- `DEBUG_ATAGLANCE` is defined in `ataglance.h` when local debug support is
  enabled.
- Module APIs are not declared here.

TODO: Retire `src/c/ataglance.h` in a focused header cleanup slice.

- Remove `ATAGLANCE_USE_AND_PERSIST_SETTINGS`; settings persistence is the
  expected product behavior now that Clay exposes a configuration page.
  `main.c` should always load persisted settings on init and save settings on
  deinit.
- Replace `ATAGLANCE_MAX_STR_LEN` with module-local buffer constants. Date,
  time, climate, battery, BPM, and steps should size their own buffers based
  on the strings they actually format.
- Move `DEBUG_ATAGLANCE` out of `ataglance.h`. Prefer a compiler/build define
  if the Pebble build path supports it cleanly; otherwise use a narrowly named
  debug configuration header instead of a product constants header.
- Remove `ataglance.h` includes from files that do not use its symbols,
  including current dead includes in layout/style/renderer/glyph modules.
- Remove `ataglance.h` includes from public module headers by relocating the
  debug declaration gate or debug key ownership to a narrower boundary.
- Delete `src/c/ataglance.h` after all call sites and docs are reconciled.
- Update `agents.md`, this ledger, and any active handoff docs that still
  describe `ataglance.h` as an app/product constants owner.
- Remove or move `src/c/scratch.txt` out of the source tree after confirming
  it is disposable; source-tree scratch files pollute header and symbol
  audits.

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

### Layout API

Decision: the public layout boundary is split between first-time geometry
initialization and repeatable style updates:

- `layout_watchface_initialize()`
- `layout_update_watchface_style()`

TODO: After this commit series lands, make helper macros defensive with fully
parenthesized parameters and expressions.

Invariants:

- Watchface owns `WatchfaceSurface` storage, lifetime, and initialization
  idempotence.
- Shape-specific layout initialization prepares the caller-owned surface from
  scratch: it clears the surface and applies its blueprint/geometry.
- The `memset(surface, 0, sizeof(*surface))` belongs in the active shape
  implementation because clearing the surface is step one of first-time
  layout initialization.
- The layout initialization contract is singular. Platform-specific
  rectangular and round implementations provide the same public entry point
  behind shape guards.
- Shape architects resolve geometry and compact/full classification once, then
  store the compact/full state on `WatchfaceSurfaceStyle`.
- The stylist consumes the resolved `WatchfaceSurfaceStyle` state; it does not
  re-derive compact/full from dimensions.
- Style updates are separate from geometry initialization so repaint can
  cascade display-mode changes without clearing or reinitializing geometry.
- Layout initialization calculates data; it does not create Pebble layers.

Acceptance checks:

- `watchface.c` calls `layout_watchface_initialize()` during watchface
  creation only.
- `watchface_repaint()` calls `layout_update_watchface_style()` but never
  calls `layout_watchface_initialize()`.
- Public layout headers do not expose text-layer creation, icon
  scaling, private blueprint metrics, or color-role lookup.

### Layout Stylist

Decision: `src/modules/layout_stylist.c` owns visual style resolution for
layout behind the public `layout.h` API.

Invariants:

- Palette selection, font-role to system font mapping, and custom font
  resource decisions happen here.
- Compact/full display classification is consumed here from the calculated
  surface style; the stylist must not recompute it from face dimensions during
  display-mode refresh.
- Modules may load/unload custom fonts as lifecycle owners, but they do not
  decide which font resource should be used.
- Compact classification is architect-owned because the active architect owns
  the geometry context and active blueprint policy.

Acceptance checks:

- Time module reads `surface->style.custom_font_resource_ids`; it does not
  decide resource IDs from face dimensions.
- Current implemented palette is the Shinkansen palette documented in
  `palette-options.md`.

### Layout Architect

Decision: shape architect implementations privately own shape-specific
geometry behind one architect contract.

Invariants:

- Architects assign final `x`, `y`, `w`, and `h` to each substratum.
- Architects may use file-local metric structs to improve readability.
- Architects consume canonical product constants through private file-local
  blueprints, not through public profile objects or mutable runtime copies.
- Architects derive private resolved metrics from their blueprint and face
  dimensions. They must not mutate, copy, or reinterpret immutable product
  constants as runtime state.
- Architect blueprints are private implementation details and must not be
  exposed through public surface APIs or `WatchfaceSurface`.
- Blueprints do not include dead or speculative spacing fields such as
  `column_gap`, and do not include a separate optical x-inset while
  `content_margin` owns edge spacing.
- Blueprint values are initialized from canonical product constants.
- Rectangular dimensions only scale in the compact blueprint where approved.
  Full rectangular displays use canonical values directly.
- The current rectangular visual order is open top space, dominant time,
  centered battery track, weather/date context, then bottom steps/BPM
  context.
- Rectangular weather/date context places climate icon plus temperature first,
  then starts the date text box after that weather stratum plus
  `icon_text_gap`. Date text is right-aligned in the remaining content width.
- Layout is not a generic row engine; product strata remain fixed and known.
- Round platform support requires a real round architect, clean builds, and
  emulator screenshot validation for the enabled round targets.

Acceptance checks:

- Rectangle geometry does not leak private metrics into
  `WatchfaceSurface`.
- Rectangle-only slices do not add or alter round platform support.

### Substratum Renderer

Decision: `src/modules/substratum_renderer.c/.h` owns common Pebble rendering
helpers for calculated substrata.

Invariants:

- It may create text layers, create icon layers, update text layers, resolve
  static color roles, and scale design icon coordinates.
- It owns shared glyph primitives when the geometry contract is reused across
  modules, including the unavailable slash, shared polygon outlines, and
  frame-aware line/circle/corner-derived fill helpers.
- Weather glyph composition may keep private helpers such as
  `weather_subframe()`, but repeated frame-aware primitive math belongs in the
  renderer once it is shared.
- It does not own module source state, text formatting, dynamic colors, icon
  update procs, service data, AppMessage data, or glyph behavior decisions.
- Icon update procs are passed into icon creation by the owning module.

Acceptance checks:

- Feature modules call renderer helpers for common TextLayer/Icon layer setup.
- Shared glyph contracts are not reimplemented privately in multiple feature
  modules.
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
- Display-mode handling is an explicit global repaint path. `main.c` updates
  settings and calls `watchface_repaint()`, which cascades style and refreshes
  existing strata without reinitializing geometry.
- The strata-only redraw mask is private to `watchface.c`; `main.c` must not
  know about strata.
- Setters mutate source state only. Rendering happens through a later refresh.
- Module refresh calls do not pass `WatchfaceSurface`. `create(root, surface)`
  establishes each module's retained calculated-surface contract; refresh calls
  may pass only narrow runtime payloads such as time format or temperature
  unit.

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
- The boundary between `main.c` and `watchface` remains narrow: `main.c`
  parses AppMessage transport data into typed runtime values, then calls
  watchface setters. `watchface` must not parse raw dictionaries or own
  transport-level message semantics.
- Do not introduce a shared runtime-update package between `main.c` and
  `watchface` unless message volume, update frequency, or atomic multi-field
  coherence makes the narrow-setter contract meaningfully worse. Revisit this
  boundary explicitly if AppMessage traffic becomes large, frequent, or hard to
  coalesce cleanly.

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
- Modules validate their retained `s_surface` during refresh/render. Refresh
  must not accept or replace a module's surface pointer.

Acceptance checks:

- `*_module_create()` returns true only after required layers are created.
- Destroy paths clean up every layer/font the module owns.
- Watchface records creation success only from module create return values.
- No feature-module `*_refresh()` declaration accepts `WatchfaceSurface`.
- Payload-bearing refresh APIs pass only the runtime value needed to render.

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

TODO: In a focused follow-up slice, standardize rule naming across live code
and current guardrail docs. Use `rule` as the canonical term wherever
`horizon` or similar shorthand still refers to the background rule.

Acceptance checks:

- `watchface.c` does not draw the rule directly.
- `background_module_refresh()` is used when palette/display mode changes.

### Visual Redesign

Decision: rectangle and round visual changes must stay separate from
architecture cleanup unless the user explicitly approves the combined slice.

Invariants:

- Do not mix visual redesign with architecture cleanup.
- Rectangle visual redesign remains its own visual slice.
- Round visual changes remain their own visual slice.
- Round support requires a shape-specific round architect and screenshot
  review on round emulators before claiming visual readiness.

Acceptance checks:

- `layout-architect-role-flow.md` documents the proposed visual slices.

## Reconciliation Checklist

Before marking a task complete, answer these explicitly:

- What decisions from this ledger apply?
- Which invariants were touched?
- Which files changed?
- Which call sites were audited?
- Were public and private headers audited for unnecessary includes?
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

As of the latest ledger refresh:

- Rectangular and round architects exist behind the shared architect contract.
- `package.json` currently targets rectangular platforms only; round layout
  code exists but `chalk` and `gabbro` are not enabled in the manifest.
- Build and emulator validation are task-scoped; rerun them before claiming a
  code slice is complete.
- The worktree may contain active user changes; inspect live status before
  staging or committing.
