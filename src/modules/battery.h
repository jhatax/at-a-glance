#pragma once

#include <pebble.h>

#include "watchface_components.h"

bool battery_module_create(
    Layer* root,
    const WatchfaceBatteryStratum* battery);
void battery_module_destroy();
void battery_module_refresh(const ColorPalette* palette);
