#pragma once

#include <pebble.h>

#include "watchface.h"
#include "watchface_components.h"

// Weather AppMessage wire sentinels. PebbleKit JS mirrors these values when
// weather is unavailable; do not change them without updating both sides of
// the transport contract.
enum {
  CLIMATE_CONDITION_OUTOFRANGE = -1,
  CLIMATE_CONDITION_MIN = 0,
  CLIMATE_CONDITION_MAX = 99,
};

// This macro should reside here where the enum is defined vs. in climate.c
#define CLIMATE_CONDITION_IS_VALID(cond) \
  (HELPER_VALUE_IN_RANGE((cond), CLIMATE_CONDITION_MIN, CLIMATE_CONDITION_MAX))

// Runtime supplies an atomic climate update packet. `is_complete` reflects
// runtime-owned transport completeness; climate owns applying valid weather or
// clearing stale weather when the packet is incomplete or domain-invalid.
typedef struct {
  bool is_complete;
  int celsius_tenths;
  int weather_condition;
  int is_day;
} ClimateUpdate;

bool climate_module_create(Layer* root,
    const WatchfaceTextSubstratum* text,
    const WatchfaceTextSubstratum* loc,
    const WatchfaceIconSubstratum* condition,
    GFont text_font,
    GFont location_font);
void climate_module_destroy();
void climate_module_refresh(const ColorPalette* palette,
    const WatchfaceUpdateMask refreshed,
    uint8_t temp_unit);
void climate_module_set_weather(ClimateUpdate* update);
void climate_module_set_location(char* location);
