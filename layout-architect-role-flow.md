# Layout Architect Role And Flow

This document captures the current layout/watchface boundary and the next
layout refactor direction after the stylist, rectangle architect, component,
and substratum renderer extractions. It is a handoff for future rectangle and
round layout architect work.

## Current State

- `layout.h` is the only public layout contract.
- `layout.c` is the public facade for the layout module.
- `layout_stylist.c/.h` is private to layout and owns palette/font style
  resolution.
- `layout_rect.c/.h` is private to layout and owns rectangular geometry.
- `watchface_components.h` defines the display component types shared by
  layout, watchface, renderer helpers, and feature modules.
- `substratum_renderer.c/.h` owns common text/icon layer creation, text-layer
  updates, color-role resolution, and icon coordinate scaling.
- Feature modules own Pebble layer creation, update procs, refresh, and
  destroy.
- `watchface` remains the clearing house for module creation, refresh,
  destroy, source-state intake, and mask-based render dispatch.

The next major layout step is round architecture. Rectangle geometry is already
private to layout, but future visual redesigns should keep the same ownership
model: layout calculates a `WatchfaceSurface`; modules render their own strata
from that surface; watchface coordinates lifecycle and refresh.

## Primary Drivers

Three files define the top-level product/runtime relationship:

- `src/c/ataglance.h`
  - product constants and compile-time decisions
  - debug flag
  - design dimensions, spacing constants, font-key choices, and string limits
  - no Pebble layer ownership and no runtime rendering

- `src/c/main.c`
  - Pebble app lifecycle
  - service subscriptions
  - AppMessage parsing
  - settings persistence
  - local update-mask accumulation
  - calls only watchface runtime APIs for display behavior

- `src/modules/watchface.c`
  - creates the calculated surface
  - creates/destroys feature modules
  - owns the created-strata mask
  - accepts source-state setters from `main.c`
  - dispatches `watchface_refresh(WatchfaceUpdateMask updates)`

`main.c` should not know that climate, battery, BPM, steps, time, date, or
background are separate modules. It knows only that a watchface exists and
that events imply update categories.

## Watchface Refresh Contract

`watchface_refresh(WatchfaceUpdateMask updates)` is the only render
dispatcher. Setter-style APIs mutate source state only.

Current watchface-level source-state setters:

```c
void watchface_set_temperature(int celsius_tenths);
void watchface_set_weather_condition(int weather_condition);
```

Debug-only health setters are separate and guarded by both `DEBUG_ATAGLANCE`
and `PBL_HEALTH`:

```c
void watchface_debug_set_bpm(int bpm);
void watchface_debug_set_steps(int steps);
void watchface_debug_clear_health(void);
```

These setters may redirect to module-owned setters, but neither watchface
setters nor module setters may update Pebble layers directly. Rendering occurs
only when `watchface_refresh()` receives a mask. Debug health overrides are
one-shot when consumed by refresh and can also be cleared explicitly.

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

`WATCHFACE_UPDATE_DISPLAY_MODE` is special. It updates surface style once,
updates the window/background color, refreshes the background layer if it was
created, then replaces the dispatch mask with a private strata-only redraw
mask. That private mask lives in `watchface.c`, not `watchface.h`, because
`main.c` should not know about strata.

## Surface, Strata, And Substrata

`WatchfaceSurface` is the calculated UI contract. It is not Pebble layer
state. It contains:

- face dimensions
- one `WatchfaceSurfaceStyle`
- one background substratum
- six fixed product strata: date, time, bpm, steps, battery, climate

The current strata are fixed and known. Do not replace them with a generic
dynamic strata array.

Text substrata carry:

- final frame
- text alignment
- font role
- color role

Icon substrata carry:

- final frame
- enabled flag

Feature modules consume their own substrata and own their actual Pebble
`Layer`/`TextLayer` lifecycle.

## Layout Flow

The layout module owns calculation only:

```text
layout_calculate_surface()
  -> layout architect fills geometry
  -> layout stylist fills palette/font/custom-font decisions
  -> WatchfaceSurface is complete
```

