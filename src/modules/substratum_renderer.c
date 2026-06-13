#include "substratum_renderer.h"
#include "helper.h"
#include "../c/ataglance.h"

static bool is_valid_design_x_coord(int16_t x, int16_t design_width) {
  return (x >= 0 && x <= design_width);
}

static bool is_valid_design_y_coord(int16_t y, int16_t design_height) {
  return (y >= 0 && y <= design_height);
}

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
  if (!parent || !icon || !icon->is_enabled) {
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

  text_layer_set_background_color(layer, GColorClear);
  text_layer_set_text_color(layer, text_color);
  text_layer_set_text(layer, text);
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
    case WATCHFACE_COLOR_ROLE_UNAVAILABLE_TEXT:
      return palette->unavailable_text;
    case WATCHFACE_COLOR_ROLE_DATE:
      return palette->date;
    case WATCHFACE_COLOR_ROLE_TIME:
      return palette->time;
    default:
      return palette->primary_text;
  }
}

int16_t substratum_renderer_scale_icon_x(
    const GSize* size,
    int16_t coord) {
  if (!size || !is_valid_design_x_coord(coord, DESIGN_ICON_WIDTH)) {
    return 0;
  }

  return HELPER_SCALE_ROUND(coord, size->w, DESIGN_ICON_WIDTH);
}

int16_t substratum_renderer_scale_icon_y(
    const GSize* size,
    int16_t coord) {
  if (!size || !is_valid_design_y_coord(coord, DESIGN_ICON_HEIGHT)) {
    return 0;
  }

  return HELPER_SCALE_ROUND(coord, size->h, DESIGN_ICON_HEIGHT);
}

int16_t substratum_renderer_scale_icon_coord(
    const GSize* size,
    int16_t coord) {
  if (!size) {
    return 0;
  }

  if (!(is_valid_design_x_coord(coord, DESIGN_ICON_WIDTH) ||
      is_valid_design_y_coord(coord, DESIGN_ICON_HEIGHT))) {
    return 0;
  }

  int16_t chosen_dimension = HELPER_MIN(size->w, size->h);
  int16_t design_dimension = (chosen_dimension == size->w) ?
      DESIGN_ICON_WIDTH : DESIGN_ICON_HEIGHT;
  return HELPER_SCALE_ROUND(coord, chosen_dimension, design_dimension);
}

GPoint substratum_renderer_scale_icon_point(
    const GSize* size,
    int16_t x,
    int16_t y) {
  if (!(is_valid_design_x_coord(x, DESIGN_ICON_WIDTH) &&
      is_valid_design_y_coord(y, DESIGN_ICON_HEIGHT))) {
    return GPoint(0, 0);
  }

  if (size) {
    return GPoint(
        substratum_renderer_scale_icon_x(size, x),
        substratum_renderer_scale_icon_y(size, y));
  }

  return GPoint(x, y);
}

void substratum_renderer_draw_scaled_line(
    GContext* ctx,
    const GSize* size,
    int16_t x0,
    int16_t y0,
    int16_t x1,
    int16_t y1) {
  if (!ctx || !size) {
    return;
  }

  graphics_draw_line(
      ctx,
      substratum_renderer_scale_icon_point(size, x0, y0),
      substratum_renderer_scale_icon_point(size, x1, y1));
}