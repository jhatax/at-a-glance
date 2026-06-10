#pragma once

#include <pebble.h>

#include "../c/ataglance.h"
#include "watchface_components.h"

bool steps_module_create(
    Layer* root,
    const WatchfaceSurface* surface);
void steps_module_destroy(void);
void steps_module_refresh(const WatchfaceSurface* surface);

#if defined(PBL_HEALTH) && defined(DEBUG_ATAGLANCE)
void steps_module_debug_set_steps(int steps);
#endif
