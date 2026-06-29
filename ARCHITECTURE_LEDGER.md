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

## Non-negotiables

1. Treat every change as small firmware work: reason about lifecycle, memory, platform capability, message contracts, layout, and visible behavior before coding.
2. Do not commit files that were not touched for the current explicit slice, and do not turn a narrow instruction into a broader cleanup.

---

## Constraints

1. Use integer layout and drawing math. Do not introduce floating-point math in watchface layout or render paths.
2. Avoid heap allocation unless the Pebble API requires it; every acquired resource needs a matching destroy path, including create-failure paths.
3. Use fixed-size buffers and snprintf; never introduce sprintf, strcpy, or unbounded string handling.
4. Keep render/update callbacks lightweight: no allocation, network, JSON, heavy formatting, or layout calculation inside layer update procs.
5. Treat color, health, sensors, screen shape, resources, and phone data as optional unless verified for the target platform.
6. Prefer capability and shape guards such as PBL_COLOR, PBL_BW, PBL_HEALTH, PBL_RECT, PBL_ROUND, and PBL_API_EXISTS().
7. Do not guess lifecycle, AppMessage, generated resource, or SDK behavior; verify it.
8. Preserve user work and keep one coherent slice at a time.
9. Prefer direct, auditable code over clever abstractions or tiny helper wrappers.
10. Optimize for glanceability, legibility, stable layout, and maintainable embedded C.

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
- Layout initialization clears and prepares the caller-owned surface from scratch.
- There is one public layout initialization contract, implemented by layout_architect.
- The architect owns geometry and compact/full classification.
- Compact/full is resolved once by the architect and stored on `WatchfaceSurfaceStyle.is_compact`;
the stylist consumes it.
- Blueprints are immutable constants/product choices. Calculated layout is the
  resolved geometry used to lay strata on the surface.
- Layout-private metrics do not leak into `WatchfaceSurface`.
- Feature modules own Pebble layer lifecycle and source state.
- Feature module `create()` APIs receive only the substrata, frames, and fonts
  they need from the prepared surface.
- Feature module `refresh()` APIs receive the current palette and narrow runtime
  payloads where needed.
- Feature modules do not retain `WatchfaceSurface*`; they retain only module
  source state and minimal callback context required by Pebble update procs.
- Required layer/resource creation gates module success and retained state.
  Optional resources may be attempted only when their failure is explicitly
  non-fatal and their partial resources are cleaned up before returning success.
- `ataglance.c` is the Pebble container and transport/service adapter. It may
  construct `WatchfaceEventData` for AppMessage tuples and Pebble service
  events, but it must not interpret domain meaning, mutate module state, or
  compute refresh/repaint behavior.
- `watchface_apply_received_data()` is the single runtime ingress for watchface
  updates.
- Dynamic colors remain module-owned when they depend on live source state,
  such as BPM or battery.
- `helper.h` is shared utility vocabulary. Any module or component may include
  it directly when it uses helper macros or helper APIs; direct `helper.h`
  inclusion is not header leakage by itself.
- Bitmap palette mutation helpers require palettized PNG resources; callers
  should treat non-palettized bitmaps as unsupported for in-place recoloring.
- Module-specific palette policy belongs to the owning feature module, not to
  renderer helpers. Climate owns its light/dark `ClimatePalette` templates and
  retains the current copied climate palette for Pebble update procs;
  `climate_glyphs.c` only renders from a fully-resolved `ClimatePalette*`.
- Module-owned palettes use `normal` for the injected `ColorPalette.primary_text`
  color. `normal` must not equal the active background: the base palette defines
  primary text as legible over that background, and the module palette loaded
  macros use `normal != background` as the initialized-palette sentinel.
- Steps icon foreground is selected with `gcolor_legible_over()` against the
  active palette background. This reflects real-device glanceability testing on
  lower-brightness e-paper displays, where maximum foreground/background
  contrast matters more than matching the primary text color.
- Steps display treats zero steps as unavailable and renders the shared `---`
  token. Product intent: `0` is the analog of a blank cockpit display panel,
  not a useful glanceable metric.
- BPM keeps a fixed 2px waveform stroke after real Flint hardware testing showed
  the scaled procedural glyph could become effectively invisible. This is an
  accepted product legibility decision, not a boundary-migration accident.
- The current BPM icon remains a procedural glyph. Product direction is to
  replace it with a real BPM icon if a more legible asset is found.
- Public headers expose only concepts callers need.
- Final header split: `watchface.h` owns runtime/app ingress and refresh
  contracts; `watchface_components.h` owns only reusable display primitives used
  by feature modules and renderer helpers; `layout_surface.h` owns the assembled
  `WatchfaceSurface`/style output contract; `layout.h` owns the public layout
  facade; and `layout_design.h` owns private layout design inputs and calculated
  metrics.
- Feature module headers must not include `layout.h`; they consume prepared
  substrata, fonts, and palettes through `watchface_components.h`.

---

## Header Boundaries

This is the final accepted header split for the current architecture. Do not add
more layout headers, split these headers again, or move concepts between them
without first proving that the existing split cannot express the boundary.

