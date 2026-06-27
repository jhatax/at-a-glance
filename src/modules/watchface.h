#pragma once

#include "settings.h"
#include "watchface_debug.h"
#include <pebble.h>

typedef enum {
  WATCHFACE_UPDATE_NONE = 0,
  WATCHFACE_UPDATE_TIME = 1 << 0,
  WATCHFACE_UPDATE_DATE = 1 << 1,
  WATCHFACE_UPDATE_BATTERY = 1 << 2,
  WATCHFACE_UPDATE_CLIMATE = 1 << 3,
#ifdef PBL_HEALTH
  WATCHFACE_UPDATE_HEALTH = 1 << 5,
#endif
} WatchfaceUpdateMask;

typedef enum {
  WATCHFACE_DATA_NONE = 0,
  WATCHFACE_DATA_TIME_FORMAT = 1 << 0,
  WATCHFACE_DATA_TEMP_UNIT = 1 << 1,
  WATCHFACE_DATA_TEMPERATURE = 1 << 2,
  WATCHFACE_DATA_WEATHER_CONDITION = 1 << 3,
  WATCHFACE_DATA_IS_DAY = 1 << 4,
  WATCHFACE_DATA_HR_SAMPLE_MINUTES = 1 << 5,
  WATCHFACE_DATA_DISPLAY_MODE = 1 << 6,
  WATCHFACE_DATA_TIME_TICK = 1 << 7,
  WATCHFACE_DATA_DATE_TICK = 1 << 8,
  WATCHFACE_DATA_BATTERY_EVENT = 1 << 9,
#ifdef PBL_HEALTH
  WATCHFACE_DATA_HEALTH_EVENT = 1 << 10,
  WATCHFACE_DATA_STEPS_GOAL = 1 << 11,
#endif
#if defined(PBL_HEALTH) && ATAGLANCE_DEBUG
  WATCHFACE_DATA_DEBUG_BPM = 1 << 12,
  WATCHFACE_DATA_DEBUG_STEPS = 1 << 13,
#endif
} WatchfaceDataMask;

typedef struct {
  WatchfaceDataMask received;
  WatchfaceDataMask parsed;

  int time_format;
  int temp_unit;
  int temperature_celsius_tenths;
  int weather_condition;
  int is_day;
  int hr_sample_minutes;
  int display_mode;
  int steps_goal;
  int time_units_changed;
#ifdef PBL_HEALTH
  int health_event;
#endif

#if defined(PBL_HEALTH) && ATAGLANCE_DEBUG
  int debug_bpm;
  int debug_steps;
#endif
} WatchfaceEventData;

#if defined(PBL_HEALTH) && ATAGLANCE_DEBUG
enum { WATCHFACE_DEBUG_MESSAGE_KEY_BPM = 10020, WATCHFACE_DEBUG_MESSAGE_KEY_STEPS = 10021 };
#endif

// Create/destroy contract:
// `watchface` is a runtime module. Callers that invoke `watchface_create()` must
// also drive `watchface_destroy()` through normal window teardown, even when
// create returns false, because a failed create may leave partial module state
// that must be unwound. Calling create after a successful create is idempotent
// and returns true without rebuilding the active surface.
bool watchface_create(Window *window, const WatchfaceSettings *settings);
void watchface_destroy();
void watchface_repaint(void);
void watchface_refresh(WatchfaceUpdateMask updates);
void watchface_apply_received_data(const WatchfaceEventData *data, WatchfaceSettings *settings);
