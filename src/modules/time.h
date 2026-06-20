#pragma once

#include <pebble.h>

#include "watchface_components.h"

bool time_module_create(
    Layer* root,
    const WatchfaceTextSubstratum* text,
    GFont font);
void time_module_destroy(void);
void time_module_refresh(
    const ColorPalette* palette,
    uint8_t time_format);
