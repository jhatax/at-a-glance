#include "substratum_renderer.h"

#include "layout_blueprints.h"
#include "substratum_computations.h"
#include "watchface_debug.h"

void substratum_renderer_draw_scaled_line_in_frame(
    GContext* ctx,
    const GRect* frame,
    int16_t x0,
    int16_t y0,
    int16_t x1,
    int16_t y1) {
  if (!ctx || !frame) {
    return;
  }

  graphics_draw_line(
      ctx,
      substratum_renderer_scale_icon_x_y_in_frame(frame, x0, y0),
      substratum_renderer_scale_icon_x_y_in_frame(frame, x1, y1));
}

void substratum_renderer_fill_scaled_rect_from_corners_in_frame(
    GContext* ctx,
    const GRect* frame,
    int16_t x0,
    int16_t y0,
    int16_t x1,
    int16_t y1) {
  if (!ctx || !frame) {
    return;
  }

  int16_t scaled_w = substratum_renderer_scale_icon_x(&frame->size, (x1 - x0));
  int16_t scaled_h = substratum_renderer_scale_icon_y(&frame->size, (y1 - y0));
  GRect rect = GRect(
      substratum_renderer_scale_icon_x_in_frame(frame, x0),
      substratum_renderer_scale_icon_y_in_frame(frame, y0),
      scaled_w,
      scaled_h);

  graphics_fill_rect(ctx, rect, 0, GCornerNone);
}

void substratum_renderer_fill_scaled_circle_in_frame(
    GContext* ctx,
    const GRect* frame,
    int16_t x,
    int16_t y,
    int16_t r) {
  if (!ctx || !frame) {
    return;
  }

  GPoint center = substratum_renderer_scale_icon_x_y_in_frame(frame, x, y);
  int16_t s_r = substratum_renderer_scale_icon_coord(&frame->size, r);
  graphics_fill_circle(ctx, center, s_r);
}

void substratum_renderer_draw_scaled_circle_in_frame(
    GContext* ctx,
    const GRect* frame,
    int16_t x,
    int16_t y,
    int16_t r) {
  if (!ctx || !frame) {
    return;
  }
  GPoint center = substratum_renderer_scale_icon_x_y_in_frame(frame, x, y);
  int16_t s_r = substratum_renderer_scale_icon_coord(&frame->size, r);
  graphics_draw_circle(ctx, center, s_r);
}

void substratum_renderer_draw_filled_bolt_in_frame(
    GContext* ctx,
    const GRect* frame,
    GColor fill_color) {
  if (!ctx || !frame) {
    return;
  }

  uint8_t stroke_width = 2;
  uint8_t offset = stroke_width;
  uint8_t double_offset = offset << 1;
  // Need 7-bolt points to form a path
  GPoint bolt_points[] = {
      substratum_renderer_scale_icon_x_y_in_frame(frame, WATCHFACE_ICON_CENTER_X + offset, offset),
      substratum_renderer_scale_icon_x_y_in_frame(frame, double_offset, WATCHFACE_ICON_CENTER_Y),
      substratum_renderer_scale_icon_x_y_in_frame(
          frame,
          WATCHFACE_ICON_CENTER_X,
          WATCHFACE_ICON_CENTER_Y),
      substratum_renderer_scale_icon_x_y_in_frame(
          frame,
          WATCHFACE_ICON_CENTER_X - offset,
          WATCHFACE_ICON_GRID_HEIGHT - offset),
      substratum_renderer_scale_icon_x_y_in_frame(
          frame,
          WATCHFACE_ICON_GRID_WIDTH - double_offset,
          WATCHFACE_ICON_CENTER_Y - offset),
      substratum_renderer_scale_icon_x_y_in_frame(
          frame,
          WATCHFACE_ICON_CENTER_X,
          WATCHFACE_ICON_CENTER_Y - offset),
      bolt_points[0],
  };

  GPathInfo path_info = (GPathInfo){
      .num_points = ARRAY_LENGTH(bolt_points),
      .points = bolt_points,
  };
  GPath* path = gpath_create(&path_info);

  if (path) {
    graphics_context_set_fill_color(ctx, fill_color);
    graphics_context_set_stroke_color(ctx, fill_color);
    gpath_draw_filled(ctx, path);
    gpath_draw_outline(ctx, path);
    gpath_move_to(path, GPoint(1, 0));
    gpath_draw_filled(ctx, path);
    graphics_context_set_stroke_width(ctx, stroke_width);
    gpath_destroy(path);
  }
  path = NULL;
}

