#include "watchface.h"
#include "climate.h"

#ifdef PBL_HEALTH
#include "bpm.h"
#include "steps.h"
#endif

typedef enum {
  WATCHFACE_RUNTIME_UPDATE_NONE = 0,
  WATCHFACE_RUNTIME_UPDATE_SETTINGS = 1 << 0,
  WATCHFACE_RUNTIME_UPDATE_WEATHER = 1 << 1,
#if defined(PBL_HEALTH) && ATAGLANCE_DEBUG
  WATCHFACE_RUNTIME_UPDATE_DEBUG_HEALTH = 1 << 2,
#endif
} WatchfaceRuntimeUpdateMask;

static void apply_setting_data(
    const WatchfaceEventData* data,
    WatchfaceSettings* settings,
    WatchfaceRuntimeUpdateMask* runtime_updates,
    WatchfaceUpdateMask* refresh,
    bool* repaint);
static void apply_weather_data(
    const WatchfaceEventData* data,
    WatchfaceRuntimeUpdateMask* runtime_updates,
    WatchfaceUpdateMask* refresh);
static void apply_service_event_data(
    const WatchfaceEventData* data,
    WatchfaceUpdateMask* refresh);
#ifdef PBL_HEALTH
static void apply_health_setting_data(
    const WatchfaceEventData* data,
    WatchfaceSettings* settings,
    WatchfaceRuntimeUpdateMask* runtime_updates);
#if ATAGLANCE_DEBUG
static void apply_debug_health_data(
    const WatchfaceEventData* data,
    WatchfaceRuntimeUpdateMask* runtime_updates,
    WatchfaceUpdateMask* refresh);
#endif
#endif

static void apply_setting_data(
    const WatchfaceEventData* data,
    WatchfaceSettings* settings,
    WatchfaceRuntimeUpdateMask* runtime_updates,
    WatchfaceUpdateMask* refresh,
    bool* repaint) {
  if (!data || !settings || !runtime_updates || !refresh || !repaint) {
    return;
  }

  if (data->parsed & WATCHFACE_DATA_TIME_FORMAT) {
    if (TIME_FORMAT_VALID(data->time_format)) {
      if (settings->time_format != (uint8_t)data->time_format) {
        settings->time_format = (uint8_t)data->time_format;
        *runtime_updates =
            (WatchfaceRuntimeUpdateMask)(*runtime_updates | WATCHFACE_RUNTIME_UPDATE_SETTINGS);
        *refresh = (WatchfaceUpdateMask)(*refresh | WATCHFACE_UPDATE_TIME);
      }
    } else {
      APP_LOG(APP_LOG_LEVEL_WARNING,
              "Runtime received invalid TIME_FORMAT: value=%d",
              data->time_format);
    }
  }

  if (data->parsed & WATCHFACE_DATA_TEMP_UNIT) {
    if (TEMP_UNIT_VALID(data->temp_unit)) {
      if (settings->temp_unit != (uint8_t)data->temp_unit) {
        settings->temp_unit = (uint8_t)data->temp_unit;
        *runtime_updates =
            (WatchfaceRuntimeUpdateMask)(*runtime_updates | WATCHFACE_RUNTIME_UPDATE_SETTINGS);
        *refresh = (WatchfaceUpdateMask)(*refresh | WATCHFACE_UPDATE_CLIMATE);
      }
    } else {
      APP_LOG(APP_LOG_LEVEL_WARNING,
              "Runtime received invalid TEMP_UNIT: value=%d",
              data->temp_unit);
    }
  }

  if (data->parsed & WATCHFACE_DATA_DISPLAY_MODE) {
    if (DISPLAY_MODE_VALID(data->display_mode)) {
      if (settings->display_mode != (uint8_t)data->display_mode) {
        settings->display_mode = (uint8_t)data->display_mode;
        *runtime_updates =
            (WatchfaceRuntimeUpdateMask)(*runtime_updates | WATCHFACE_RUNTIME_UPDATE_SETTINGS);
        *repaint = true;
      }
    } else {
      APP_LOG(APP_LOG_LEVEL_WARNING,
              "Runtime received invalid DISPLAY_MODE: value=%d",
              data->display_mode);
    }
  }
}

static void apply_weather_data(
    const WatchfaceEventData* data,
    WatchfaceRuntimeUpdateMask* runtime_updates,
    WatchfaceUpdateMask* refresh) {
  if (!data || !runtime_updates || !refresh) {
    return;
  }

  WatchfaceDataMask weather_mask =
      WATCHFACE_DATA_TEMPERATURE |
      WATCHFACE_DATA_WEATHER_CONDITION |
      WATCHFACE_DATA_IS_DAY;
  WatchfaceDataMask weather_received =
      (WatchfaceDataMask)(data->received & weather_mask);
  WatchfaceDataMask weather_parsed =
      (WatchfaceDataMask)(data->parsed & weather_mask);

  if (weather_received == WATCHFACE_DATA_NONE) {
    return;
  }

  *runtime_updates =
      (WatchfaceRuntimeUpdateMask)(*runtime_updates | WATCHFACE_RUNTIME_UPDATE_WEATHER);
  *refresh = (WatchfaceUpdateMask)(*refresh | WATCHFACE_UPDATE_CLIMATE);

  if (weather_parsed != weather_mask) {
    climate_module_set_weather_unavailable();
    return;
  }

  climate_module_set_weather(
      data->temperature_celsius_tenths,
      data->weather_condition,
      data->is_day);
}

