#pragma once

#include <pebble.h>

#include "display.h"

void date_module_create(
    Layer* root,
    const GRect* frame,
    const VisualPalette* palette);
void date_module_destroy(void);
void date_module_refresh(const VisualPalette* palette);
