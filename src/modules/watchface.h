#pragma once

#include <pebble.h>

#include "../c/ataglance.h"
#include "settings.h"

#if defined(PBL_HEALTH) && defined(DEBUG_ATAGLANCE)
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
#ifdef PBL_HEALTH
  WATCHFACE_UPDATE_HEALTH = 1 << 4,
#endif
  WATCHFACE_UPDATE_DISPLAY_MODE = 1 << 5,
  WATCHFACE_UPDATE_ALL = WATCHFACE_UPDATE_TIME |
      WATCHFACE_UPDATE_DATE |
      WATCHFACE_UPDATE_BATTERY |
      WATCHFACE_UPDATE_CLIMATE |
#ifdef PBL_HEALTH
      WATCHFACE_UPDATE_HEALTH |
#endif
      WATCHFACE_UPDATE_DISPLAY_MODE
} WatchfaceUpdateMask;

bool watchface_create(
    Window* window,
    const WatchfaceSettings* settings);
void watchface_destroy();
void watchface_refresh(WatchfaceUpdateMask updates);
void watchface_set_temperature(int celsius_tenths);
void watchface_set_weather_condition(int weather_condition);

#if defined(PBL_HEALTH) && defined(DEBUG_ATAGLANCE)
void watchface_debug_set_bpm(int bpm);
void watchface_debug_set_steps(int steps);
void watchface_debug_clear_health(void);
#endif
