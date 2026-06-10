# Layout Restructuring Plan
**Status: Complete.**

This document records the completed layout restructuring that prepared the
rectangular watchface for future round layout work. The scope was boundary
cleanup and ownership realignment, not enabling Chalk or Gabbro.

## Completed Outcome

The layout/watchface architecture is now split along the intended boundaries:

- `watchface` is the runtime clearing house.
- feature modules own Pebble layer creation, update procs, refresh behavior,
  source state, and destroy paths.
- `layout` calculates the display contract and exposes only public layout
  APIs.
- `layout_stylist.c/.h` privately owns palette and typography decisions.
- `layout_rect.c/.h` privately owns rectangular geometry.
- `watchface_components.h` owns shared display component types.
- `substratum_renderer.c/.h` owns common Pebble rendering helpers for
  calculated substrata.

The refactor is complete for the current rectangular watchface. Round support
remains a separate future design and implementation effort.

## Public And Private Boundaries

### `src/c/ataglance.h`

Owns product constants and compile-time product decisions:

- debug flag
- design face dimensions
- icon, margin, row, column, and text sizing constants
- compact/full font key choices
- string limits
- persisted-settings toggle

It must not own Pebble layer helpers, module lifecycle APIs, or runtime
watchface orchestration.

### `src/modules/watchface_components.h`

Owns the shared display component model:

- `WATCHFACE_UNAVAILABLE_TEXT`
- `WatchfaceFontRole`
- `WatchfaceColorRole`
- `ColorPalette`
- `WatchfaceTextSubstratum`
- `WatchfaceIconSubstratum`
- `WatchfaceBackgroundSubstratum`
- fixed stratum structs
- `WatchfaceSurfaceStyle`
- `WatchfaceSurface`

This header is intentionally type-focused. It is not a dumping ground for
settings APIs, layout functions, renderer functions, or feature-module
lifecycle declarations.

### `src/modules/layout.h`

The public layout API is now narrow:

```c
void layout_calculate_surface(
    int16_t face_width,
    int16_t face_height,
    uint8_t display_mode,
    WatchfaceSurface* surface);

void layout_update_surface_style(
    WatchfaceSurface* surface,
    uint8_t display_mode);
```

`layout.h` includes `watchface_components.h` because layout calculates a
`WatchfaceSurface`. It does not expose text-layer helpers, icon scaling
helpers, color-role lookup, or private architect/stylist APIs.

### `src/modules/layout.c`

`layout.c` is the layout facade:

- clears caller-owned `WatchfaceSurface` storage
- dispatches rectangular geometry calculation under `PBL_RECT`
- applies style through the stylist
- updates style when display mode changes

Only layout implementation files should include private layout headers.

### `src/modules/layout_stylist.c/.h`

The stylist is private to layout and owns visual decisions:

- dark/light `ColorPalette` constants
- display-mode palette selection
- compact/full display classification
- `WatchfaceFontRole` to Pebble system font key mapping
- custom font resource selection for time
- filling `WatchfaceSurfaceStyle`

Current compact rule:

- round: compact below `260x260`
- rectangular: compact below `200x228`

Current palette implementation is the Shinkansen palette captured in
`palette-options.md`.

### `src/modules/layout_rect.c/.h`

The rectangle architect is private to layout and owns rectangular geometry.
It fills final frames on each substratum instead of leaking intermediate row
math into `WatchfaceSurface`.

Current rectangular layout flow:

- derive private `LayoutRectMetrics` from face width/height
- assign the full background frame and horizontal rule
- assign date and time text frames centered around the rule
- assign battery/climate top-row icon and text frames
- assign steps/BPM bottom-row icon and text frames

Durable rule: layout is not a generic row engine. The product strata remain
fixed and known, and the architect assigns each substratum directly.

### `src/modules/substratum_renderer.c/.h`

The renderer helper owns common Pebble runtime operations for calculated
substrata:

```c
TextLayer* substratum_renderer_create_text_layer(
    Layer* parent,
    const WatchfaceTextSubstratum* text,
    GFont font);

Layer* substratum_renderer_create_icon_layer(
    Layer* parent,
    const WatchfaceIconSubstratum* icon,
    LayerUpdateProc update_proc);

void substratum_renderer_update_text_layer(
    TextLayer* layer,
    const char* text,
    GColor text_color);

GColor substratum_renderer_color_for_role(
    const ColorPalette* palette,
    WatchfaceColorRole role);
```

It also owns icon design-coordinate scaling helpers. This keeps feature
modules from duplicating Pebble layer setup while preserving module ownership
of actual layer pointers and destroy paths.

The renderer does not own:

- module source state
- text formatting
- dynamic color decisions
- icon update procs
- service/AppMessage data
- glyph behavior decisions

### `src/modules/watchface.c/.h`

`watchface` owns the live runtime surface and coordinates all rendering:

- creates the calculated `WatchfaceSurface`
- creates modules explicitly
- tracks created strata with `s_strata_created_mask`
- destroys only strata that were actually created
- accepts source-state setters from `main.c`
- dispatches `watchface_refresh(WatchfaceUpdateMask updates)`
- handles display-mode style recalculation inline in the display-mode branch
  of `watchface_refresh()`
