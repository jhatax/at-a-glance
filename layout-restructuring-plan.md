# Layout Restructuring Plan

This plan captures the current architecture after the `WatchfaceSurface`
refactor, stylist extraction, rectangle architect extraction, and rectangular
visual redesign. It is the handoff for the next cleanup phase before round
watchface work.

This is still a boundary cleanup plan. It is not round support.

## Current State

The current code works and has the right broad ownership model:

- `watchface` is the runtime clearing house.
- feature modules own Pebble layer creation, refresh, update procs, state, and
  destroy paths.
- `layout` calculates the display contract: surface, strata, substrata, frames,
  style, fonts, palette, and color roles.
- `layout_stylist.c/.h` privately owns palette and font resolution.
- `layout_rect.c/.h` privately owns rectangular geometry.

The next cleanup is to split the shared display component model and
substratum rendering helpers out of `layout.h` / `layout.c`.

## Revised Target Shape

Use these files and boundaries:

- `ataglance.h`
  - product constants and compile-time product decisions only
  - examples: debug flag, design dimensions, row heights, font key constants,
    icon design dimensions, persisted-settings toggle
  - no Pebble layer helpers
  - no module lifecycle APIs

- `watchface_components.h`
  - shared display component model
  - contains only type definitions and display vocabulary
  - used by layout, renderer helpers, watchface runtime, climate glyphs, and
    feature modules

- `layout.h`
  - public layout API only
  - includes `watchface_components.h`
  - exposes `layout_calculate_surface()`, `layout_update_surface_style()`, and
    possibly `layout_color_for_role()`
  - does not expose text-layer creation/update helpers
  - does not expose icon coordinate scaling helpers

- `layout.c`
  - public facade for the layout module
  - dispatches to private layout architects
  - calls private stylist
  - keeps only layout API implementations

- `layout_stylist.c/.h`
  - private to layout
  - owns `ColorPalette` constants
  - owns display-mode palette selection
  - owns compact/full classification
  - owns font-key mapping for `WatchfaceFontRole`
  - fills `WatchfaceSurfaceStyle`

- `layout_rect.c/.h`
  - private to layout
  - owns rectangular coordinate calculation
  - fills every rectangular stratum and substratum frame
  - fills rectangular background/rule geometry

- `layout_round.c/.h`
  - future private layout architect
  - not added until round geometry is deliberately planned
  - must account for row-specific circular safe spans

- `substratum_renderer.c/.h`
  - public helper for rendering calculated substrata into Pebble runtime objects
  - includes `watchface_components.h`
  - creates and updates Pebble text layers from `WatchfaceTextSubstratum`
  - scales icon design coordinates into actual icon-layer bounds
  - does not own module state, update procs, source data, or dynamic colors

- `watchface.c/.h`
  - runtime orchestration API
  - creates modules explicitly
  - refreshes modules explicitly
  - routes platform/service/AppMessage events
  - owns the live `WatchfaceSurface`
  - should not become a utility dependency for feature modules

## `watchface_components.h`

Create `src/modules/watchface_components.h`.

Move these definitions out of `layout.h`:

- `WATCHFACE_UNAVAILABLE_TEXT`
- `WatchfaceFontRole`
- `WatchfaceColorRole`
- `ColorPalette`
- `WatchfaceTextSubstratum`
- `WatchfaceIconSubstratum`
- `WatchfaceBackgroundSubstratum`
- `WatchfaceTextStratum`
- `WatchfaceMetricStratum`
- `WatchfaceSurfaceStyle`
- `WatchfaceSurface`

This file is not a dumping ground for all shared typedefs. It is only for the
watchface display component model.

Do not put these in `watchface_components.h`:

- product constants: keep in `ataglance.h`
- layout APIs: keep in `layout.h`
- renderer APIs: keep in `substratum_renderer.h`
- module lifecycle APIs: keep in feature-module headers
- settings/runtime event APIs: keep in `settings.h` and `watchface.h`

## `substratum_renderer.h`

Create `src/modules/substratum_renderer.h`.

Target public API:

```c
TextLayer* substratum_renderer_create_text_layer(
    Layer* parent,
    const WatchfaceTextSubstratum* text,
    GFont font);

void substratum_renderer_update_text_layer(
    TextLayer* layer,
    const char* text,
    GColor text_color);

int16_t substratum_renderer_scale_icon_x(
    const GSize* bounds_size,
    int16_t coord);

int16_t substratum_renderer_scale_icon_y(
    const GSize* bounds_size,
    int16_t coord);

int16_t substratum_renderer_scale_icon_coord(
    const GSize* bounds_size,
    int16_t coord);

GPoint substratum_renderer_scale_icon_point(
    const GSize* bounds_size,
    int16_t x,
    int16_t y);
```

