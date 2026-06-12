# Round Watchface Plan

This is the planning artifact for adding round-display support. It is
not approval to add `chalk` or `gabbro` yet, and it is not approval to
change rectangular behavior.

The current codebase has already moved to the `WatchfaceSurface`
architecture. This plan reflects that architecture and should be used as
the handoff for the eventual round implementation.

## Source Findings

- Pebble's round UI guidance says rectangular layouts fail on round
  screens because corners are clipped and because centered geometry
  becomes more important on the `180x180` Chalk display.
- Pebble's general multi-platform guidance says each horizontal line of
  text has a different available width on round displays.
- Pebble's round design guidance recommends a two-pixel edge margin and
  warns against thin rings at the outer edge.
- Pebble text APIs support text flow and paging, but this watchface has
  single-line fixed values. Calculate safe frames instead of relying on
  text flow to rescue poor placement.
- The rePebble Round 2 SDK note says `gabbro` is the Pebble Round 2
  platform and its display is `260x260`, compared with `180x180` for
  Chalk.
- Official example work such as Time Dots and Concentricity is
  deliberately round-native. It does not preserve a rectangular row
  layout.

Sources:

- <https://developer.repebble.com/guides/user-interfaces/round-app-ui/>
- <https://developer.repebble.com/guides/design-and-interaction/in-the-round/>
- <https://developer.repebble.com/guides/best-practices/building-for-every-pebble/>
- <https://developer.rebble.io/docs/c/Graphics/Drawing_Text/>
- <https://repebble.com/blog/cloudpebble-returns-plus-new-pure-javascript-and-round-2-sdk>
- <https://github.com/pebble-examples/time-dots/>

## Current Architecture

The current watchface is organized around a calculated surface and fixed
module-owned strata:

- `surface_builder_prepare()` is the public surface-construction API.
- The surface builder initializes the caller-owned surface, asks the active
  architect to apply the shape blueprint, and then applies style.
- Rectangular and round architects use one architect contract. The active
  implementation is selected by Pebble shape guards at compile time.
- `layout_stylist.c/.h` owns palette, font, and custom-font decisions. It
  consumes compact/full state from the calculated surface.
- `layout_rect.c` owns rectangular geometry.
- `layout_round.c` owns round geometry.
- `watchface.c` is the runtime clearing house.
- Feature modules own Pebble layers, render state, buffers, refresh,
  update procs, and destroy paths.
- `main.c` dispatches platform/service/AppMessage events to `watchface`;
  it should not call feature modules directly.

Current layout call flow:

```c
watchface_create()
  -> surface_builder_prepare(face_width,
                             face_height,
                             display_mode,
                             &s_surface)
       -> memset(surface, 0, sizeof(*surface))
       -> architect_apply_blueprint(face_width,
                                    face_height,
                                    surface)
       -> layout_stylist_update_surface_style(&surface->style, display_mode)
```

This is the model for round. `surface_builder_prepare()` remains the
single public entry point from watchface runtime to surface construction.
It prepares a fresh calculated surface, then calls the active architect
contract and stylist in sequence:

```c
memset(surface, 0, sizeof(*surface));
architect_apply_blueprint(face_width, face_height, surface);
layout_stylist_update_surface_style(&surface->style, display_mode);
```

Use `PBL_RECT` and `PBL_ROUND` for shape-specific architect implementation.
Use capability macros such as `PBL_COLOR`, `PBL_BW`, and `PBL_HEALTH`
only for color, monochrome, and health behavior. Do not use
`PBL_PLATFORM_CHALK` or `PBL_PLATFORM_GABBRO` for general round
geometry unless a model-specific exception is proven by screenshots.

`WatchfaceSurface` is the calculated UI contract. It is not Pebble layer
state. It contains:

- face dimensions
- one `WatchfaceSurfaceStyle`, including the calculated compact/full state
- one background substratum
- six fixed strata: date, time, battery, climate, steps, bpm

The fixed strata are intentional. Do not replace them with a generic
dynamic strata array.

Text substrata carry final frame, text alignment, font role, and color
role. Icon substrata carry final frame and enabled flag.
The background substratum carries the full-face frame and the horizon
line coordinates.

Architect invariants:

- `WatchfaceSurface` carries final substratum frames only.
- Intermediate metrics such as content bounds, safe spans, margins,
  row gaps, and pair widths stay private to the active architect
  implementation.
