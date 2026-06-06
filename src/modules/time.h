#pragma once

#include <pebble.h>

#include "display.h"

bool time_module_create(
    Layer* root,
    const GRect* frame,
    const VisualPalette* palette);
void time_module_destroy(void);
void time_module_refresh(
    uint8_t time_format,
    const VisualPalette* palette);