Move these implementations out of `layout.c`:

- `layout_update_text_layer()`
- `layout_scale_icon_x()`
- `layout_scale_icon_y()`
- `layout_scale_icon_coord()`
- `layout_scaled_icon_point()`
- private design-coordinate validators used by those scaling helpers

Rename call sites to the `substratum_renderer_*` names in the same slice.

Move repeated text-layer creation setup into
`substratum_renderer_create_text_layer()`:

```c
text_layer_create(text->frame);
text_layer_set_background_color(layer, GColorClear);
text_layer_set_font(layer, font);
text_layer_set_text_alignment(layer, text->alignment);
layer_add_child(parent, text_layer_get_layer(layer));
```

Feature modules should still own the returned layer pointer and destroy it.

Do not move these into `substratum_renderer`:

- text formatting
- dynamic color calculation
- icon update procs
- module source state
- weather glyph decisions
- battery/BPM/steps availability decisions

## Layout API After Cleanup

After `watchface_components.h` and `substratum_renderer.h` exist, `layout.h`
should be reduced to layout APIs over shared components:

```c
void layout_calculate_surface(
    int16_t face_width,
    int16_t face_height,
    uint8_t display_mode,
    WatchfaceSurface* surface);

void layout_update_surface_style(
    WatchfaceSurface* surface,
    uint8_t display_mode);

GColor layout_color_for_role(
    const ColorPalette* palette,
    WatchfaceColorRole role);
```

Keep `layout_color_for_role()` in `layout` for now. It maps layout color roles
to palette colors. Moving it to the renderer would blur layout vocabulary with
Pebble layer rendering.

## Watchface Display Mode API

Add a public watchface API:

```c
void watchface_update_display_mode(uint8_t display_mode);
```

`main.c` should call this when the `DISPLAY_MODE` setting changes.

`watchface_update_display_mode()` should:

- call private `watchface_update_style()`
- update the window background color
- refresh the background layer
- refresh all created modules whose rendered colors come from the palette

After this exists, remove defensive `watchface_update_style()` calls from
unrelated event paths:

- tick events
- battery events
- weather temperature updates
- weather condition updates
- health events
- debug BPM/steps updates

Do not make `watchface_update_style()` public. `main.c` should notify
watchface of display-mode changes, not mutate internal style state directly.

## Watchface Runtime Cleanup

Remove the redundant unconditional `background_module_destroy()` at the end of
`watchface_destroy()`.

The layer-created mask should be the single source of truth for which modules
were created and therefore which modules need destroy calls.

## Implementation Sequence

Use the prescribed coding cycle for each slice: audit, state patch shape,
edit narrowly, validate, review diff.

1. Remove redundant background destroy.
   - Edit only `watchface.c`.
   - Build.

2. Add `watchface_components.h`.
   - Move component/display type definitions out of `layout.h`.
   - Update includes so modules that only need `WatchfaceSurface` or
     `ColorPalette` include `watchface_components.h` instead of `layout.h`.
   - Keep `layout.h` as an API header over components.
   - Build.

3. Add `substratum_renderer.c/.h`.
   - Move text update and icon scaling helpers out of `layout.c/.h`.
   - Add text-layer creation helper.
   - Convert modules one at a time or in one coherent renderer slice after
     auditing all call sites.
   - Build.

4. Add `watchface_update_display_mode()`.
   - Keep `watchface_update_style()` private.
   - Route display mode changes from `main.c` through the new public API.
   - Remove defensive style updates from unrelated event handlers.
   - Build and test dark/light mode.

5. Re-audit layout public surface.
   - Confirm no renderer helpers remain in `layout.h`.
   - Confirm private layout headers are only included by layout implementation
     files.
   - Confirm feature modules do not include `layout.h` unless they call layout
     APIs directly.

6. Only after this cleanup, resume round planning.
   - Do not add `layout_round.c/.h` until round geometry is designed.
   - Round must use row-specific circular safe spans.

## Non-Goals

- Do not enable Chalk or Gabbro in this cleanup.
- Do not introduce round geometry yet.
- Do not move Pebble layer ownership out of feature modules.
- Do not create a generic dynamic stratum array.
- Do not hide watchface module creation behind a registry.
- Do not rename feature modules.
- Do not change AppMessage keys.
- Do not change glyph behavior.
- Do not change palette or layout visuals while extracting components and
  renderer helpers.

## Validation

Required after each code slice:

- `git diff --check`
- `pebble build`
- review diff for intended files only

Additional validation after display-mode API work:

- install/screenshot Emery in dark and light mode
- install/screenshot Flint if health-row colors or refresh behavior changed

Unrelated files, screenshots, and local plan copies must remain unstaged unless
explicitly requested.