- Shape architects consume canonical product constants through private
  file-local blueprints and derive resolved metrics privately.
- Shape architects own compact/full classification because they own the
  geometry context. They store that resolved state on
  `WatchfaceSurfaceStyle`.
- `layout_rect.c` owns rectangular intermediate math in private structs
  such as `LayoutRectMetrics`.
- `layout_round.c` should own round intermediate math in private structs
  such as `LayoutRoundMetrics` or row-safe-span helpers.
- No layout-private metrics should leak into `watchface_components.h`.
- Each substratum gets its own `x`, `y`, `w`, and `h` through its final
  `GRect`; callers should not reconstruct coordinates.

## Current Rectangular Baseline

The rectangular architect currently calculates this visual sequence:

```text
Battery icon + text        Climate icon + temperature
Centered time
2px horizon rule
Centered date
Steps icon + text          BPM icon + text
```

The rectangular layout is the product baseline, not a geometry template
for round. Round should feel related to this visual vocabulary, but it
must be calculated from circular safe spans.

Current styling decisions that round must account for:

- `ColorPalette` is colors-only.
- The active shape architect calculates compact/full once from its geometry
  blueprint and face dimensions, then stores compact/full on
  `WatchfaceSurfaceStyle`.
- Palette and font decisions are in `layout_stylist.c`; the stylist consumes
  the stored compact/full state and does not recompute it from dimensions.
- Dynamic battery and BPM colors are module-owned by
  `calculate_battery_color()` and `calculate_bpm_color()`.
- With the current rectangular blueprint, compact means face width below `200` or
  face height below `228`.
- Chalk is compact.
- Gabbro is full.

## Round Geometry Rule

Round layout needs row-specific circular safe spans, not one global
content width.

For a round display:

```c
diameter = min(face_width, face_height);
radius = diameter / 2;
center_x = face_width / 2;
center_y = face_height / 2;
safe_radius = radius - edge_margin;

dy = max(abs(row_y - center_y), abs((row_y + row_h - 1) - center_y));
half_width = isqrt((safe_radius * safe_radius) - (dy * dy));
safe_x = center_x - half_width;
safe_w = half_width * 2;
```

Use integer math only. Do not introduce floating point.

Use the farthest vertical edge of a row band when calculating `dy`.
This protects the full height of text/icon frames, not only the row's
centerline.

Use `content_margin` as the edge margin. Do not add a separate optical inset
unless screenshots later prove a distinct round-only correction is needed.

## Computed Chalk Safe Spans

Chalk is the hard target because it is `180x180`.

Reference calculation with `margin = 4`:

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

Implications:

- Chalk cannot use a scaled rectangular two-column layout near the top
  or bottom.
- Lower-row horizontal icon-plus-value pairs become fragile as soon as
  the row moves below roughly `y=146`.
- Text-first complications are more durable than full icon/value pairs.
- Optional icons must be disabled explicitly when they do not fit.

## Computed Gabbro Safe Spans

Gabbro is `260x260` and should be treated as full round.

Reference calculation with `margin = 6`:

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

Implications:

- Gabbro can likely preserve more horizontal icon/text vocabulary than
  Chalk.
- Gabbro should still use the same round-safe-span model.
- Do not hard-code a scaled rectangular layout just because Gabbro has
  more room.

## Product Layout Direction

Preserve the current visual vocabulary across rectangular and round
faces:

```text
Top context: battery and climate
Dominant centered time
Centered 2px horizon rule
Centered date
Bottom health context: steps and bpm
```

Chalk should start from a compact, text-first interpretation:

- top context may need stacking or aggressively compact placement
- time remains centered and dominant
- rule remains centered and short enough to fit its safe span
- date remains centered
- bottom health context may need text-first placement or stacked
  icon/text treatment
- climate and battery icons are optional if text is already
  self-labeling
- steps and BPM icons are more valuable because raw values are less
  self-labeling

Gabbro should start with the same conceptual model:

- same sequence as Chalk
- larger safe spans may allow more icon/value pairs
- geometry still comes from circular safe spans
- screenshots decide whether optional icons return

## Round Architect Role

`layout_round.c` is the round implementation of the shared architect
contract. It remains private to surface construction.

The round architect uses the same narrow API as the rectangular architect:

```c
void architect_apply_blueprint(
    int16_t face_width,
    int16_t face_height,
    WatchfaceSurface* surface);
```

