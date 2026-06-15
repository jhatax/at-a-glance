#pragma once

#include <pebble.h>

#include "../c/ataglance.h"
#include "watchface_components.h"

#ifdef PBL_HEALTH
bool bpm_module_create(
    Layer* root,
    const WatchfaceSurface* surface);
void bpm_module_destroy(void);
void bpm_module_refresh(void);

#if defined(PBL_HEALTH) && (DEBUG_ATAGLANCE == 1)
void bpm_module_debug_set_bpm(int bpm);
void bpm_module_debug_clear_bpm(void);
#endif
#endif
