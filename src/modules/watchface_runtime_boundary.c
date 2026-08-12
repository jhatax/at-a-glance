#include "climate.h"
#include "settings.h"
#include "watchface.h"

#ifdef PBL_HEALTH
#include "bpm.h"
#include "steps.h"

static void apply_health_setting_data(
    const WatchfaceEventData* data,
    WatchfaceSettings* settings);
static void apply_oneshot_health_data(
    const WatchfaceEventData* data,
    WatchfaceUpdateMask* refresh);
#endif

static void apply_setting_data(
    const WatchfaceEventData* data,
    WatchfaceSettings* settings,
    WatchfaceUpdateMask* refresh);

static void apply_weather_or_location_data(
    const WatchfaceEventData* data,
    WatchfaceUpdateMask* refresh);

static void apply_subscribed_service_updates(
    const WatchfaceEventData* data,
    WatchfaceUpdateMask* refresh);

// Function definitions
static void apply_setting_data(
    const WatchfaceEventData* data,
    WatchfaceSettings* settings,
    WatchfaceUpdateMask* refresh) {
  if (!data || !settings || !refresh) {
    return;
  }

  if (data->parsed & WATCHFACE_DATA_DISPLAY_MODE) {
    if (DISPLAY_MODE_VALID(data->display_mode)) {
      SupportedDisplayModes mode = (SupportedDisplayModes)data->display_mode;
      if (settings->display_mode != mode) {
        settings->display_mode = mode;
        *refresh = (WatchfaceUpdateMask)(*refresh | WATCHFACE_REPAINT);
      }
    } else {
      APP_LOG(
          APP_LOG_LEVEL_WARNING,
          "Runtime received invalid DISPLAY_MODE: value=%d",
          data->display_mode);
    }
  }

  if (data->parsed & WATCHFACE_DATA_TIME_FORMAT) {
    if (TIME_FORMAT_VALID(data->time_format)) {
      if (settings->time_format != (uint8_t)data->time_format) {
        settings->time_format = (uint8_t)data->time_format;
        *refresh = (WatchfaceUpdateMask)(*refresh | WATCHFACE_UPDATE_TIME);
      }
    } else {
      APP_LOG(
          APP_LOG_LEVEL_WARNING,
          "Runtime received invalid TIME_FORMAT: value=%d",
          data->time_format);
    }
  }

  if (data->parsed & WATCHFACE_DATA_TEMP_UNIT) {
    if (TEMP_UNIT_VALID(data->temp_unit)) {
      if (settings->temp_unit != (uint8_t)data->temp_unit) {
        settings->temp_unit = (uint8_t)data->temp_unit;
        *refresh = (WatchfaceUpdateMask)(*refresh | WATCHFACE_UPDATE_CLIMATE);
      }
    } else {
      APP_LOG(
          APP_LOG_LEVEL_WARNING,
          "Runtime received invalid TEMP_UNIT: value=%d",
          data->temp_unit);
    }
  }

  if (data->parsed & WATCHFACE_DATA_WEATHER_UPDATE_MINUTES) {
    if (WEATHER_UPDATE_MINUTES_VALID(data->weather_update_minutes)) {
      settings->weather_update_minutes = (uint8_t)data->weather_update_minutes;
    } else {
      APP_LOG(
          APP_LOG_LEVEL_WARNING,
          "Runtime received invalid WEATHER_UPDATE_MINUTES: value=%d",
          data->weather_update_minutes);
    }
  }

#ifdef PBL_HEALTH
  if (data->parsed & WATCHFACE_DATA_STEPS_GOAL) {
    uint16_t d_s_g = (uint16_t)data->steps_goal;
    if (!STEPS_GOAL_VALID(d_s_g)) {
      // Set steps goal back to the default setting
      d_s_g = STEPS_GOAL_DEFAULT;
      APP_LOG(
          APP_LOG_LEVEL_WARNING,
          "Runtime reset STEPS_GOAL to default. Received: %d",
          data->steps_goal);
    }
    if (settings->steps_goal != d_s_g) {
      settings->steps_goal = d_s_g;
      *refresh = (WatchfaceUpdateMask)(*refresh | WATCHFACE_UPDATE_HEALTH);
    }
  }
#endif
}

