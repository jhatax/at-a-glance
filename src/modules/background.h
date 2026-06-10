#pragma once

#include <pebble.h>

#include "watchface_components.h"

bool background_module_create(
    Layer* root,
    const WatchfaceSurface* surface);
void background_module_destroy(void);
void background_module_refresh(const WatchfaceSurface* surface);