void substratum_renderer_mark_info_outofrange(
    GContext* ctx,
    const GRect* frame,
    GColor color,
    GColor background) {
  if (!ctx || !frame) {
    return;
  }

  // Unavailable slash contract: keep the 28x28 design-space diagonal
  // shared across metric and weather glyphs so missing-data affordance
  // does not drift between modules. Stroke-width set to 2.
  graphics_context_set_stroke_color(ctx, background);
  graphics_context_set_stroke_width(ctx, 2);
  substratum_renderer_draw_scaled_line_in_frame(ctx, frame, 1, 1, 26, 26);
  graphics_context_set_stroke_color(ctx, color);
  substratum_renderer_draw_scaled_line_in_frame(ctx, frame, 1, 1, 6, 6);
  substratum_renderer_draw_scaled_line_in_frame(ctx, frame, 11, 11, 16, 16);
  substratum_renderer_draw_scaled_line_in_frame(ctx, frame, 21, 21, 26, 26);
}

// Module creation and lookup-helpers
TextLayer* substratum_renderer_create_text_layer(
    Layer* parent,
    const WatchfaceTextSubstratum* text,
    GFont font) {
  if (!parent || !text) {
    return NULL;
  }

  TextLayer* layer = text_layer_create(text->frame);
  if (!layer) {
    return NULL;
  }

  text_layer_set_background_color(layer, GColorClear);
  text_layer_set_font(layer, font);
  text_layer_set_text_alignment(layer, text->alignment);
  layer_add_child(parent, text_layer_get_layer(layer));
  return layer;
}

Layer* substratum_renderer_create_icon_layer(
    Layer* parent,
    const WatchfaceIconSubstratum* icon,
    LayerUpdateProc update_proc) {
  if (!parent || !icon) {
    return NULL;
  }

  Layer* layer = layer_create(icon->frame);
  if (!layer) {
    return NULL;
  }

  if (update_proc) {
    layer_set_update_proc(layer, update_proc);
  }
  layer_add_child(parent, layer);
  return layer;
}

void substratum_renderer_update_text_layer(
    TextLayer* layer,
    const char* text,
    GColor text_color) {
  if (!layer || !text) {
    return;
  }

#if ATAGLANCE_DEBUG
  // Visual indicator of text layer size and text rendering
  GColor switched = gcolor_legible_over(text_color);
  text_layer_set_background_color(layer, text_color);
  text_layer_set_text_color(layer, switched);
  text_layer_set_text(layer, text);
#else
  text_layer_set_background_color(layer, GColorClear);
  // Write the text twice, once in the clear color, then in the text color
  // Will this increase contrast?
  text_layer_set_text_color(layer, GColorClear);
  text_layer_set_text(layer, text);
  text_layer_set_text_color(layer, text_color);
  text_layer_set_text(layer, text);
#endif
}

GColor substratum_renderer_color_for_role(
    const ColorPalette* palette,
    WatchfaceColorRole role) {
  if (!palette) {
    return GColorWhite;
  }

  switch (role) {
    case WATCHFACE_COLOR_ROLE_PRIMARY_TEXT:
      return palette->primary_text;
    case WATCHFACE_COLOR_ROLE_OUTOFRANGE_TEXT:
      return palette->outofrange_text;
    case WATCHFACE_COLOR_ROLE_DATE:
      return palette->date_text;
    case WATCHFACE_COLOR_ROLE_TIME:
      return palette->time_text;
    default:
      return palette->primary_text;
  }
}
