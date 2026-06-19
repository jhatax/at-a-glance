# Architecture Ledger

This ledger is the current source of truth for At A Glance architecture,
invariants, and cleanup backlog. Start here for non-trivial code work.

---

## Context Summary

At A Glance is a mature Pebble watchface organized around a prepared
`WatchfaceSurface`. Treat it like embedded firmware on small ARM devices:
design first, audit live code, preserve lifecycle safety, avoid cleverness, and
validate before staging or committing.

Durable engineering values:

1. Safety first: no stale pointers, partial layer ownership, unmatched resource
   lifetimes, buffer overflows, or hidden failure branches.
2. Architecture before edits: understand ownership, state, APIs, and visual
   consequences before changing code.
3. Small coherent slices: cleanup, visual changes, AppMessage changes,
   platform work, and glyph tuning are separate unless explicitly approved.
4. Direct code over helper churn: keep simple calculations inline; add helpers
   only for real boundaries, repeated complexity, callbacks, or non-obvious
   domain math.
5. Product constants are decisions, not active mutable profiles.
6. Visual hierarchy is part of product correctness: time dominates, text is
   primary, icons support recognition, and unavailable state uses shared calm
   vocabulary.

---

## Working Cycle

1. Audit live source, docs, generated files, dirty tree, and relevant memory.
2. Reconcile the request against this ledger and `RAID_LOG.md`.
3. State patch shape, touched files, invariants, and validation before editing.
4. For API, lifecycle, layout, or platform changes, review the plan first.
5. Edit narrowly and preserve unrelated user work.
6. Validate with `git diff --check`; run `pebble build` for code changes.
7. Use emulator screenshots for visual/layout changes when practical.
8. Review the diff, stage only intended files, and summarize deferred risk.
9. Do not commit API/lifecycle changes without approved before/after flow and
   commit message.
10. Update this ledger when an accepted decision changes.

---

## Core Runtime Flow

```text
ataglance.c
  -> settings_load(&settings)
  -> watchface_create(window, &settings)
       -> layout_watchface_initialize(width, height, &surface)
            -> memset(surface, 0, sizeof(*surface))
            -> calculate active blueprint and final geometry
            -> store compact/full on surface.style.is_compact
       -> layout_watchface_update_palette(&surface.style, display_mode)
       -> feature_module_create(root, &surface)
       -> watchface_refresh(...)
```

Display-mode repaint:

```text
ataglance.c updates settings
  -> watchface_repaint()
       -> layout_watchface_update_palette(&surface.style, display_mode)
       -> window_set_background_color(...)
       -> refresh created strata
```

---

## Core Invariants

- `watchface` owns live `WatchfaceSurface` storage and lifetime.
- Layout initialization clears and prepares the caller-owned surface from
  scratch.
- There is one public layout initialization contract. Rectangular and round
  implementations provide it behind platform guards.
- The architect owns geometry and compact/full classification.
- Compact/full is resolved once by the architect and stored on
  `WatchfaceSurfaceStyle.is_compact`; the stylist consumes it.
- Blueprints are immutable constants/product choices. Calculated layout is the
  resolved geometry used to lay strata on the surface.
- Layout-private metrics do not leak into `WatchfaceSurface`.
- Feature modules own Pebble layer lifecycle and source state.
- `create(root, surface)` may retain the calculated surface once.
- Required layer/resource creation gates module success and retained state.
  Optional resources may be attempted after retaining `surface` only when their
  failure is explicitly non-fatal and their partial resources are cleaned up
  before returning success.
- `refresh()` APIs do not accept `WatchfaceSurface`; they accept only narrow
  runtime payloads where needed.
- `ataglance.c` is the Pebble container and transport/service adapter. It may
  construct `WatchfaceEventData` for AppMessage tuples and Pebble service
  events, but it must not interpret domain meaning, mutate module state, or
  compute refresh/repaint behavior.
- `watchface_apply_received_data()` is the single runtime ingress for watchface
  updates.
- Dynamic colors remain module-owned when they depend on live source state,
  such as BPM or battery.
- Public headers expose only concepts callers need.

---

## Source Map

- `src/c/ataglance.c`: Pebble lifecycle, services, settings, AppMessage
  parsing, and dispatch to `watchface`.
- `src/modules/watchface.c/.h`: runtime clearing house, surface owner, module
  create/destroy order, refresh dispatch, and source-state setter boundary.
- `src/modules/watchface_components.h`: shared display component types and
  design input dimensions.
