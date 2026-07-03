#pragma once

#include <pebble.h>

#include "glyph_lab_components.h"

bool glyph_lab_glyphs_init(void);
void glyph_lab_glyphs_deinit(void);

void glyph_lab_select_palette(ColorPalette *palette, bool is_light_mode);

void glyph_lab_draw_climate_icon(GContext *ctx, const GRect *frame, int16_t weather_condition,
                                 bool is_day, const ColorPalette *palette);
