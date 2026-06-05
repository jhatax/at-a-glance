#pragma once

#include <pebble.h>

#include "display.h"

// JS sends these same sentinel values when weather is unavailable.
#define WEATHER_TEMP_INVALID INT16_MIN
#define WEATHER_CONDITION_UNKNOWN -1

void weather_icon_init(void);
void weather_icon_create(
    Layer* root,
    const GRect* frame,
    const VisualPalette* palette);
void weather_icon_destroy(void);
void weather_icon_set_condition(int16_t weather_condition);
void weather_icon_update_display(
    bool is_temperature_available,
    const VisualPalette* palette);
void weather_icon_mark_dirty(void);
