#pragma once
#include <pebble.h>

#ifdef PBL_HEALTH
#include "watchface_components.h"
#include "watchface_debug.h"

bool steps_module_create(Layer* root,
  const WatchfaceTextSubstratum* text,
  const WatchfaceIconSubstratum* icon,
  const GRect* progress,
  GFont font);
void steps_module_destroy();
void steps_module_refresh(const ColorPalette* palette, uint16_t steps_goal);

void steps_module_oneshot_set_steps(int steps);
#endif