- `src/modules/layout.h`: public layout initialization/style API.
- `src/modules/layout_architect.c`: current geometry provider and surface
  preparation implementation.
- `src/modules/layout_design.h`: private design constants, blueprints, and
  calculated layout structs.
- `src/modules/layout_stylist.c`: palette, font-role, and custom-font
  resolution.
- `src/modules/substratum_renderer.c/.h`: shared TextLayer/Icon setup,
  color-role lookup, icon coordinate scaling, and shared glyph primitives.
- `src/modules/settings.c/.h`: settings defaults, persistence, validation, and
  HR sample interval mapping.
- `src/modules/date.c/.h`: date formatting and date text lifecycle.
- `src/modules/time.c/.h`: time formatting, time text lifecycle, custom font.
- `src/modules/climate.c/.h`: weather source state, temperature formatting,
  availability, and text/icon lifecycle.
- `src/modules/climate_glyphs.c/.h`: Open-Meteo code mapping and procedural
  weather glyph rendering.
- `src/modules/battery.c/.h`: battery source state, track/bolt layers, battery
  color decisions, and refresh.
- `src/modules/bpm.c/.h`: BPM source state, text/icon lifecycle, health reads,
  and BPM color decisions.
- `src/modules/steps.c/.h`: steps source state, bitmap/icon/text lifecycle,
  health reads, and refresh.
- `src/pkjs/index.js`: Clay bootstrap, geolocation, Open-Meteo fetch, request
  sequencing, and weather AppMessage sends.
- `src/pkjs/config.json`: Clay configuration UI.
- `package.json`: metadata, platforms, capabilities, message keys, resources.
- `wscript`: Pebble build entry point.

---

## Product And Visual Decisions

- Supported build targets currently include `aplite`, `basalt`, `diorite`,
  `emery`, `flint`, `chalk`, and `gabbro`.
- Required capabilities are `configurable`, `location`, and `health`.
- Shared visual stack:

  ```text
  top metric context
  dominant centered time
  centered battery track and charging bolt
  weather/date context
  bottom metric context
  ```

- The shared stack is visual DNA, not shared coordinates.
- Rectangular and round geometry may differ, but the product should read as the
  same watchface.
- Time remains visually dominant.
- Text is primary. Icons support recognition.
- Text widths and heights are per-field product/layout decisions dictated by
  font selection, value role, row ownership, and available geometry.
- Battery uses a primary-color outline track and module-owned state-color fill.
- Weather/date share one row; weather owns icon plus temperature and date is
  aligned in the remaining space.
- Health metrics use centerline vocabulary rather than far-corner placement.
- Unavailable data uses the shared slash vocabulary.
- Round targets need screenshot-led validation on Chalk and Gabbro before
  publication confidence.

---

## Header And Code Audit Findings

Observed in the live tree during the June 17, 2026 audit and resolved:

- `src/c/ataglance.h` was deleted after its constants were removed or moved.
- `ATAGLANCE_USE_AND_PERSIST_SETTINGS` was removed; settings are loaded during
  init and saved during deinit.
- `ATAGLANCE_MAX_STR_LEN` was replaced with module-local buffer constants in
  date, time, climate, BPM, and steps.
- `DEBUG_ATAGLANCE` was replaced by the build-defined `ATAGLANCE_DEBUG` gate,
  with defensive default guards in modules that compile debug branches.
- Public module headers no longer include the app entrypoint header for debug
  gates.
- Dead `ataglance.h` includes were removed from implementation files.
- `src/modules/layout_design.h` is included only by `layout_architect.c`; its structs
  and constants are implementation details.
- `layout_architect.c` includes `layout.h` directly because it implements the
  public layout initialization contract. `layout_design.h` includes only the
  component types it directly needs.
- `HELPER_CLAMP_MIN` is unused and not fully parenthesized.
- `HELPER_SCALE_ROUND` is used, but not fully parenthesized.
- `helper_swap_colors_in_bitmap()` is declared and implemented but unused.
- `ColorPalette.background_layer_background` and
  `ColorPalette.background_layer_rule` are unused live struct members.
- `WatchfaceBackgroundStratum.rule_enabled` and `.rule` are written as
  disabled/zero and not consumed.
- `WatchfaceSurface.background.frame` is assigned but there is no live
  background module consuming the background stratum.
- `WATCHFACE_UPDATE_BACKGROUND` exists in the public update mask but has no
  live refresh path.