The function receives scalar face dimensions and a caller-owned surface
pointer. It must fill the existing substrata in place. It must not pass
or return `WatchfaceSurface` by value.

The round architect calculates geometry only. It fills the existing
`WatchfaceSurface` substrata:

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

The round architect must not:

- create Pebble `Layer` or `TextLayer` objects
- set update procs
- call feature modules
- decide runtime data availability
- change AppMessage behavior
- own palette or font resolution
- pass or return large watchface structs by value

Every known substratum gets an explicit frame. Layout is not a generic
row engine, even when a round design uses visual rows.

Conditional compilation flow:

- The public surface-preparation API remains shape-agnostic.
- The surface builder calls the shared architect contract.
- `layout_rect.c` provides `architect_apply_blueprint()` under `PBL_RECT`.
- `layout_round.c` provides `architect_apply_blueprint()` under `PBL_ROUND`.
- Shape-specific implementation stays behind shape guards if the Pebble build
  system compiles all listed C files for all platforms.
- Feature modules and `watchface.c` continue to include only public
  contracts such as the surface-preparation API, `watchface_components.h`,
  and `substratum_renderer.h` as appropriate. They must not include private
  architect or stylist headers.

## Guardrails And Invariants

- Do not enable `chalk` or `gabbro` in `package.json` until the round
  geometry is implemented and screenshot-reviewed.
- Do not add dormant round source code without an implementation slice
  that can be built and inspected.
- Preserve current rectangular behavior while planning and implementing
  round support.
- Feature modules continue to create/destroy their own layers from their
  own substrata.
- Optional icon behavior uses `WatchfaceIconSubstratum.is_enabled`.
- Do not use failed `layer_create()`, zero-size frames, or hidden text
  as control flow.
- Text is the primary glance surface; icons are secondary support.
- If a text-bearing stratum creates its text layer, the module can be
  considered meaningfully created even if an optional icon layer fails.
- Keep `ColorPalette` colors-only.
- Dynamic battery and BPM colors remain module-owned.
- Public APIs must be external functions declared in headers, not
  `static` or `inline`.
- Private file-local helpers may remain `static`.
- Preserve AppMessage keys and runtime behavior.
- Build rectangular targets after any source change, even if round is
  behind platform guards.
- Screenshot before approving any geometry change.
- Preserve the current `watchface_create()` responsibility split:
  watchface owns the live `WatchfaceSurface`, layout calculates it, and
  feature modules create their own layers from their assigned substrata.
- Keep `watchface_refresh()` as the runtime dispatch boundary. Round
  support should not make `main.c` aware of feature modules or shape
  internals.
- Do not add round-specific conditionals to feature modules unless a
  module has a proven rendering exception that cannot be expressed by
  calculated substratum frames, font roles, text color roles, or
  icon `is_enabled`.
- Do not change palette, typography, AppMessage, debug health, or
  module lifecycle behavior in the same slice as first round geometry.
- Keep custom-font decisions in `layout_stylist.c`; layout calculation owns
  compact/full style classification, and `time.c` must not make shape
  decisions locally.

## Implementation Sequence

Phase 1: round architect scaffolding.

- Add private `layout_round.c`.
- Implement `architect_apply_blueprint()` under `PBL_ROUND`.
- Keep the surface builder's public call shape unchanged.
- Keep `package.json` targets unchanged in this phase.
- Build current rectangular targets to confirm no drift.

Phase 2: geometry only.

- Add integer safe-span helpers in `layout_round.c`.
- Calculate explicit frames for every existing substratum.
- Use `is_enabled` for optional icon suppression.
- Do not change fonts, palette, AppMessage, module lifecycle, or
  feature-module behavior.

Phase 3: target enablement and screenshot review.

- Add `chalk` only when the round architect has real geometry.
- Build and screenshot Chalk.
- Adjust only round geometry/icon enablement based on screenshot
  findings.
- Add `gabbro` after Chalk is understood, then screenshot and evaluate
  whether Gabbro can restore optional icons.

Phase 4: documentation and cleanup.

- Update README and design notes only after screenshots confirm the
  round layout.
- Keep source comments sparse and tied to non-obvious geometry.
- Commit round support as a focused slice, not mixed with palette,
  AppMessage, or unrelated module cleanup.

## Validation For This Plan Update

This document update is a documentation-only slice.

- Run `git diff --check`.
- Review the diff and confirm only `RoundWatchfacePlan.md` changed.
- No `pebble build` is required for this documentation-only update.
