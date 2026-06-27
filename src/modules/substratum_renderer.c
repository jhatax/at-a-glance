#include "substratum_renderer.h"
#include "helper.h"
#include "pebble.h"
#include "watchface_debug.h"

// Scale icon's X relative to the current frame vs. the reference design
int16_t substratum_renderer_scale_icon_x(const GSize *size, int16_t coord) {
  if (!size || !HELPER_VALID_DESIGN_X(coord)) {
    return 0;
  }

  // Clamp so coords go from 0..total-1
  return HELPER_CLAMP_MAX(
      HELPER_SCALE_ROUND(coord, size->w, WATCHFACE_ICON_GRID_WIDTH),
      (size->w - 1));
}

// Scale icon's Y relative to the current frame vs. the reference design
int16_t substratum_renderer_scale_icon_y(const GSize *size, int16_t coord) {
  if (!size || !HELPER_VALID_DESIGN_Y(coord)) {
    return 0;
  }

  // Clamp so coords go from 0..total-1
  return HELPER_CLAMP_MAX(
      HELPER_SCALE_ROUND(coord, size->h, WATCHFACE_ICON_GRID_HEIGHT),
      (size->h - 1));
}

int16_t substratum_renderer_scale_icon_coord(const GSize *size, int16_t coord) {
  if (!size) {
    return 0;
  }

  if (!(HELPER_VALID_DESIGN_X(coord) || HELPER_VALID_DESIGN_Y(coord))) {
    return 0;
  }

  int16_t chosen_dimension = HELPER_MIN(size->w, size->h);
  int16_t design_dimension = (chosen_dimension == size->w)
                                 ? WATCHFACE_ICON_GRID_WIDTH
                                 : WATCHFACE_ICON_GRID_HEIGHT;
  // Clamp so coords go from 0..total-1
  return HELPER_CLAMP_MAX(
      HELPER_SCALE_ROUND(coord, chosen_dimension, design_dimension),
      (chosen_dimension - 1));
}

GPoint substratum_renderer_scale_icon_point(const GSize *size, int16_t x, int16_t y) {
  if (!(HELPER_VALID_DESIGN_X(x) && HELPER_VALID_DESIGN_Y(y))) {
    // Return the origin of the frame; at least something will get drawn
    return GPoint(0, 0);
  }

  if (size) {
    return GPoint(substratum_renderer_scale_icon_x(size, x),
                  substratum_renderer_scale_icon_y(size, y));
  }

  return GPoint(x, y);
}

int16_t substratum_renderer_scale_icon_x_in_frame(const GRect *frame, int16_t coord) {
  if (!frame) {
    return 0;
  }

  // Clamp so coords go from 0..total-1
  return HELPER_CLAMP_MAX(
      frame->origin.x + substratum_renderer_scale_icon_x(&frame->size, coord),
      (frame->origin.x + frame->size.w - 1));
}

int16_t substratum_renderer_scale_icon_y_in_frame(const GRect *frame, int16_t coord) {
  if (!frame) {
    return 0;
  }

  // Clamp so coords go from 0..total-1
  return HELPER_CLAMP_MAX(
      frame->origin.y + substratum_renderer_scale_icon_y(&frame->size, coord),
      (frame->origin.y + frame->size.h - 1));
}

void substratum_renderer_scale_icon_point_in_frame(const GRect *frame, GPoint *input) {
  input->x = substratum_renderer_scale_icon_x_in_frame(frame, input->x);
  input->y = substratum_renderer_scale_icon_y_in_frame(frame, input->y);
}

GPoint substratum_renderer_scale_icon_x_y_in_frame(const GRect *frame,
                                                   int16_t x, int16_t y) {
  return GPoint(substratum_renderer_scale_icon_x_in_frame(frame, x),
                substratum_renderer_scale_icon_y_in_frame(frame, y));
}

void substratum_renderer_draw_scaled_line(GContext *ctx, const GSize *size,
                                          int16_t x0, int16_t y0, int16_t x1,
                                          int16_t y1) {
  if (!ctx || !size) {
    return;
  }

  graphics_draw_line(ctx, substratum_renderer_scale_icon_point(size, x0, y0),
                     substratum_renderer_scale_icon_point(size, x1, y1));
}

void substratum_renderer_draw_scaled_line_in_frame(GContext *ctx,
                                                   const GRect *frame,
                                                   int16_t x0, int16_t y0,
                                                   int16_t x1, int16_t y1) {
  if (!ctx || !frame) {
    return;
  }

  graphics_draw_line(
      ctx, substratum_renderer_scale_icon_x_y_in_frame(frame, x0, y0),
      substratum_renderer_scale_icon_x_y_in_frame(frame, x1, y1));
}

void substratum_renderer_fill_scaled_rect_from_corners_in_frame(
    GContext *ctx, const GRect *frame, int16_t x0, int16_t y0, int16_t x1,
    int16_t y1) {
  if (!ctx || !frame) {
    return;
  }

  graphics_fill_rect(
      ctx,
      GRect(substratum_renderer_scale_icon_x_in_frame(frame, x0),
            substratum_renderer_scale_icon_y_in_frame(frame, y0),
            substratum_renderer_scale_icon_x_in_frame(frame, x1) -
                substratum_renderer_scale_icon_x_in_frame(frame, x0),
            substratum_renderer_scale_icon_y_in_frame(frame, y1) -
                substratum_renderer_scale_icon_y_in_frame(frame, y0)),
      0, GCornerNone);
}

