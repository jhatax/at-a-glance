#pragma once

#include <pebble.h>

#include "watchface_components.h"

void draw_climate_icon(
    GContext* ctx,
    const GRect* frame,
    int16_t weather_condition,
    const ColorPalette* palette);
