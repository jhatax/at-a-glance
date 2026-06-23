#pragma once

#include <pebble.h>

#include "watchface_components.h"

// JS sends these same sentinel values when weather is unavailable.
#define WEATHER_TEMP_INVALID INT16_MIN
#define WEATHER_CONDITION_UNKNOWN -1

// Runtime supplies an atomic climate update packet. `is_complete` reflects
// runtime-owned transport completeness; climate owns applying valid weather or
// clearing stale weather when the packet is incomplete or domain-invalid.
typedef struct {
  bool is_complete;
  int celsius_tenths;
  int weather_condition;
  int is_day;
} ClimateUpdate;

bool climate_module_create(
    Layer* root,
    const WatchfaceTextSubstratum* text,
    const WatchfaceIconSubstratum* icon,
    GFont font);
void climate_module_destroy(void);
void climate_module_refresh(
    const ColorPalette* palette,
    uint8_t temp_unit);
void climate_module_set_weather(ClimateUpdate* update);
