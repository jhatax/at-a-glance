#pragma once

#include <pebble.h>

#include "watchface_components.h"

bool time_module_create(
    Layer* root,
    const WatchfaceSurface* surface);
void time_module_destroy(void);
void time_module_refresh(uint8_t time_format);
