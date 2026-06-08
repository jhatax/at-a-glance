#pragma once

#include <pebble.h>
#include "display.h"

void draw_weather_icon(
    GContext* ctx,
    const GRect* frame,
    int16_t weather_condition,
    const VisualPalette* palette);
