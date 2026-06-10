#pragma once

#include <pebble.h>

#include "../c/ataglance.h"
#include "watchface_components.h"

bool bpm_module_create(
    Layer* root,
    const WatchfaceSurface* surface);
void bpm_module_destroy(void);
void bpm_module_refresh(const WatchfaceSurface* surface);

#ifdef PBL_HEALTH
void bpm_module_handle_event(HealthEventType event);
#ifdef DEBUG_ATAGLANCE
void bpm_module_debug_set_value(int bpm);
#endif
#endif
