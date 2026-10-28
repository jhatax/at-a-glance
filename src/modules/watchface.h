#pragma once
#include <pebble.h>

#include "settings.h"

#define WATCHFACE_EVENT_LOCATION_BUFFER_SIZE 31

typedef enum {
  WATCHFACE_UPDATE_NONE = 0,
  WATCHFACE_REPAINT = 1 << 0,
  WATCHFACE_UPDATE_TIME = 1 << 1,
  WATCHFACE_UPDATE_DATE = 1 << 2,
  WATCHFACE_UPDATE_BATTERY = 1 << 3,
  WATCHFACE_UPDATE_CLIMATE = 1 << 4,
  WATCHFACE_UPDATE_LOCATION = 1 << 5,
  WATCHFACE_UPDATE_BLUETOOTH = 1 << 6,
  WATCHFACE_UPDATE_BT_LAYOUT = 1 << 7,
  WATCHFACE_UPDATE_BATTERY_ORIENTATION = 1 << 8,
  WATCHFACE_UPDATE_HORIZ_RULE = 1 << 9,
#ifdef PBL_HEALTH
  WATCHFACE_UPDATE_HEALTH = 1 << 10,
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
  WATCHFACE_DATA_WEATHER_UPDATE_MINUTES = 1 << 7,
  WATCHFACE_DATA_TIME_TICK = 1 << 8,
  WATCHFACE_DATA_DATE_TICK = 1 << 9,
  WATCHFACE_DATA_BATTERY_EVENT = 1 << 10,
  WATCHFACE_DATA_LOCATION = 1 << 11,
  WATCHFACE_DATA_BLUETOOTH = 1 << 12,
  WATCHFACE_DATA_BATTERY_ORIENTATION = 1 << 13,
#ifdef PBL_HEALTH
  WATCHFACE_DATA_HEALTH_EVENT = 1 << 14,
  WATCHFACE_DATA_STEPS_GOAL = 1 << 15,
  WATCHFACE_DATA_ONESHOT_BPM = 1 << 16,
  WATCHFACE_DATA_ONESHOT_STEPS = 1 << 17,
#endif
} WatchfaceDataMask;

#ifdef PBL_HEALTH
enum { WATCHFACE_ONESHOT_MESSAGE_KEY_BPM = 10020, WATCHFACE_ONESHOT_MESSAGE_KEY_STEPS = 10021 };
#endif

// Runtime supplies an atomic climate update packet. `is_complete` reflects
// runtime-owned transport completeness; climate owns applying valid weather or
// clearing stale weather when the packet is incomplete or domain-invalid.
typedef struct {
  bool complete : 1;
  bool received : 1;
  int celsius_tenths;
  int weather_condition;
  int is_day;
} ClimateUpdate;

typedef struct {
  char name[WATCHFACE_EVENT_LOCATION_BUFFER_SIZE];
  bool received : 1;
} LocationUpdate;

typedef struct {
  WatchfaceDataMask received;
  WatchfaceDataMask parsed;

  int time_format;
  int temp_unit;
  int hr_sample_minutes;
  int display_mode;
  int weather_update_minutes;
  int steps_goal;
  int battery_orientation;
  bool settings_changed : 1;

  // Climate data
  int temperature_celsius_tenths;
  int weather_condition;
  int is_day;

  LocationUpdate pin;
  void* event_context;
  ClimateUpdate current_climate;

#ifdef PBL_HEALTH
  int oneshot_bpm;
  int oneshot_steps;
#endif
} WatchfaceEventData;

// Create/destroy contract:
// `watchface` is a runtime module. Callers that invoke `watchface_create()` must
// also drive `watchface_destroy()` through normal window teardown, even when
// create returns false, because a failed create may leave partial module state
// that must be unwound. Calling create after a successful create is idempotent
// and returns true without rebuilding the active surface.
bool watchface_create(
    Window* window,
    const WatchfaceSettings* settings);
void watchface_destroy();
void watchface_maybe_relayout(WatchfaceUpdateMask* refresh);
void watchface_repaint(const WatchfaceEventData* data);
void watchface_refresh(
    WatchfaceUpdateMask updates,
    const WatchfaceEventData* data);
void watchface_apply_received_data(
    WatchfaceEventData* data,
    WatchfaceSettings* settings);
