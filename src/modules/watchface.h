#pragma once

#include <pebble.h>

#include "settings.h"

bool watchface_create(
    Window* window,
    const WatchfaceSettings* settings);
void watchface_destroy();
void watchface_refresh();
void watchface_handle_tick(TimeUnits units_changed);
void watchface_update_battery(const BatteryChargeState* state);
void watchface_update_temp(int celsius_tenths, uint8_t temp_unit);
void watchface_update_weather_condition(int weather_condition, uint8_t temp_unit);

#if defined(PBL_HEALTH)
void watchface_handle_health_event(HealthEventType event);
#endif
