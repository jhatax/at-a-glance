#include "glyph_lab_renderer.h"

#include "glyph_lab_components.h"
#include "glyph_lab_helper.h"

static bool is_valid_design_x_coord(int16_t x, int16_t design_width) {
  return x >= 0 && x <= design_width;
}

static bool is_valid_design_y_coord(int16_t y, int16_t design_height) {
  return y >= 0 && y <= design_height;
}

int16_t glyph_lab_scale_icon_x(const GSize *size, int16_t coord) {
  if (!size || !is_valid_design_x_coord(coord, DESIGN_ICON_WIDTH)) {
    return 0;
  }

  return HELPER_SCALE_ROUND(coord, size->w, DESIGN_ICON_WIDTH);
}

int16_t glyph_lab_scale_icon_y(const GSize *size, int16_t coord) {
  if (!size || !is_valid_design_y_coord(coord, DESIGN_ICON_HEIGHT)) {
    return 0;
  }

  return HELPER_SCALE_ROUND(coord, size->h, DESIGN_ICON_HEIGHT);
}

int16_t glyph_lab_scale_icon_coord(const GSize *size, int16_t coord) {
  if (!size) {
    return 0;
  }

  if (!(is_valid_design_x_coord(coord, DESIGN_ICON_WIDTH) ||
        is_valid_design_y_coord(coord, DESIGN_ICON_HEIGHT))) {
    return 0;
  }

  int16_t chosen_dimension = HELPER_MIN(size->w, size->h);
  int16_t design_dimension = chosen_dimension == size->w ? DESIGN_ICON_WIDTH : DESIGN_ICON_HEIGHT;

  return HELPER_SCALE_ROUND(coord, chosen_dimension, design_dimension);
}

GPoint glyph_lab_scale_icon_point(const GSize *size, int16_t x, int16_t y) {
  if (!(is_valid_design_x_coord(x, DESIGN_ICON_WIDTH) &&
        is_valid_design_y_coord(y, DESIGN_ICON_HEIGHT))) {
    return GPointZero;
  }

  if (!size) {
    return GPoint(x, y);
  }

  return GPoint(glyph_lab_scale_icon_x(size, x), glyph_lab_scale_icon_y(size, y));
}

void glyph_lab_draw_scaled_line(GContext *ctx, const GSize *size, int16_t x0, int16_t y0,
                                int16_t x1, int16_t y1) {
  if (!ctx || !size) {
    return;
  }

  graphics_draw_line(ctx, glyph_lab_scale_icon_point(size, x0, y0),
                     glyph_lab_scale_icon_point(size, x1, y1));
}
