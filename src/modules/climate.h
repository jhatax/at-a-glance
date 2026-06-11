#pragma once

#include <pebble.h>

#include "watchface_components.h"

// JS sends these same sentinel values when weather is unavailable.
#define WEATHER_TEMP_INVALID INT16_MIN
#define WEATHER_CONDITION_UNKNOWN -1

bool climate_module_create(
    Layer* root,
    const WatchfaceSurface* surface,
    uint8_t temp_unit);
void climate_module_destroy(void);
void climate_module_refresh(uint8_t temp_unit);
void climate_module_set_temperature(int celsius_tenths);
void climate_module_set_condition(int weather_condition);
