#pragma once

#include <pebble.h>

#include "../c/ataglance.h"
#include "watchface_components.h"

bool bpm_module_create(
    Layer* root,
    const WatchfaceSurface* surface);
void bpm_module_destroy(void);
void bpm_module_refresh(const WatchfaceSurface* surface);

#if defined(PBL_HEALTH) && defined(DEBUG_ATAGLANCE)
void bpm_module_debug_set_bpm(int bpm);
void bpm_module_debug_clear_bpm(void);
#endif
