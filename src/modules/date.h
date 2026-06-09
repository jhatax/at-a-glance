#pragma once

#include <pebble.h>

#include "layout.h"

bool date_module_create(
    Layer* root,
    const WatchfaceSurface* surface);
void date_module_destroy(void);
void date_module_refresh(const WatchfaceSurface* surface);