void substratum_renderer_fill_scaled_circle_in_frame(GContext *ctx,
                                                     const GRect *frame,
                                                     int16_t x, int16_t y,
                                                     int16_t r) {
  if (!ctx || !frame) {
    return;
  }

  graphics_fill_circle(ctx,
                       substratum_renderer_scale_icon_x_y_in_frame(frame, x, y),
                       substratum_renderer_scale_icon_coord(&frame->size, r));
}

void substratum_renderer_draw_scaled_circle_in_frame(GContext *ctx,
                                                     const GRect *frame,
                                                     int16_t x, int16_t y,
                                                     int16_t r) {
  if (!ctx || !frame) {
    return;
  }
  graphics_draw_circle(ctx,
                       substratum_renderer_scale_icon_x_y_in_frame(frame, x, y),
                       substratum_renderer_scale_icon_coord(&frame->size, r));
}

void substratum_renderer_draw_filled_bolt_in_frame(GContext *ctx,
                                                   const GRect *frame,
                                                   GColor fill_color) {
  if (!ctx || !frame) {
    return;
  }

  // Need 7-bolt points to form a path
  GPoint bolt_points[] = {
      substratum_renderer_scale_icon_x_y_in_frame(frame, 8, 1),
      substratum_renderer_scale_icon_x_y_in_frame(frame, 4, 14),
      substratum_renderer_scale_icon_x_y_in_frame(frame, 8, 14),
      substratum_renderer_scale_icon_x_y_in_frame(frame, 6, 24),
      substratum_renderer_scale_icon_x_y_in_frame(frame, 24, 8),
      substratum_renderer_scale_icon_x_y_in_frame(frame, 16, 8),
      substratum_renderer_scale_icon_x_y_in_frame(frame, 20, 1),
  };

  GPathInfo path_info = (GPathInfo){
      .num_points = ARRAY_LENGTH(bolt_points),
      .points = bolt_points,
  };
  GPath *path = gpath_create(&path_info);

  if (path) {
    graphics_context_set_fill_color(ctx, fill_color);
    gpath_draw_filled(ctx, path);
    graphics_context_set_stroke_color(ctx, fill_color);
    uint8_t stroke_width = SUBSTRATUM_RENDERER_ICON_STROKE_WIDTH(HELPER_MIN(frame->size.w, frame->size.h), 2);
    graphics_context_set_stroke_width(ctx,stroke_width);
    // Add a 2-pt offset to draw a halo outside the currently filled path
    gpath_move_to(path, GPoint(-stroke_width,stroke_width));
    gpath_draw_outline(ctx, path);
    gpath_destroy(path);
  }
  path = NULL;
}

void substratum_renderer_draw_unavailable_slash(GContext *ctx,
                                                const GSize *size,
                                                GColor color) {
  if (!ctx || !size) {
    return;
  }

  // Unavailable slash contract: keep the 28x28 design-space diagonal
  // shared across metric and weather glyphs so missing-data affordance
  // does not drift between modules. Stroke-width set to 2.
  graphics_context_set_stroke_color(ctx, color);
  graphics_context_set_stroke_width(ctx, 2);
  substratum_renderer_draw_scaled_line(ctx, size, 5, 5, 10, 10);
  substratum_renderer_draw_scaled_line(ctx, size, 14, 14, 18, 18);
  substratum_renderer_draw_scaled_line(ctx, size, 22, 22, 26, 26);
}

// Module creation and lookup-helpers
TextLayer *substratum_renderer_create_text_layer(
    Layer *parent, const WatchfaceTextSubstratum *text, GFont font) {
  if (!parent || !text) {
    return NULL;
  }

  TextLayer *layer = text_layer_create(text->frame);
  if (!layer) {
    return NULL;
  }

  text_layer_set_background_color(layer, GColorClear);
  text_layer_set_font(layer, font);
  text_layer_set_text_alignment(layer, text->alignment);
  layer_add_child(parent, text_layer_get_layer(layer));
  return layer;
}

Layer *
substratum_renderer_create_icon_layer(Layer *parent,
                                      const WatchfaceIconSubstratum *icon,
                                      LayerUpdateProc update_proc) {
  if (!parent || !icon || !icon->is_enabled) {
    return NULL;
  }

  Layer *layer = layer_create(icon->frame);
  if (!layer) {
    return NULL;
  }

  if (update_proc) {
    layer_set_update_proc(layer, update_proc);
  }
  layer_add_child(parent, layer);
  return layer;
}

void substratum_renderer_update_text_layer(TextLayer *layer, const char *text,
                                           GColor text_color) {
  if (!layer || !text) {
    return;
  }

#if ATAGLANCE_DEBUG
  // Visual indicator of text layer size and text rendering
  GColor switched = gcolor_legible_over(text_color);
  text_layer_set_background_color(layer, text_color);
  text_layer_set_text_color(layer, switched);
#else
  text_layer_set_background_color(layer, GColorClear);
  text_layer_set_text_color(layer, text_color);
#endif
  text_layer_set_text(layer, text);
}

GColor substratum_renderer_color_for_role(const ColorPalette *palette,
                                          WatchfaceColorRole role) {
  if (!palette) {
    return GColorWhite;
  }

  switch (role) {
  case WATCHFACE_COLOR_ROLE_PRIMARY_TEXT:
    return palette->primary_text;
  case WATCHFACE_COLOR_ROLE_UNAVAILABLE_TEXT:
    return palette->unavailable_text;
  case WATCHFACE_COLOR_ROLE_DATE:
    return palette->date_text;
  case WATCHFACE_COLOR_ROLE_TIME:
    return palette->time_text;
  default:
    return palette->primary_text;
  }
}
