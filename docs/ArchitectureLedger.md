# Architecture Ledger

This ledger is the current source of truth for At A Glance runtime architecture.

It describes runtime flow, ownership, boundaries, lifecycle, and source organization. It does not own product principles, visual rules, or current UI tables.

## Required Reading

- README.md
- ProductInvariants.md
- VisualVocabulary.md
- UserInterface.md

## Runtime Flow

The current source uses one watchface runtime with shape-aware layout and platform-aware styling.

Core flow:

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

Notes:
- `watchface_runtime_boundary.c` owns runtime interpretation and repaint-versus-refresh decisions.
- `ataglance.c` owns message receipt, parsing, and dispatch.

## Prepared Surface

At A Glance is organized around a prepared `WatchfaceSurface`.

Layout initialization clears and prepares the caller-owned surface from scratch. Feature modules then consume prepared substrata, frames, fonts, and palettes instead of recalculating layout policy for themselves.

Blueprints are immutable product choices. Calculated layout is the resolved geometry used to place live strata on the surface.

## Architect

- Owns geometry and compact/full classification.
- Resolves compact/full once and stores the result on `WatchfaceSurfaceStyle.is_compact`.
- Responsible for platform-specific geometry. It does not assume that one device class can be scaled blindly into another.

## Stylist

- Consumes `WatchfaceSurfaceStyle.is_compact`.
- `layout_watchface_update_palette()` resolves palette and font selection.
- Owns display-mode styling decisions. It does not own live module state such as BPM zone, battery charge condition, or climate condition colors.

## Watchface Runtime

- `ataglance.c` owns Pebble lifecycle, services, persistent settings, AppMessage parsing, and transport dispatch.
- `watchface_runtime_boundary.c` interprets runtime events, mutates settings and domain state, and decides repaint versus refresh.
- `watchface.c` owns the live `WatchfaceSurface`, feature-module lifecycle, and visual dispatch.
- `watchface_apply_received_data()` is the single runtime ingress for watchface updates.

## Module Ownership

Responsibilities of feature modules:
- own Pebble layer lifecycle and source state
- `create()` APIs receive only the prepared substrata, frames, and fonts they need
- `refresh()` APIs receive the current palette and narrow runtime payloads where needed
- must not retain `WatchfaceSurface*`

Required layer or resource creation gates module success and retained state. Optional resources may fail only when that failure is explicitly non-fatal and cleaned up correctly.

## Palette Resolution

- Static palette roles come from the stylist.
- Dynamic colors remain module-owned when they depend on live source state, such as BPM, battery, steps, or climate condition state.
- Module-specific palette policy belongs to the owning feature module.
- Bitmap palette mutation helpers require palettized PNG resources.

## Capability Guards

- Treat color, health, sensors, screen shape, resources, and phone data as optional unless verified for the target platform.
- Prefer `PBL_COLOR`, `PBL_BW`, `PBL_HEALTH`, `PBL_RECT`, `PBL_ROUND`, and `PBL_API_EXISTS()`.
- Do not assume that a supported target has every capability that a neighboring target has.

## Transport And AppMessage

`ataglance.c` is the Pebble container and transport/service adapter.

The phone companion requests current weather from Open-Meteo every 30 minutes, uses phone geolocation when available, and falls back to Oakland, CA when location is unavailable.

Temperature is sent to the watch in Celsius tenths and rendered according to settings. `weather_code` and `is_day` are sent to C for glyph selection.

Runtime transport completeness is evaluated before climate state is applied. Incomplete or invalid weather updates clear stale weather state and render the unavailable vocabulary.

Do not guess lifecycle, AppMessage, generated resource, or SDK behavior. Verify it.

## Lifecycle

Safety comes first.

Do not leave stale pointers, partial layer ownership, unmatched resource lifetimes, buffer overflows, or hidden failure branches.

Architecture review comes before non-trivial edits. Keep code changes small and coherent. Prefer direct code over helper churn when the boundary is obvious and local.

## Header Boundaries

Public headers expose only the concepts callers need.

Final header split:

- `watchface.h`
- `watchface_components.h`
- `layout_surface.h`
- `layout.h`
- `layout_design.h`
- `watchface_debug.h`

Feature module headers must not include `layout.h`.

## Debug And Diagnostic Guardrails

