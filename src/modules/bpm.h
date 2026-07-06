#pragma once

#include <pebble.h>

#include "watchface_components.h"
#include "watchface_debug.h"

#ifdef PBL_HEALTH
bool bpm_module_create(Layer* root,
  const WatchfaceTextSubstratum* text,
  const WatchfaceIconSubstratum* icon,
  GFont font);
void bpm_module_destroy();
void bpm_module_refresh(const ColorPalette* palette);

void bpm_module_oneshot_set_bpm(int bpm);
void bpm_module_oneshot_clear_bpm();
#endif
// End Health Capability Check
