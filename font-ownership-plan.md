# WatchfaceSurface Refactor And Module Boundary Handoff

## Guiding Principles

- Follow the prescribed cycle: inspect, audit call sites, state the
  patch, review, then edit.
- Keep the patch coherent but not inflated. This is a surface/module
  boundary refactor, not round enablement or AppMessage redesign.
- Public APIs must be declared in headers and implemented as normal
  external functions. Do not make public APIs `static` or `inline`.
- File-local private helpers may remain `static`; they are not APIs.
- `watchface` is the clearing house. `main.c` dispatches platform/service
  events to watchface and should not call feature modules directly.
- Modules own their Pebble layers, buffers, update procs, refresh
  behavior, and destroy paths.
- Layout owns calculated surface data: strata, substrata, color palette,
  fonts, frames, alignment, and optional icon policy.

## Target Architecture

Introduce `WatchfaceSurface` as the top-level calculated UI contract.

- `ColorPalette` lives in `layout.h` and contains colors only.
- `WatchfaceSurface` owns:
  - `ColorPalette*`
  - resolved fonts
  - background frame
  - six fixed strata: date, time, bpm, steps, battery, climate
- Each stratum has known substrata, not a generic dynamic array.
- Text substrata are mandatory and carry frame, alignment, font role, and
  color role.
- Icon substrata carry frame, enabled flag, update-proc requirement, and
  color role.
- Dynamic color roles are explicit. BPM and battery compute actual colors
  in their owning modules from state plus `const ColorPalette*`.

## Module Restructure

- Rename the `weather` module to `climate`.
  Climate owns temperature text, weather condition state, and weather icon
  rendering. Keep AppMessage key names such as `WEATHER_CONDITION`
  unchanged in this slice.
- Break `health` into two modules: `bpm` and `steps`.
  Each module owns its own text layer, optional icon layer, buffers,
  refresh, update proc, and destroy path.
- Keep platform guards explicit. BPM and steps module declarations/calls
  that depend on Pebble Health stay behind `PBL_HEALTH` as needed,
  including non-health stubs if retained.
- Rename `weather_glyphs` to `climate_glyphs` in the same slice and keep
  glyph behavior unchanged.

## API Shape

Composer creates all modules symmetrically:

```c
date_module_create(root, &s_surface);
time_module_create(root, &s_surface);
bpm_module_create(root, &s_surface);
steps_module_create(root, &s_surface);
battery_module_create(root, &s_surface);
climate_module_create(root, &s_surface, s_wf_settings->temp_unit);
```

Refresh APIs also receive the surface:

```c
date_module_refresh(&s_surface);
time_module_refresh(&s_surface, s_wf_settings->time_format);
bpm_module_refresh(&s_surface);
steps_module_refresh(&s_surface);
battery_module_refresh(&s_surface);
climate_module_refresh(&s_surface, s_wf_settings->temp_unit);
```

Rendering helpers that only need colors receive `const ColorPalette*`, not
the full style or surface. For example, climate glyph drawing should take
`const ColorPalette*`.

Watchface gets event-routing wrappers so `main.c` no longer calls feature
modules directly:

```c
watchface_update_battery(&state);
watchface_handle_health_event(event);
watchface_update_temp(celsius_tenths, temp_unit);
watchface_update_weather_condition(condition, temp_unit);
```

## Implementation Sequence

1. Audit current symbols and call sites.
   Confirm all includes, create/refresh/destroy paths, health guards,
   weather glyph calls, and `display.h` references before editing.
2. Update this plan document.
   Replace the old display-owned plan with this surface/module plan.
3. Add surface types in `layout.h`.
   Add `ColorPalette`, font/color roles, text/icon substrata, fixed
   strata, and `WatchfaceSurface`.
4. Move palette and font resolution into `layout.c`.
   Add surface calculation and style update functions. Use compact class
   by shape and baseline: rectangular compact below `200x228`; round
   compact below `260x260`.
5. Convert watchface to own `WatchfaceSurface`.
   Calculate once on create. Update style once on display-mode refresh.
   Pass the surface to every module.
6. Split and rename modules.
   Rename weather to climate. Split health into bpm and steps. Preserve
   behavior, buffers, update procs, and product semantics.
7. Replace display helper usage.
   Move `display_update_text_layer()` to layout as
   `layout_update_text_layer()`. Delete `display.c/.h` only after no
   public symbols or includes remain.
8. Tighten `main.c`.
   Remove direct feature-module includes and calls. Route battery and
   health events through watchface.
9. Validate and review.
   Build all current target platforms: `aplite`, `basalt`, `diorite`,
   `emery`, `flint`. Screenshot Emery and one compact rectangular target.
   Review the full diff for unintended behavior, platform, AppMessage, or
   glyph changes.
10. Commit.
   When validation passes, stage only intended files and create one
   focused commit. Do not commit build artifacts or unrelated scratch
   files.

## Acceptance Criteria

- No `display.h`, `VisualPalette`, `display_get_palette()`, or
  `display_update_text_layer()` remains.
- Composer creates, refreshes, destroys, and routes updates consistently.
- `main.c` no longer calls battery, climate, bpm, or steps modules
  directly.
- Climate replaces weather as the module boundary without changing
  message keys or behavior.
- BPM and steps are separate module boundaries.
- APIs are external functions declared in headers, not `static` or
  `inline`.
- Dynamic BPM and battery colors remain module-owned.
- No round platform support is enabled in this slice.
