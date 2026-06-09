#pragma once

#include <pebble.h>

#include "settings.h"

bool watchface_composer_create(
    Window* window,
    const WatchfaceSettings* settings);
void watchface_composer_destroy();
void watchface_composer_refresh();
void watchface_composer_handle_tick(TimeUnits units_changed);
void watchface_composer_update_battery(const BatteryChargeState* state);
void watchface_composer_update_temp(int celsius_tenths, uint8_t temp_unit);
void watchface_composer_update_weather_condition(int weather_condition, uint8_t temp_unit);

#if defined(PBL_HEALTH)
void watchface_composer_handle_health_event(HealthEventType event);
#endif
