#pragma once

#include <pebble.h>

#include "watchface_components.h"

bool date_module_create(Layer* root, const WatchfaceTextSubstratum* text, GFont font);
void date_module_destroy();
void date_module_refresh(const ColorPalette* palette);
