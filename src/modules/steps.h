#pragma once

#include <pebble.h>

#include "layout.h"

bool steps_module_create(
    Layer* root,
    const WatchfaceSurface* surface);
void steps_module_destroy(void);
void steps_module_refresh(const WatchfaceSurface* surface);

#if defined(PBL_HEALTH)
void steps_module_handle_event(HealthEventType event);
#endif
