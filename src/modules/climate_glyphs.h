#pragma once

#include <pebble.h>

// Fully-resolved climate colors consumed by the glyph renderer.
typedef struct {
  GColor background;
  GColor normal;
  GColor outofrange;
  GColor sun;
  GColor cold;
  GColor cloud;
  GColor clear_ring;
  GColor clear_fill;
} ClimatePalette;

void draw_climate_icon(
    GContext* ctx,
    const GRect* frame,
    int16_t weather_condition,
    bool is_day,
    const ClimatePalette* climate_palette);
