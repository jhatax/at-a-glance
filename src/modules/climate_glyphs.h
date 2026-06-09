#pragma once

#include <pebble.h>
#include "layout.h"

void draw_climate_icon(
    GContext* ctx,
    const GRect* frame,
    int16_t weather_condition,
    const ColorPalette* palette);