static void maybe_apply_weather_update(
    const WatchfaceEventData* data,
    WatchfaceUpdateMask* refresh) {
  // This a function local const vs. a global const
  static const WatchfaceDataMask c_weather_mask =
      WATCHFACE_DATA_TEMPERATURE | WATCHFACE_DATA_WEATHER_CONDITION | WATCHFACE_DATA_IS_DAY;

  // Data was received; should be refreshed
  // If data couldn't be parsed, refresh will display "Unavailable state"
  *refresh = (WatchfaceUpdateMask)(*refresh | WATCHFACE_UPDATE_CLIMATE);

  WatchfaceDataMask weather_parsed = (WatchfaceDataMask)(data->parsed & c_weather_mask);

  ClimateUpdate climate = {0};
  if (weather_parsed == c_weather_mask) {
    climate.is_complete = true;
    climate.celsius_tenths = data->temperature_celsius_tenths;
    climate.weather_condition = data->weather_condition;
    climate.is_day = data->is_day;
  }

  climate_module_set_weather(&climate);
}

static void apply_location_data(
    const WatchfaceEventData* data,
    WatchfaceUpdateMask* refresh) {
  if (!data || !refresh) {
    return;
  }

  WatchfaceDataMask received = (WatchfaceDataMask)(data->received & WATCHFACE_DATA_LOCATION);
  if (received == WATCHFACE_DATA_NONE) {
    return;
  }

  // Data was received; should be refreshed
  // If data couldn't be parsed, refresh will show blank location
  *refresh = (WatchfaceUpdateMask)(*refresh | WATCHFACE_UPDATE_LOCATION);
  climate_module_set_location((char*)data->location);
}

static void apply_weather_or_location_data(
    const WatchfaceEventData* data,
    WatchfaceUpdateMask* refresh) {
  if (!data || !refresh) {
    return;
  }

  if (data->received & WATCHFACE_DATA_LOCATION) {
    apply_location_data(data, refresh);
  }

  // This a function local const vs. a global const
  static const WatchfaceDataMask c_weather_mask =
      WATCHFACE_DATA_TEMPERATURE | WATCHFACE_DATA_WEATHER_CONDITION | WATCHFACE_DATA_IS_DAY;

  if (data->received & c_weather_mask) {
    maybe_apply_weather_update(data, refresh);
  }
}

static void apply_subscribed_service_updates(
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

  if (data->parsed & WATCHFACE_DATA_BLUETOOTH) {
    *refresh = (WatchfaceUpdateMask)(*refresh | WATCHFACE_UPDATE_BLUETOOTH);
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
    WatchfaceSettings* settings) {
  if (!data || !settings) {
    return;
  }

  if (data->parsed & WATCHFACE_DATA_HR_SAMPLE_MINUTES) {
    if (HR_SAMPLE_MINUTES_VALID(data->hr_sample_minutes)) {
      if (settings->hr_sample_minutes != (uint8_t)data->hr_sample_minutes) {
        settings->hr_sample_minutes = (uint8_t)data->hr_sample_minutes;
      }
    } else {
      APP_LOG(
          APP_LOG_LEVEL_WARNING,
          "Runtime received invalid HR_SAMPLE_MINUTES: value=%d",
          data->hr_sample_minutes);
    }
  }
}

// Applying this data is based on user message
// Clearing runtime values is module responsibility
static void apply_oneshot_health_data(
    const WatchfaceEventData* data,
    WatchfaceUpdateMask* refresh) {
  if (!data || !refresh) {
    return;
  }

  if (data->parsed & WATCHFACE_DATA_ONESHOT_BPM) {
    bpm_module_oneshot_set_bpm(data->oneshot_bpm);
    *refresh = (WatchfaceUpdateMask)(*refresh | WATCHFACE_UPDATE_HEALTH);
  }

  if (data->parsed & WATCHFACE_DATA_ONESHOT_STEPS) {
    steps_module_oneshot_set_steps(data->oneshot_steps);
    *refresh = (WatchfaceUpdateMask)(*refresh | WATCHFACE_UPDATE_HEALTH);
  }
}

#endif

void watchface_apply_received_data(
    const WatchfaceEventData* data,
    WatchfaceSettings* settings) {
  if (!data || !settings) {
    return;
  }

  WatchfaceUpdateMask refresh = WATCHFACE_UPDATE_NONE;

  apply_setting_data(data, settings, &refresh);
  apply_weather_or_location_data(data, &refresh);
  apply_subscribed_service_updates(data, &refresh);

#ifdef PBL_HEALTH
  apply_health_setting_data(data, settings);
  apply_oneshot_health_data(data, &refresh);
#endif

  if (refresh & WATCHFACE_REPAINT) {
    watchface_repaint();
  } else if (refresh != WATCHFACE_UPDATE_NONE) {
    watchface_refresh(refresh);
  }
}