The layout module does not create Pebble layers, subscribe to services, parse
AppMessage tuples, or render module source state.

## Architect Responsibility

Layout architects calculate geometry only. They fill substrata in
`WatchfaceSurface`.

Architects do not:

- create Pebble `Layer` or `TextLayer` objects
- set update procs
- call feature modules
- decide runtime data availability
- change AppMessage behavior
- own palette or font resolution

Architects assign each substratum's actual frame:

- `surface->background`
- `surface->date.text`
- `surface->time.text`
- `surface->battery.icon`
- `surface->battery.text`
- `surface->climate.icon`
- `surface->climate.text`
- `surface->steps.icon`
- `surface->steps.text`
- `surface->bpm.icon`
- `surface->bpm.text`

The important design rule is that layout is not a generic row engine. The
known product strata remain fixed. The architect calculates where each
substratum lives.

## Private File Shape

Keep only `layout.h` public.

Private implementation files:

- `layout.c`
  - public facade
  - owns `layout_calculate_surface()`
  - owns `layout_update_surface_style()`
  - includes private architect/stylist headers

- `layout_stylist.c/.h`
  - already extracted
  - owns palette constants, compact/full classification, font key mapping, and
    style filling

- `layout_rect.c/.h`
  - already extracted
  - private to layout
  - owns rectangular geometry
  - fills every rectangular substratum frame

- `layout_round.c/.h`
  - future extraction
  - private to layout
  - owns round geometry
  - must not be introduced speculatively before round layout is planned

- `substratum_renderer.c/.h`
  - helper boundary for rendering calculated substrata
  - owns common text/icon layer creation helpers
  - owns text-layer update helper
  - owns color-role lookup from a `ColorPalette`
  - owns icon coordinate scaling helpers

Only `layout.c` should include architect headers. Feature modules and
`watchface.c` should not include private architect/stylist headers.

## Rectangle Architect Flow

The rectangle architect is already extracted and should remain geometry-only.
Future rectangle visual redesigns should change calculated substratum frames
inside `layout_rect.c`, not layer creation or module lifecycle.

Private flow:

```c
void layout_rect_calculate_surface(
    int16_t face_width,
    int16_t face_height,
    WatchfaceSurface* surface);
```

Inside `layout_rect.c`, prefer small file-local helpers:

```c
static void assign_background(...);
static void assign_date(...);
static void assign_time(...);
static void assign_battery(...);
static void assign_climate(...);
static void assign_steps(...);
static void assign_bpm(...);
```

This is better than large helpers named after rows such as
`assign_bottom_strata()`. The watchface may use rows visually, but the durable
contract is per substratum. Assigning each module's text/icon frames directly
also prepares the code for round layouts where icon and text may stack or
separate differently.

Use a private metrics struct only if it reduces clutter:

```c
typedef struct {
  int16_t face_width;
  int16_t face_height;
  int16_t margin;
  int16_t content_left;
  int16_t content_right;
  int16_t content_width;
  int16_t display_center;
  int16_t row_gap;
  GSize icon_size;
  int16_t icon_text_gap;
  int16_t data_width;
} LayoutRectMetrics;
```

This struct is not a public object. It is only a file-local grouping of
derived rectangle metrics so the calculation is readable and not a spreadsheet
of unrelated locals.

## Proposed Rectangle Redesign

Do not mix this with the behavior-preserving rectangle architect extraction.
This has not been implemented. Treat it as a future committed visual slice,
separate from the completed layout-boundary refactor.

Proposed visual order:

```text
Battery icon + text    Weather icon + temperature
Time
Rule
Date
Steps icon + text      BPM icon + text
```

Intent:

- time remains dominant
- rule remains the horizon line
- date becomes less subordinate with a larger Gothic font
- top row becomes quiet system context
- bottom row becomes health context

This reads more like a centered instrument than the current dashboard-like
four-row layout.

Implementation scope for the rectangle visual slice:

- revise `layout_rect.c` geometry only after auditing the current rectangular
  screenshots
