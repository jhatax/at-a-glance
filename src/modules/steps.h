#pragma once
#include <pebble.h>

#ifdef PBL_HEALTH
#include "watchface_components.h"
#include "watchface_debug.h"

bool steps_module_create(Layer *root, const WatchfaceTextSubstratum *text,
                         const WatchfaceIconSubstratum *icon, GFont font);
void steps_module_destroy(void);
void steps_module_refresh(const ColorPalette *palette);

#if ATAGLANCE_DEBUG
void steps_module_debug_set_steps(int steps);
void steps_module_debug_clear_steps(void);
#endif

#endif
