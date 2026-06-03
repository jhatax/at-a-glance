#pragma once

#include <pebble.h>

#include "../c/ataglance.h"

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