| Header | Owns | Direct Consumers | Must Not Own |
| --- | --- | --- | --- |
| `watchface.h` | Runtime/app ingress, update masks, event masks, `WatchfaceEventData`, weather transport sentinels, public watchface lifecycle/refresh/apply APIs | `ataglance.c`, `watchface.c`, `watchface_runtime_boundary.c`, narrow implementation consumers of watchface transport values | Layout surface structs, feature-module substrata, renderer helpers, glyph details, design constants |
| `watchface_components.h` | Reusable display primitives: shared unavailable text token, uninitialized text color, font/color roles, `ColorPalette`, icon grid constants, text/icon/battery substrata | Feature module headers, `substratum_renderer.h`, `layout_surface.h`, `layout_design.h` when it needs component-shaped calculated output | `WatchfaceSurface`, `WatchfaceSurfaceStyle`, runtime masks, AppMessage parsing/data packets, layout blueprints, private calculated metrics |
| `layout_surface.h` | Assembled layout output contract: `WatchfaceTextStratum`, `WatchfaceTextWithIconStratum`, `WatchfaceSurfaceStyle`, `WatchfaceSurface` | `layout.h`; direct includes require a fresh include-graph audit | Feature module APIs, runtime event masks, AppMessage keys, module source state, glyph drawing policy, private design constants |
| `layout.h` | Public layout facade: layout initialization, palette update, font lifecycle APIs, and the re-exported layout surface contract | `watchface.c`, `layout_architect.c`, `layout_stylist.c` | Feature module APIs, runtime event masks, AppMessage keys, module source state, glyph drawing policy, private design constants |
| `layout_design.h` | Private layout design vocabulary: geometry constants, font design constants, blueprints, and calculated layout structs used to prepare a `WatchfaceSurface` | `layout_architect.c`, `layout_stylist.c` only when it needs design/font constants | Public runtime contracts, feature-module APIs, Pebble service/AppMessage contracts, module source state |
| `watchface_debug.h` | Debug build gate normalization and debug ingress declarations only | Modules or runtime files that compile debug-only branches | Helper APIs, product flags, layout policy, runtime behavior implementation |
| `helper.h` | Shared utility macros/functions with no module ownership | Any module/component that directly uses a helper macro/API | Product policy, hidden runtime dispatch, feature-specific source state |

Header inclusion directives:

1. `ataglance.c` includes `watchface.h` and must not include layout, component,
   or feature-module headers.
2. Feature module headers include `watchface_components.h`, not `layout.h`.
   They receive prepared substrata, fonts, palettes, and narrow runtime payloads.
3. `watchface.c` is the joining point that may include `watchface.h`, `layout.h`,
   and feature module headers because it owns surface lifetime and dispatch.
4. `layout_architect.c` and `layout_stylist.c` may include `layout.h`; they are
   the only implementation files that should need the assembled surface/style
   contract outside `watchface.c`.
5. `layout_design.h` remains implementation-private. If another file appears to
   need it, audit whether the needed value belongs in a public component
   primitive before adding the include.
6. `watchface_components.h` may expose reusable display primitives, but it must
   not become a home for assembled layout output or runtime packets.
7. `layout_surface.h` owns the assembled surface/style structs. Existing
   consumers should continue to include `layout.h` unless a future audit proves
   a direct `layout_surface.h` include is the narrower boundary.
8. Anonymous enums are acceptable for constant groups whose type name is not
   used. Keep named enum typedefs only when the type itself is part of an API,
   a cast target, or useful persisted-domain documentation.

Future direction:

- Pay down header debt by narrowing consumers, not by creating more headers
  after the accepted `layout_surface.h` split.
- If a feature module needs a new display primitive, add the smallest reusable
  shape to `watchface_components.h`; do not expose `WatchfaceSurface`.
- If a layout implementation needs a new private design input, add it to
  `layout_design.h`; do not leak it through `layout.h`.
- If runtime transport grows, keep packet and mask shapes in `watchface.h`; do
  not push runtime ingress into feature module headers.
- Any future header movement must include an include-graph audit and answer:
  who needs the symbol, who should not see it, and whether the current final
  split already has the correct home.

## Runtime Boundary RACI

