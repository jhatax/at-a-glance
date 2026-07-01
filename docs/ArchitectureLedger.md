# Architecture Ledger

This ledger is the current source of truth for At A Glance runtime architecture.

It describes runtime flow, ownership, boundaries, lifecycle, and source organization. It does not own product principles, visual rules, or current UI tables.

## Required Reading

- ../README.md
- ProductInvariants.md
- VisualVocabulary.md
- UserInterface.md

## Runtime Flow

The current source uses one watchface runtime with shape-aware layout and platform-aware styling.

Core flow:

```text
ataglance.c
  -> settings_load(&settings)
  -> window_set_window_handlers(...)
  -> window_stack_push(window, true)
       -> main_window_load(window)
            -> watchface_create(window, &settings)
                 -> window_get_root_layer(window)
                 -> layout_watchface_initialize(width, height, &surface)
                      -> memset(surface, 0, sizeof(*surface))
                      -> calculate active blueprint and final geometry
                      -> store compact/full on surface.style.is_compact
                 -> layout_watchface_update_palette(&surface.style, display_mode)
                 -> window_set_background_color(...)
                 -> layout_watchface_initialize_fonts(...)
                 -> layout_watchface_load_custom_fonts(...)
                 -> feature_module_create(root, prepared surface strata...)
                 -> require date/time/battery strata to succeed
                 -> watchface_refresh(WATCHFACE_UPDATE_ALL_STRATA)
  -> subscribe tick, battery, and health services
  -> register AppMessage callbacks
  -> open_app_message()
```

Runtime event flow:

```text
service callback or AppMessage callback in ataglance.c
  -> build WatchfaceEventData
  -> watchface_apply_received_data()
       -> apply_setting_data(...)
            -> mutate settings for time format, temp unit, display mode, steps goal
            -> accumulate targeted refresh mask
            -> mark repaint only for a valid display-mode change
       -> apply_weather_data(...)
            -> push complete or unavailable climate state
            -> request climate refresh when any weather field was received
       -> apply_service_event_data(...)
            -> translate tick, battery, and health callbacks into refresh masks
       -> apply_health_setting_data(...)
            -> mutate HR sample setting only
       -> apply_debug_health_data(...) in debug builds
            -> queue one-shot BPM/steps overrides
            -> request health refresh
       -> if repaint
            -> watchface_repaint()
                 -> layout_watchface_update_palette(&surface.style, display_mode)
                 -> window_set_background_color(...)
                 -> watchface_refresh(WATCHFACE_UPDATE_ALL_STRATA)
       -> else if refresh mask != WATCHFACE_UPDATE_NONE
            -> watchface_refresh(mask)
                 -> refresh only the addressed strata that were created
```

Settings persistence and side effects:

```text
ataglance.c inbox_received_callback(iter)
  -> copy previous_settings
  -> parse settings, weather, health settings, and debug health tuples
  -> watchface_apply_received_data(&data, &settings)
  -> if any persisted setting changed
       -> settings_save(&settings)
  -> if hr_sample_minutes changed
       -> apply_hr_sample_period()
```

Notes:
- `watchface_runtime_boundary.c` owns runtime interpretation and repaint-versus-refresh decisions.
- `ataglance.c` owns message receipt, parsing, and dispatch.
- `watchface.c` owns palette application, full repaint, and per-stratum refresh dispatch after the runtime boundary decides what changed.

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

## Selective Watchface Refresh Strategy

- The runtime prefers targeted refresh over whole-watchface redraw.
- Incoming events are translated into narrow update masks so only affected strata refresh.
- `watchface.c` dispatches those masks to the owning modules instead of repainting unrelated layers.
- Full repaint is reserved for style-wide changes such as display-mode transitions.
- This selective-refresh discipline, along with other narrow runtime decisions, exists to avoid unnecessary work and help maximize time between charges.

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

Any received weather field triggers a climate refresh. Only a fully parsed temperature/condition/`is_day` triplet is applied as complete climate state; incomplete or invalid weather payloads fall back to the unavailable vocabulary.

Note: Verify lifecycle, AppMessage, generated resource, or SDK behaviors.

## Lifecycle

Safety comes first.

Do not leave stale pointers, partial layer ownership, unmatched resource lifetimes, buffer overflows, or hidden failure branches.

Architecture review comes before non-trivial edits. Keep code changes small and coherent. Prefer direct code over helper churn when the boundary is obvious and local.

## Header Boundaries

Public and shared headers expose only the concepts cross-module callers need. This section documents the architecture-wide shared header split, not every feature-module header. Shared header split:

- `watchface.h`: runtime ingress, update masks, event data, and runtime-visible transport keys
- `watchface_components.h`: reusable watchface display primitives built on shared layout/style vocabulary
- `layout.h`: public layout facade
- `layout_surface.h`: shared layout geometry/design vocabulary and calculated layout structures
- `layout_style.h`: shared style vocabulary, including font roles, color roles, palettes, and font bookkeeping
- `watchface_debug.h`: debug-gate normalization only

Feature-module headers such as `battery.h`, `climate.h`, `time.h`, `steps.h`, and `bpm.h` remain narrow module-local API surfaces. They are expected to consume the shared header vocabulary above, not expand the architecture-wide boundary set.

Feature module headers must not include `layout.h`.

## Debug And Diagnostic Guardrails

`ATAGLANCE_DEBUG` is a watchface-specific debug build gate, not a product mode.

Current contract:

- `src/modules/watchface_debug.h` owns debug-gate normalization only, with `wscript` as the build-time enable point.
- `watchface.h` owns runtime-visible debug transport keys and debug event fields when they participate in ingress handling.
- `watchface_runtime_boundary.c` applies debug health payloads as one-shot health refresh overrides.
- `watchface_debug.h` remains dedicated to the debug flag only; it is not a home for helper APIs, product flags, layout policy, or runtime behavior.

Debug-hook policy:

- Treat debug hooks as narrow transport or render test hooks layered on top of normal runtime behavior.
- Normal runtime data is used when no debug packet is queued.
- Debug state defaults to off and queued debug health overrides are cleared during watchface teardown.

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
- `src/modules/watchface_debug.h`: debug-gate normalization only
- `src/modules/watchface_runtime_boundary.c`: runtime event interpretation, settings mutation, weather ingress application, debug health ingress application, and repaint-versus-refresh decisions
- `src/modules/watchface_components.h`: shared display primitives, palettes, strata, roles, and reusable watchface component vocabulary
- `src/modules/layout.h`: public layout facade for surface initialization, palette updates, and font lifecycle
- `src/modules/layout_surface.h`: shared layout geometry/design vocabulary and calculated layout structures
- `src/modules/layout_style.h`: shared style vocabulary, including font roles, color roles, palettes, and font bookkeeping
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

## Further Reading

- `Contributing.md` for contributor workflow, validation, and review discipline