#pragma once

#include <pebble.h>

#include "../c/ataglance.h"
#include "watchface_components.h"

bool steps_module_create(
    Layer* root,
    const WatchfaceSurface* surface);
void steps_module_destroy(void);
void steps_module_refresh(const WatchfaceSurface* surface);

#ifdef PBL_HEALTH
void steps_module_handle_event(HealthEventType event);
#ifdef DEBUG_ATAGLANCE
void steps_module_debug_set_value(int steps);
#endif
#endif