`ATAGLANCE_DEBUG` is a watchface-specific debug build gate, not a product mode.

Current contract:

- `src/modules/watchface_debug.h` owns debug-gate normalization only, with `wscript` as the build-time enable point.
- Debug ingress and debug-only declarations flow through `watchface_debug.h`.
- `watchface.h` owns public transport key definitions that must be visible at runtime, including debug transport keys when they are part of ingress handling.
- `watchface_debug.h` remains dedicated to the debug flag and debug ingress only.

Debug-hook policy:

- Treat debug hooks as narrow transport or render test hooks layered on top of normal runtime behavior.
- Normal runtime data is used when no debug packet is queued.
- Debug state defaults to off and is cleared during module teardown.

`APP_LOG` policy:

- `APP_LOG(APP_LOG_LEVEL_DEBUG, ...)` is allowed only for narrow diagnostics that materially help runtime investigation.
- `APP_LOG(APP_LOG_LEVEL_INFO, ...)` should be treated as temporary bring-up scaffolding.
- Delete all `INFO` logs before commit.
- Keep durable logs at `WARNING` or `ERROR` when they protect failure diagnosis or transport validation.
- Avoid noisy logging in hot paths or steady-state rendering.

Review rule:

- If a debug change needs new persistent runtime branching, new state ownership, or a new mode concept, stop and review the architecture before coding.

## Source Organization

```text
resources/
src/c/
src/modules/
src/pkjs/
package.json
wscript
```

Current source map:

- `src/c/ataglance.c`: Pebble app lifecycle, window ownership, service subscriptions, settings load/save, AppMessage parsing, and dispatch into the watchface runtime
- `src/modules/watchface.c`: watchface runtime clearing house, live `WatchfaceSurface` owner, module create/destroy order, repaint, refresh, and lifecycle cleanup
- `src/modules/watchface.h`: public watchface runtime ingress, update masks, event data, and runtime-visible transport key definitions
- `src/modules/watchface_debug.h`: debug-gate normalization and debug ingress declarations only
- `src/modules/watchface_runtime_boundary.c`: runtime event interpretation, settings mutation, weather ingress application, debug health ingress application, and repaint-versus-refresh decisions
- `src/modules/watchface_components.h`: shared display primitives, palettes, strata, roles, and reusable watchface component vocabulary
- `src/modules/layout.h`: public layout facade for surface initialization, palette updates, and font lifecycle
- `src/modules/layout_surface.h`: assembled `WatchfaceSurface` and style output contract
- `src/modules/layout_design.h`: private layout design constants, blueprints, and calculated-layout vocabulary
- `src/modules/layout_architect.c`: geometry provider, blueprint selection, compact/full classification, and prepared-surface assembly
- `src/modules/layout_stylist.c`: palette resolution, font-role selection, custom-font load/unload, and display-mode styling
- `src/modules/substratum_renderer.c/.h`: shared text/icon layer setup, text updates, color-role lookup, glyph primitives, and small rendering helpers
- `src/modules/helper.c/.h`: shared utility helpers and macros with no feature-module ownership
- `src/modules/settings.c/.h`: defaults, persistence, validation, and heart-rate sample-period mapping
- `src/modules/date.c/.h`: date layer lifecycle and date-text refresh
- `src/modules/time.c/.h`: time layer lifecycle, formatting, and custom-font use
- `src/modules/climate.c/.h`: climate source state, weather availability handling, temperature text, and climate icon lifecycle
- `src/modules/climate_glyphs.c/.h`: Open-Meteo weather-code mapping and procedural weather glyph rendering
- `src/modules/battery.c/.h`: battery source state, track/fill/bolt rendering, and battery refresh
- `src/modules/bpm.c/.h`: BPM source state, health reads, BPM text/icon refresh, and debug BPM override handling
- `src/modules/steps.c/.h`: steps source state, health reads, steps text/icon/progress refresh, and debug steps override handling
- `src/pkjs/index.js`: Clay bootstrap, geolocation, Open-Meteo weather fetch, fallback location handling, request sequencing, and weather AppMessage sends
- `package.json`: app manifest, target platforms, capabilities, message keys, and resources
- `wscript`: Pebble build definition, source globs, JS bundling, and optional `ATAGLANCE_DEBUG` build define