| Runtime concern | Responsible | Accountable | Consulted | Informed |
| --- | --- | --- | --- | --- |
| Pebble app lifecycle, window ownership, service subscriptions, and AppMessage inbox setup | `ataglance.c` | `ataglance.c` | `watchface.c` for create/destroy contract | Feature modules through `watchface.c` lifecycle calls |
| AppMessage tuple lookup and transport parsing | `ataglance.c` | `ataglance.c` | `watchface.h` for `WatchfaceEventData` shape and mask values | `watchface_runtime_boundary.c` through populated event data |
| Pebble service callbacks converted to watchface events | `ataglance.c` | `ataglance.c` | Pebble SDK service contracts | `watchface_runtime_boundary.c` through `WatchfaceEventData` service bits |
| Domain validation of settings values | `watchface_runtime_boundary.c` | `watchface_runtime_boundary.c` | `settings.h` validators | `ataglance.c` only through changed `WatchfaceSettings` |
| Settings mutation from received watchface events | `watchface_runtime_boundary.c` | `watchface_runtime_boundary.c` | `settings.h` persisted field definitions | `ataglance.c` by pre/post settings comparison |
| Settings persistence | `ataglance.c` | `ataglance.c` | `settings.c` save contract | `watchface_runtime_boundary.c` does not receive the persistence result |
| HR sample period application | `ataglance.c` | `ataglance.c` | `settings.h` HR period mapping and Pebble health service contract | `watchface_runtime_boundary.c` only mutates the setting when valid |
| Weather transport completeness from received/parsed masks | `watchface_runtime_boundary.c` | `watchface_runtime_boundary.c` | `WatchfaceEventData.received` and `.parsed` semantics | `climate.c` receives an atomic `ClimateUpdate` packet with `is_complete` set by runtime |
| Weather domain validity and source-state fallback | `climate.c` | `climate.c` | Open-Meteo code mapping and climate glyph contract | `watchface_runtime_boundary.c` supplies raw complete/incomplete weather packets; climate applies valid weather or clears stale state |
| Debug health data application | `watchface_runtime_boundary.c` under `PBL_HEALTH && ATAGLANCE_DEBUG` | `watchface_runtime_boundary.c` | `bpm.c` and `steps.c` debug module APIs | `ataglance.c` only parses debug transport keys |
| Repaint versus refresh decision for watchface events | `watchface_runtime_boundary.c` | `watchface_runtime_boundary.c` | `watchface.c` public repaint/refresh API | `ataglance.c` does not receive or inspect this decision |
| Module refresh dispatch and surface lifecycle | `watchface.c` | `watchface.c` | Feature module create/refresh/destroy contracts | `ataglance.c` calls only public watchface entry points |

The runtime boundary does not return a runtime-update mask to `ataglance.c`.
That earlier option was removed in favor of the RACI split: `ataglance.c`
parses and owns container responsibilities, while the watchface runtime owns
event interpretation and visual update decisions. `ataglance.c` may persist
settings or apply HR service changes only by comparing `WatchfaceSettings`
before and after `watchface_apply_received_data()`.

---

## Source Map

- `src/c/ataglance.c`: Pebble lifecycle, services, settings, AppMessage
  parsing, and dispatch to `watchface`.
- `src/modules/watchface.c/.h`: runtime clearing house, surface owner, module
  create/destroy order, refresh dispatch, and source-state setter boundary.
- `src/modules/watchface_components.h`: shared display primitives for feature
  modules and renderer helpers: color/font roles, palettes, icon grid constants,
  and text/icon/battery substrata.
- `src/modules/layout_surface.h`: assembled surface/style output contract.
- `src/modules/layout.h`: public layout facade that re-exports the surface
  contract for `watchface.c`, `layout_architect.c`, and `layout_stylist.c`.
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
  top heart-rate context
  dominant centered time
  centered battery track and charging bolt
  weather/date context
  bottom steps context
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
- Health metrics use centerline vocabulary rather than far-corner placement;
  BPM owns the top context row and steps own the bottom context row on health
  platforms.
- Unavailable data uses the shared slash vocabulary.
- Round targets need screenshot-led validation on Chalk and Gabbro before
  publication confidence.

---

## Current Closeout State

Resolved and now part of the steady-state contract:

- `src/c/ataglance.h` is gone. Runtime ingress lives in `watchface.h`.
- Settings always load through `settings_load()` and persist through the app
  lifecycle; `ATAGLANCE_USE_AND_PERSIST_SETTINGS` is gone.
- Module-local fixed buffers own formatted text storage in date, time, climate,
  BPM, and steps.
- `ATAGLANCE_DEBUG` is the only accepted debug build gate.
- The background stratum and `WATCHFACE_UPDATE_BACKGROUND` are gone; background
  color is window/style state.
- Runtime update-mask plumbing no longer leaks back into `ataglance.c`.
- Weather transport is atomic at the runtime boundary: completeness is checked
  before climate state is applied, and incomplete payloads clear stale weather.
- Round validation is complete for the current shipped hierarchy. Future round
  edits still need screenshot review.
- The shared renderer hardening pass is complete: bounded polygon drawing no
  longer heap-allocates in draw paths, and icon-coordinate scaling clamps
  against the resolved target frame.
- The live documentation set is now `README.md`, `ARCHITECTURE_LEDGER.md`,
  `RAID_LOG.md`, `VisualVocabulary.md`, `DESIGN.md`, and `User Interface.md`.
  Historical planning material under `archive/` is reference context only.

---

## Remaining Active Technical Debt

Only keep items here if they remain true in live code:

1. Font-role and style-helper ownership should be narrowed only if a concrete
   boundary can be expressed more clearly than the current `WatchfaceSurfaceStyle`
   contract.
2. Publication work remains operational, not architectural: PBW build, store
   metadata, and release screenshots live outside this ledger unless they force
   a code or contract change.

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