- `LayoutBlueprint.icon_text_pair_height` is initialized but not consumed.
- `DESIGN_COMPACT_RIGHT_TEXT_X` is unused.
- `src/modules/battery.c` still uses `rule` naming for the battery track layer.
- `substratum_renderer_draw_filled_bolt_in_frame()` reaches
  `draw_filled_polygon_in_frame()`, which heap-allocates a bounded point array
  during a layer update proc. This is not dead code, but it is a renderer
  hardening target.
- Ignored local scratch/artifact files should stay out of the source tree.
- Tracked prototype artifacts such as `src/scratch.txt` should be reviewed
  before GitHub publication.
- `design-preview.html` is tracked and should be reviewed before publication.

---

## Backlog

### Phase 1: Documentation And Publication Prep

1. Finalize README screenshots for representative color, monochrome, and round
   targets.
2. Use `appstore-submission.md` to prepare PBW, screenshots, metadata,
   capabilities disclosure, and release notes.
3. Confirm generated PBW filename after any project/package rename.
4. Review `project-rename-plan.md` before renaming the repo folder to
   `at-a-glance`.

### Phase 2: Header Cleanup

1. Remove `ATAGLANCE_USE_AND_PERSIST_SETTINGS`; always load settings during
   init and save during deinit.
2. Replace `ATAGLANCE_MAX_STR_LEN` with module-local buffer constants sized to
   actual formatted values. Current target modules are `date.c`, `time.c`,
   `climate.c`, `bpm.c`, and `steps.c`.
3. Keep `WATCHFACE_UNAVAILABLE_TEXT` in `watchface_components.h`; it is shared
   display vocabulary, not buffer storage.
4. Move debug gating to the build-defined `ATAGLANCE_DEBUG` value. Public
   module headers should not include the app entrypoint header for debug gates.
5. Remove `ataglance.h` from public module headers.
6. Remove dead `ataglance.h` includes from implementation files.
7. Delete `src/c/ataglance.h` when no symbols remain.
8. Keep private design constants and calculated layout structs in
   `layout_design.h` unless that header stops clarifying `layout_architect.c`.

### Phase 3: Dead Code And Struct Cleanup

1. Remove or use `HELPER_CLAMP_MIN`.
2. Fully parenthesize helper macros that remain.
3. Remove `helper_swap_colors_in_bitmap()` if no planned caller remains.
4. Remove unused `ColorPalette` background-layer fields.
5. Remove or reconnect unused `WatchfaceBackgroundStratum` fields. If there is
   still no background module, consider removing the background stratum from
   `WatchfaceSurface` entirely and keep background color as window style.
6. Remove `WATCHFACE_UPDATE_BACKGROUND` unless a real background refresh path is
   restored.
7. Remove `LayoutBlueprint.icon_text_pair_height` if it remains unused.
8. Remove `DESIGN_COMPACT_RIGHT_TEXT_X`.
9. Rename battery `rule` layer names to `track` vocabulary if approved.
10. Delete tracked scratch/prototype artifacts if they are not intentional
   source artifacts.

### Phase 4: Lifecycle And Renderer Hardening

Status: Closed.

Required layer/resource creation gates module success and retained state.
Optional resources are explicitly non-fatal when the text-bearing stratum
remains usable, and partial optional resources are cleaned up before returning
success. The shared renderer no longer heap-allocates bounded polygon point
storage in draw paths.

### Phase 5: Round And Visual Validation

Status: Closed for the current milestone.

Round screenshot-led validation on Chalk and Gabbro has been confirmed
complete. Future round changes still require screenshot review, but there is no
active round visual validation blocker for the current rectangular-completion
milestone.

### Phase 6: PebbleKit JS And Data Robustness

Status: Closed.

PebbleKit JS weather handling uses request ids to ignore stale callbacks.
Geolocation denial or unavailability falls back to the fixed OAK weather
location. Open-Meteo network errors, timeouts, non-200 responses, parse
failures, and malformed current-weather payloads send unavailable weather
sentinels. OAK remains the fixed documented fallback location.

---

## Reconciliation Checklist

Before closing a task, answer:

- Which ledger decisions applied?
- Which invariants were touched?
- Which files changed?
- Were public headers changed?
- Did `ataglance.c` learn module details?
- Did `watchface_components.h` gain behavior?
- Did a module lose lifecycle ownership?
- Did the change introduce tiny helpers that should stay inline?
- Did visuals, platforms, AppMessage keys, palette, fonts, or geometry change?
- Were similar patterns audited?
- What validation ran?
- What remains intentionally deferred?
