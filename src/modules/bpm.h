#pragma once

#include <pebble.h>

#include "layout.h"

bool bpm_module_create(
    Layer* root,
    const WatchfaceSurface* surface);
void bpm_module_destroy(void);
void bpm_module_refresh(const WatchfaceSurface* surface);

#if defined(PBL_HEALTH)
void bpm_module_handle_event(HealthEventType event);
#endif
