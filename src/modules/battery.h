#pragma once

#include <pebble.h>

#include "watchface_components.h"

bool battery_module_create(
    Layer* root,
    const WatchfaceSurface* surface);
void battery_module_destroy(void);
void battery_module_refresh(const WatchfaceSurface* surface);
