# Watchface Surface Ownership Handoff

This document records the plan that led to the WatchfaceSurface refactor and
the implementation findings after the code landed. Keep it as the future
reference for font, color, layout, and module ownership.

Implemented in:

- `5895fcd Refactor watchface surface ownership`
- `aec3981 Rename watchface coordinator module`

## Core Principles

- Follow the prescribed cycle: inspect current symbols, audit call sites,
  state the patch, review the ramifications, then edit.
- Keep boundaries meaningful. This was a surface and module ownership
  refactor, not round enablement, AppMessage redesign, or visual redesign.
- Public APIs are declared in headers and implemented as normal external
  functions. Do not make public APIs `static` or `inline`.
- File-local private helpers may remain `static`; they are not APIs.
- `watchface` is the runtime clearing house. `main.c` owns Pebble lifecycle,
  settings/AppMessage parsing, service subscriptions, and dispatch entry
  points, then hands runtime events to `watchface`.
- Feature modules own their Pebble layers, buffers, update procs, refresh
  behavior, state-derived colors, and destroy paths.
- `layout` owns calculated surface data: frames, strata, substrata, color
  palette, font roles, resolved fonts, alignment, and icon policy.
- Text is the primary glance surface. If a metric text layer cannot be
  created, do not create or depend on its icon. Text-only is acceptable;
  icon-only is not.
- Store source state, not redundant render state, unless a derived value is
  expensive or impossible to recompute.

## Current Ownership Model

`WatchfaceSurface` is the top-level calculated UI contract. It lives in
`layout.h` and is filled by `layout_calculate_surface()`.

`WatchfaceSurface` contains:

- Face dimensions and background frame.
- Shared content metrics such as content width, row gap, and column gap.
- One `WatchfaceSurfaceStyle`.
- Six fixed product strata: date, time, BPM, steps, battery, and climate.
- One horizontal rule substratum.

`WatchfaceSurfaceStyle` contains:

- `const ColorPalette* palette`
- one resolved `GFont` for every `WatchfaceFontRole`

`ColorPalette` lives in `layout.h` and contains colors only. Callers that
only need colors receive `const ColorPalette*`, not the full surface.

Each stratum has known substrata, not a generic dynamic array:

- Date: one text substratum.
- Time: one text substratum.
- BPM: icon substratum plus text substratum.
- Steps: icon substratum plus text substratum.
- Battery: icon substratum plus text substratum.
- Climate: icon substratum plus text substratum.

Text substrata carry frame, alignment, font role, and color role. Icon
substrata carry frame, enabled flag, update-proc requirement, and color role.

Dynamic color roles are explicit. BPM, steps, battery, and climate compute
actual render colors in their owner modules from source state plus
`const ColorPalette*`.

## Runtime Flow

`main.c` calls `watchface_create(window, &s_settings)` after the main window
has a root layer. From there, `watchface.c` owns the active runtime state:

- `Window*`
- `const WatchfaceSettings*`
- one static `WatchfaceSurface`
- a layer-created bit mask for cleanup and optional-module refresh checks

Creation flow:

```c
watchface_create(window, settings);
  layout_calculate_surface(width, height, display_mode, &s_surface);
  date_module_create(root, &s_surface);
  time_module_create(root, &s_surface);
  battery_module_create(root, &s_surface);
  climate_module_create(root, &s_surface, settings->temp_unit);
  bpm_module_create(root, &s_surface);    // PBL_HEALTH only
  steps_module_create(root, &s_surface);  // PBL_HEALTH only
```

Refresh flow:

```c
watchface_refresh();
  layout_update_surface_style(&s_surface, settings->display_mode);
  window_set_background_color(window, s_surface.style.palette->background);
  date_module_refresh(&s_surface);
  time_module_refresh(&s_surface, settings->time_format);
  battery_module_refresh(&s_surface);
  climate_module_refresh(&s_surface, settings->temp_unit);
  bpm_module_refresh(&s_surface);    // PBL_HEALTH only
  steps_module_refresh(&s_surface);  // PBL_HEALTH only
```

Event routing:

```c
watchface_handle_tick(units_changed);
watchface_update_battery(&state);
watchface_update_temp(celsius_tenths, temp_unit);
watchface_update_weather_condition(condition, temp_unit);
watchface_handle_health_event(event);  // PBL_HEALTH only
```

Destroy flow:

```c
watchface_destroy();
  date_module_destroy();
  time_module_destroy();
  battery_module_destroy();
  climate_module_destroy();
  bpm_module_destroy();    // PBL_HEALTH only
  steps_module_destroy();  // PBL_HEALTH only
  memset(&s_surface, 0, sizeof(s_surface));
```

The destroy path uses the created-layer mask so partially created optional
modules are cleaned up without assuming all modules exist.

## Module Structure

The old `watchface_composer` boundary is now `watchface`.

Current feature modules:

- `date`: date text layer, date buffer, uppercase date formatting, refresh,
  and destroy.
- `time`: time text layer, time buffer, time-format rendering, refresh, and
  destroy.
- `bpm`: BPM text layer, heart icon layer, BPM buffer, BPM source state,
  health event handling, BPM color derivation, refresh, and destroy.