- keep each substratum responsible for its own final `x`, `y`, `w`, and `h`
- keep module lifecycle unchanged: modules still create, refresh, and destroy
  their own layers from calculated substrata
- keep watchface as the clearing house only; do not move rendering details
  into `watchface.c`
- avoid adding helper functions for calculations that are clearer inline
- build and screenshot at least Emery and one compact rectangular platform

The rectangle slice should not introduce round support. It should establish
the visual vocabulary that the later round architect can translate.

## Proposed Round Redesign

This has not been implemented. Treat it as a separate future visual slice
after the rectangle redesign has been validated.

The round design should preserve the same product sequence where possible:

```text
Battery / Climate
Time
Rule
Date
Steps / BPM
```

On Chalk, top and bottom modules likely need stacked or compact icon/text
treatments because circle-safe spans shrink quickly near the edges. On Gabbro,
more of the rectangular horizontal vocabulary may survive, but the architect
must still calculate safe spans from the actual face size.

Implementation scope for the round visual slice:

- introduce `layout_round.c/.h` privately behind `layout.c`
- calculate row-specific safe spans from chord width
- assign final frames directly to each substratum
- keep the same six fixed strata: date, time, bpm, steps, battery, climate
- do not enable round platforms until the layout is built, installed, and
  screenshot-reviewed
- do not scale rectangular coordinates wholesale and call that round support

## Round Architect Flow

Round layouts must use circle-aware row spans, not square-screen assumptions.

For a round display:

```c
radius = face_width / 2;
center_x = face_width / 2;
center_y = face_height / 2;
dy = row_y - center_y;
half_width = sqrt(radius * radius - dy * dy);
safe_left = center_x - half_width + margin;
safe_right = center_x + half_width - margin;
safe_width = safe_right - safe_left;
```

Use row-specific safe spans. Do not assume a top or bottom row can use the
same horizontal width as the center.

## Chalk Reference

Chalk reference size: `180x180`.

With `margin = 4`, approximate safe spans:

```text
y=18   x=40..140   width=100
y=32   x=25..155   width=130
y=50   x=13..167   width=153
y=74   x=5..175    width=169
y=90   x=4..176    width=172
y=114  x=7..173    width=165
y=138  x=18..162   width=144
y=146  x=24..157   width=133
y=154  x=31..149   width=119
y=166  x=46..134   width=88
y=172  x=57..123   width=66
```

Chalk likely needs stacked or compact top/bottom modules. Horizontal icon-text
pairs near the top and bottom will clip or feel cramped.

Initial Chalk visual order:

```text
Battery / Weather, likely stacked
Time
Rule
Date
Steps / BPM, likely stacked or compact
```

Avoid using fixed frames such as `GRect(94, 146, 70, 28)` as a Chalk
reference without checking the row's safe circular span.

## Gabbro Reference

Gabbro reference size: `260x260`.

With `margin = 6`, approximate safe spans:

```text
y=18   x=70..190    width=120
y=32   x=51..209    width=159
y=50   x=34..227    width=193
y=74   x=19..241    width=223
y=90   x=12..248    width=235
y=114  x=7..253     width=246
y=130  x=6..254     width=248
y=154  x=8..252     width=244
y=178  x=15..245    width=230
y=202  x=28..232    width=205
y=226  x=48..212    width=163
y=238  x=64..196    width=133
y=246  x=77..183    width=105
```

Gabbro can preserve more of the rectangular horizontal vocabulary than Chalk.
The round architect should still calculate from chord width rather than use a
hard-coded platform layout whenever possible.

## Guardrails

- Extract rectangle architect before redesigning rectangle.
- Do not add round code in the rectangle extraction slice.
- Do not move Pebble layer creation out of feature modules.
- Do not introduce generic dynamic strata arrays.
- Do not pass or return large watchface structs by value.
- Do not change palette, font choices, or visible geometry during the
  behavior-preserving architect extraction.
- Build after each extraction.
- Screenshot after geometry moves.
