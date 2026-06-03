#pragma once

#include <pebble.h>

#include "display.h"
#include "layout.h"

void health_module_create(
    Layer* root,
    const WatchfaceLayout* layout,
    GFont value_font,
    const VisualPalette* palette);
void health_module_destroy(void);
void health_module_refresh(const VisualPalette* palette);

#if defined(PBL_HEALTH)
void health_module_handle_event(HealthEventType event);
#endif