- `steps`: steps text layer, paw icon layer, steps buffer, steps source
  state, health event handling, steps color derivation, refresh, and destroy.
- `battery`: battery text layer, battery icon layer, battery source state,
  battery color derivation, refresh, update proc, and destroy.
- `climate`: temperature text layer, weather condition state, weather icon
  layer, temperature formatting, refresh, update proc, and destroy.
- `climate_glyphs`: procedural weather glyph rendering from explicit
  condition, frame, and `const ColorPalette*` inputs.

The old `health` module was split into `bpm` and `steps`. The old `weather`
module was renamed to `climate`. AppMessage keys were intentionally left
unchanged, including `WEATHER_CONDITION`.

## Layout And Font Decisions

Compact classification is based on actual face dimensions, not platform name.

- Rectangular displays are compact when width is below `200` or height is
  below `228`.
- Round displays are reserved for future work and currently use a compact
  threshold below `260x260` if a round branch is introduced.
- This slice did not enable Chalk or Gabbro.

Font roles are explicit even when product choices currently map several roles
to the same Pebble system font:

- `WATCHFACE_FONT_ROLE_DATE`
- `WATCHFACE_FONT_ROLE_TIME`
- `WATCHFACE_FONT_ROLE_BPM`
- `WATCHFACE_FONT_ROLE_STEPS`
- `WATCHFACE_FONT_ROLE_BATTERY`
- `WATCHFACE_FONT_ROLE_CLIMATE`

Full rectangular font mapping:

- Date: `FONT_KEY_GOTHIC_18_BOLD`
- Time: `FONT_KEY_BITHAM_42_BOLD`
- BPM: `FONT_KEY_GOTHIC_18`
- Steps: `FONT_KEY_GOTHIC_18`
- Battery: `FONT_KEY_GOTHIC_18_BOLD`
- Climate: `FONT_KEY_GOTHIC_18`

Compact rectangular font mapping:

- Date: `FONT_KEY_GOTHIC_14_BOLD`
- Time: `FONT_KEY_BITHAM_30_BLACK`
- BPM: `FONT_KEY_GOTHIC_14`
- Steps: `FONT_KEY_GOTHIC_14`
- Battery: `FONT_KEY_GOTHIC_14_BOLD`
- Climate: `FONT_KEY_GOTHIC_14`

Exact computed rectangular geometry is documented in `README.md` for Emery
`200x228` and compact `144x168` devices.

## Implemented Findings

- `display.c` and `display.h` were removed. Palette selection and text-layer
  updates moved into `layout`.
- `VisualPalette`, `display_get_palette()`, and
  `display_update_text_layer()` were removed.
- The correct helper boundary is `layout_update_text_layer()`, because text
  layer styling depends on layout-owned palette and surface decisions.
- `main.c` no longer includes or calls feature modules directly.
- Feature modules receive `const WatchfaceSurface*` for create/refresh where
  they need frame, font, alignment, or palette data.
- Glyph rendering that only needs color receives `const ColorPalette*`.
- Health-related declarations and calls remain behind `PBL_HEALTH`.
- Battery and BPM color changes remain module-owned, not palette-owned.
- Steps icon color is still palette-owned for the normal product color, but
  unavailable and dynamic render states remain module-derived.
- Climate owns temperature text, weather condition state, and weather icon
  rendering. It still preserves the existing phone/AppMessage contract.
- The watchface refresh path updates the palette pointer and resolved fonts
  once through `layout_update_surface_style()` before refreshing modules.
- The tick path updates only time and date, preserving existing behavior.
- Optional modules are guarded during refresh/destroy by the created-layer
  mask. Required modules are date, time, and battery.

## Future Round Work

Round support still needs a separate layout plan before enabling `chalk` or
`gabbro`. Do not scale the rectangular layout wholesale and call it round
support.

A round plan must account for:

- circular clipping by row position
- row-specific available width
- date and time text extremes
- health-row and bottom-row differences
- icon legibility near curved edges
- color and monochrome variants
- platform capabilities and missing sensors

The current surface model prepares for round work by making each product
stratum explicit. A future `PBL_ROUND` layout branch should calculate the
same six strata with round-safe frames instead of changing module ownership.

## Validation Notes

The code refactor was validated with:

- `pebble build`
- Emery emulator install and screenshot
- Aplite compact rectangular emulator install and screenshot

The current README now records the computed geometry for both full
rectangular `200x228` and compact rectangular `144x168` displays. Build
artifacts and screenshots should not be committed unless they are selected
for publishing.

## Guardrails

- Do not reintroduce `watchface_composer`; the public boundary is
  `watchface`.
- Do not reintroduce `display.h`, `VisualPalette`, `display_get_palette()`,
  or `display_update_text_layer()`.
- Do not add generic substratum arrays unless there is a concrete product
  need. The six strata are known and fixed.
- Do not make `main.c` pass window, settings, palette, or surface state
  through direct module calls.
- Do not rename AppMessage keys as part of module naming cleanup.
- Do not add round platforms to `package.json` until the separate round plan
  is reviewed.
