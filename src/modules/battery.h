#pragma once

#include <pebble.h>

#include "display.h"
#include "layout.h"

bool battery_module_create(
    Layer* root,
    const WatchfaceLayout* layout,
    const VisualPalette* palette);
void battery_module_destroy(void);
void battery_module_refresh(const VisualPalette* palette);
void battery_module_set_state(const BatteryChargeState* state);
