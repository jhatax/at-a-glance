#pragma once

#include <pebble.h>

#include "display.h"
#include "layout.h"

// JS sends these same sentinel values when weather is unavailable.
#define WEATHER_TEMP_INVALID INT16_MIN
#define WEATHER_CONDITION_UNKNOWN -1

void weather_module_init(void);
bool weather_module_create(
    Layer* root,
    const WatchfaceLayout* layout,
    uint8_t temp_unit,
    const VisualPalette* palette);
void weather_module_destroy(void);
void weather_module_refresh(
    uint8_t temp_unit,
    const VisualPalette* palette);
void weather_module_set_temperature(
    int celsius_tenths,
    uint8_t temp_unit,
    const VisualPalette* palette);
void weather_module_set_condition(
    int weather_condition,
    uint8_t temp_unit,
    const VisualPalette* palette);
