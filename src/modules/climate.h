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

bool climate_module_create(
    Layer* root,
    const WatchfaceTextSubstratum* text,
    const WatchfaceTextSubstratum* loc,
    const WatchfaceFrame* condition,
    GFont text_font,
    GFont location_font);
void climate_module_destroy();

void climate_module_refresh_climate(
    const ColorPalette* palette,
    uint8_t temp_unit,
    const ClimateUpdate* update);

void climate_module_refresh_location(
    const ColorPalette* palette,
    const char* location);
