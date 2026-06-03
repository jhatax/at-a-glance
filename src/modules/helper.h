#pragma once

#include <pebble.h>

#define HELPER_ICON_GRID_SIZE 28

static inline int16_t helper_icon_draw_size(
    const GSize* bounds_size) {
  if (!bounds_size) {
    return 0;
  }

  return bounds_size->w < bounds_size->h ?
      bounds_size->w : bounds_size->h;
}

static inline int16_t helper_scale_icon_coord(
    const GSize* bounds_size,
    int16_t coord) {
  int16_t draw_size = helper_icon_draw_size(bounds_size);
  return (coord * draw_size) / HELPER_ICON_GRID_SIZE;
}

static inline int16_t helper_scale_icon_x(
    const GSize* bounds_size,
    int16_t coord) {
  if (!bounds_size) {
    return 0;
  }

  return (coord * bounds_size->w) / HELPER_ICON_GRID_SIZE;
}

static inline int16_t helper_scale_icon_y(
    const GSize* bounds_size,
    int16_t coord) {
  if (!bounds_size) {
    return 0;
  }

  return (coord * bounds_size->h) / HELPER_ICON_GRID_SIZE;
}

static inline GPoint helper_get_scaled_icon_point(
    const GSize* bounds_size,
    int16_t x,
    int16_t y) {
  return GPoint(helper_scale_icon_x(bounds_size, x),
                helper_scale_icon_y(bounds_size, y));
}

static inline void helper_draw_scaled_icon_line(
    GContext* ctx,
    const GSize* bounds_size,
    int16_t x0,
    int16_t y0,
    int16_t x1,
    int16_t y1) {
  graphics_draw_line(
      ctx,
      helper_get_scaled_icon_point(bounds_size, x0, y0),
      helper_get_scaled_icon_point(bounds_size, x1, y1));
}