static void apply_service_event_data(
    const WatchfaceEventData* data,
    WatchfaceUpdateMask* refresh) {
  if (!data || !refresh) {
    return;
  }

  if (data->parsed & WATCHFACE_DATA_TIME_TICK) {
    *refresh = (WatchfaceUpdateMask)(*refresh | WATCHFACE_UPDATE_TIME);
  }
  if (data->parsed & WATCHFACE_DATA_DATE_TICK) {
    *refresh = (WatchfaceUpdateMask)(*refresh | WATCHFACE_UPDATE_DATE);
  }
  if (data->parsed & WATCHFACE_DATA_BATTERY_EVENT) {
    *refresh = (WatchfaceUpdateMask)(*refresh | WATCHFACE_UPDATE_BATTERY);
  }

#ifdef PBL_HEALTH
  if (data->parsed & WATCHFACE_DATA_HEALTH_EVENT) {
    *refresh = (WatchfaceUpdateMask)(*refresh | WATCHFACE_UPDATE_HEALTH);
  }
#endif
}

#ifdef PBL_HEALTH
static void apply_health_setting_data(
    const WatchfaceEventData* data,
    WatchfaceSettings* settings,
    WatchfaceRuntimeUpdateMask* runtime_updates) {
  if (!data || !settings || !runtime_updates) {
    return;
  }

  if (data->parsed & WATCHFACE_DATA_HR_SAMPLE_MINUTES) {
    if (HR_SAMPLE_MINUTES_VALID(data->hr_sample_minutes)) {
      if (settings->hr_sample_minutes != (uint8_t)data->hr_sample_minutes) {
        settings->hr_sample_minutes = (uint8_t)data->hr_sample_minutes;
        *runtime_updates =
            (WatchfaceRuntimeUpdateMask)(*runtime_updates | WATCHFACE_RUNTIME_UPDATE_SETTINGS);
      }
    } else {
      APP_LOG(APP_LOG_LEVEL_WARNING,
              "Runtime received invalid HR_SAMPLE_MINUTES: value=%d",
              data->hr_sample_minutes);
    }
  }
}

#if ATAGLANCE_DEBUG
static void apply_debug_health_data(
    const WatchfaceEventData* data,
    WatchfaceRuntimeUpdateMask* runtime_updates,
    WatchfaceUpdateMask* refresh) {
  if (!data || !runtime_updates || !refresh) {
    return;
  }

  if (data->parsed & WATCHFACE_DATA_DEBUG_BPM) {
    bpm_module_debug_set_bpm(data->debug_bpm);
    *runtime_updates =
        (WatchfaceRuntimeUpdateMask)(*runtime_updates | WATCHFACE_RUNTIME_UPDATE_DEBUG_HEALTH);
    *refresh = (WatchfaceUpdateMask)(*refresh | WATCHFACE_UPDATE_HEALTH);
  }

  if (data->parsed & WATCHFACE_DATA_DEBUG_STEPS) {
    steps_module_debug_set_steps(data->debug_steps);
    *runtime_updates =
        (WatchfaceRuntimeUpdateMask)(*runtime_updates | WATCHFACE_RUNTIME_UPDATE_DEBUG_HEALTH);
    *refresh = (WatchfaceUpdateMask)(*refresh | WATCHFACE_UPDATE_HEALTH);
  }
}
#endif
#endif

void watchface_apply_received_data(
    const WatchfaceEventData* data,
    WatchfaceSettings* settings) {
  if (!data || !settings) {
    return;
  }

  WatchfaceRuntimeUpdateMask runtime_updates = WATCHFACE_RUNTIME_UPDATE_NONE;
  WatchfaceUpdateMask refresh = WATCHFACE_UPDATE_NONE;
  bool repaint = false;

  apply_setting_data(data, settings, &runtime_updates, &refresh, &repaint);
  apply_weather_data(data, &runtime_updates, &refresh);
  apply_service_event_data(data, &refresh);

#ifdef PBL_HEALTH
  apply_health_setting_data(data, settings, &runtime_updates);
#if ATAGLANCE_DEBUG
  apply_debug_health_data(data, &runtime_updates, &refresh);
#endif
#endif

  (void)runtime_updates;

  if (repaint) {
    watchface_repaint();
  } else if (refresh != WATCHFACE_UPDATE_NONE) {
    watchface_refresh(refresh);
  }
}