- keeps the strata-only redraw mask private inside `watchface.c`

`main.c` should not know that climate, battery, BPM, steps, time, date, or
background are separate modules. It should only know that events imply
watchface update categories.

## Watchface Refresh Flow

The implemented flow uses one public render dispatcher:

```c
void watchface_refresh(WatchfaceUpdateMask updates);
```

Current update categories:

```c
WATCHFACE_UPDATE_TIME
WATCHFACE_UPDATE_DATE
WATCHFACE_UPDATE_BATTERY
WATCHFACE_UPDATE_CLIMATE
WATCHFACE_UPDATE_HEALTH
WATCHFACE_UPDATE_DISPLAY_MODE
WATCHFACE_UPDATE_ALL
```

`WATCHFACE_UPDATE_DISPLAY_MODE` is handled inside `watchface_refresh()`. It:

- updates the surface style once
- updates the window background color
- refreshes the background layer if it was created
- replaces the refresh mask with a private strata-only mask so
  `WATCHFACE_UPDATE_DISPLAY_MODE` is not retained during module dispatch

The strata-only mask is intentionally not part of `watchface.h`; `main.c`
should not know about strata. `WATCHFACE_UPDATE_ALL` remains public for initial
full refreshes and includes display mode plus all watchface update categories.

Source-state setters are intentionally separate from rendering:

```c
void watchface_set_temperature(int celsius_tenths);
void watchface_set_weather_condition(int weather_condition);
```

Debug health setters are guarded by `DEBUG_ATAGLANCE` and `PBL_HEALTH`:

```c
void watchface_debug_set_bpm(int bpm);
void watchface_debug_set_steps(int steps);
void watchface_debug_clear_health(void);
```

Setters may update module-owned source state, but they must not update Pebble
layers directly. Rendering occurs only through a later `watchface_refresh()`.
Debug health overrides are one-shot when consumed by refresh, and may also be
cleared explicitly through `watchface_debug_clear_health()`.

## Module Lifecycle

Feature modules own their own substrata lifecycle:

- create required text layers from calculated substrata
- optionally create icon layers from calculated icon substrata
- keep module-owned source state and buffers
- refresh only their own text/icon surfaces
- destroy only layers they created

Creation success follows product semantics:

- required text layer creation determines module success for text-bearing
  modules
- icon creation is optional unless a module explicitly makes it required
- watchface records success in `s_strata_created_mask`
- destroy paths use that mask as the source of truth

## Rectangular Visual Baseline

The current rectangular face uses:

- top row: battery on the left, climate on the right
- centered time above the rule
- horizontal rule through the face midpoint
- centered date below the rule
- bottom row: steps on the left, BPM on the right

Each text and icon substratum has its own final `GRect`. Intermediate metrics
such as content bounds, row gaps, column gaps, and pair widths stay private to
`layout_rect.c`.

## Completed Implementation Sequence

The completed sequence was:

1. Extracted palette/font decisions into `layout_stylist.c/.h`.
2. Extracted rectangular geometry into `layout_rect.c/.h`.
3. Moved shared component types into `watchface_components.h`.
4. Moved text-layer creation/update, icon-layer creation, color-role lookup,
   and icon scaling into `substratum_renderer.c/.h`.
5. Reduced `layout.h` to the layout calculation/style APIs.
6. Converted feature modules to create their own text/icon layers from their
   calculated substrata.
7. Kept feature modules as owners of buffers, source state, update procs,
   refresh behavior, and destroy paths.
8. Centralized render dispatch through `watchface_refresh()`.
9. Added watchface source-state setters for climate and debug health values.
10. Made debug BPM/steps injections one-shot and explicitly reset their
    module statics.
11. Initialized file-scope static module state explicitly instead of relying
    on implicit C zero initialization.

## Non-Goals Preserved

- Chalk and Gabbro are not enabled.
- No round geometry has been added.
- Pebble layer ownership remains in feature modules.
- There is no generic dynamic stratum array.
- Watchface module creation is not hidden behind a registry.
- AppMessage keys were not changed for this refactor.
- Glyph behavior was not changed by the layout restructuring.

## Validation Performed During The Restructure

Across the completed slices, validation included:

- `git diff --check`
- `pebble build`
- emulator install/screenshot checks for visual slices where requested
- call-site audits for layout helpers, renderer helpers, module APIs, and
  watchface refresh paths

## Future Work

Round support remains the next architectural step. It should be implemented as
a deliberate `layout_round.c/.h` architect, private to layout, and should not
scale the rectangular layout wholesale.

Round planning must account for:

- circular clipping at each row's y-position
- row-specific safe spans
- icon legibility near curved edges
- compact/full typography decisions for Chalk and Gabbro
- color and monochrome display behavior
- missing sensors or health data

Any future refresh-mask cleanup, such as renaming display-mode updates to a
palette/style-specific update category, should be handled as a separate
watchface runtime slice rather than folded into layout restructuring.
