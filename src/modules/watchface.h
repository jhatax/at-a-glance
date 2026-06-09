#pragma once

#include <pebble.h>

#include "../c/ataglance.h"
#include "settings.h"

#ifdef DEBUG_ATAGLANCE
typedef enum {
  WATCHFACE_DEBUG_MESSAGE_KEY_BPM = 10020,
  WATCHFACE_DEBUG_MESSAGE_KEY_STEPS = 10021
} WatchfaceDebugMessageKey;
#endif

bool watchface_create(
    Window* window,
    const WatchfaceSettings* settings);
void watchface_destroy();
void watchface_refresh();
void watchface_handle_tick(TimeUnits units_changed);
void watchface_update_battery(const BatteryChargeState* state);
void watchface_update_temp(int celsius_tenths, uint8_t temp_unit);
void watchface_update_weather_condition(int weather_condition, uint8_t temp_unit);

#ifdef PBL_HEALTH
void watchface_handle_health_event(HealthEventType event);
#ifdef DEBUG_ATAGLANCE
void watchface_debug_update_bpm(int bpm);
void watchface_debug_update_steps(int steps);
#endif
#endif
