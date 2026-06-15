#pragma once

#include <pebble.h>

#include "../c/ataglance.h"
#include "settings.h"

#if defined(PBL_HEALTH) && (DEBUG_ATAGLANCE == 1)
typedef enum {
  WATCHFACE_DEBUG_MESSAGE_KEY_BPM = 10020,
  WATCHFACE_DEBUG_MESSAGE_KEY_STEPS = 10021
} WatchfaceDebugMessageKey;
#endif

typedef enum {
  WATCHFACE_UPDATE_NONE = 0,
  WATCHFACE_UPDATE_TIME = 1 << 0,
  WATCHFACE_UPDATE_DATE = 1 << 1,
  WATCHFACE_UPDATE_BATTERY = 1 << 2,
  WATCHFACE_UPDATE_CLIMATE = 1 << 3,
  WATCHFACE_UPDATE_BACKGROUND = 1 << 4,
#ifdef PBL_HEALTH
  WATCHFACE_UPDATE_HEALTH = 1 << 5,
#endif
} WatchfaceUpdateMask;

bool watchface_create(
    Window* window,
    const WatchfaceSettings* settings);
void watchface_destroy();
void watchface_repaint(void);
void watchface_refresh(WatchfaceUpdateMask updates);
void watchface_set_temperature(int celsius_tenths);
void watchface_set_weather_condition(int weather_condition);
void watchface_set_is_day(bool is_day);

#if defined(PBL_HEALTH) && (DEBUG_ATAGLANCE == 1)
void watchface_debug_set_bpm(int bpm);
void watchface_debug_set_steps(int steps);
void watchface_debug_clear_health(void);
#endif
