#pragma once

#include <pebble.h>
#include "watchface_components.h"

#ifdef PBL_HEALTH
bool steps_module_create(
    Layer* root,
    const WatchfaceSurface* surface);
void steps_module_destroy(void);
void steps_module_refresh(void);

#if ATAGLANCE_DEBUG
void steps_module_debug_set_steps(int steps);
void steps_module_debug_clear_steps(void);
#endif

#endif
