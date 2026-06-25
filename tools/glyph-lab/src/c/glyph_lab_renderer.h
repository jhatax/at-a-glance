#pragma once

#include <pebble.h>

#define GLYPH_LAB_ICON_STROKE_WIDTH(frame_min) (((frame_min) < 20) ? 1 : 2)

int16_t glyph_lab_scale_icon_x(const GSize *size, int16_t coord);

int16_t glyph_lab_scale_icon_y(const GSize *size, int16_t coord);

int16_t glyph_lab_scale_icon_coord(const GSize *size, int16_t coord);

GPoint glyph_lab_scale_icon_point(const GSize *size, int16_t x, int16_t y);

void glyph_lab_draw_scaled_line(GContext *ctx, const GSize *size, int16_t x0, int16_t y0,
                                int16_t x1, int16_t y1);
