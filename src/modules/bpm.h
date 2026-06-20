#pragma once

#include <pebble.h>
#include "watchface_debug.h"
#include "watchface_components.h"

#ifdef PBL_HEALTH
bool bpm_module_create(
    Layer* root,
    const WatchfaceTextSubstratum* text,
    const WatchfaceIconSubstratum* icon,
    GFont font);
void bpm_module_destroy(void);
void bpm_module_refresh(const ColorPalette* palette);

#if ATAGLANCE_DEBUG
void bpm_module_debug_set_bpm(int bpm);
void bpm_module_debug_clear_bpm(void);
#endif
// End Debug
#endif
// End Health Capability Check
