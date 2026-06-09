#pragma once

#include <pebble.h>

#include "layout.h"

bool time_module_create(
    Layer* root,
    const WatchfaceSurface* surface);
void time_module_destroy(void);
void time_module_refresh(
    const WatchfaceSurface* surface,
    uint8_t time_format);
