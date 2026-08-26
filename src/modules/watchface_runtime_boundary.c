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

static void apply_settings_data(
    const WatchfaceEventData* data,
    WatchfaceSettings* settings,
    WatchfaceUpdateMask* refresh,
    bool* settings_changed);

static void apply_weather_or_location_data(
    const WatchfaceEventData* data,
    WatchfaceUpdateMask* refresh);

static void apply_subscribed_service_updates(
    const WatchfaceEventData* data,
    WatchfaceUpdateMask* refresh);

// Function definitions
static void apply_settings_data(
    const WatchfaceEventData* data,
    WatchfaceSettings* settings,
    WatchfaceUpdateMask* refresh,
    bool* settings_changed) {
  if (!data || !settings || !refresh || !settings_changed) {
    return;
  }

  uint16_t previous = 0;

  if (data->parsed & WATCHFACE_DATA_DISPLAY_MODE) {
    SupportedDisplayModes previous = settings->display_mode;
    if (settings_set_display_mode(settings, data->display_mode) &&
        (settings->display_mode != previous)) {
      *refresh = (WatchfaceUpdateMask)(*refresh | WATCHFACE_REPAINT);
      *settings_changed = true;
    }
  }

  if (data->parsed & WATCHFACE_DATA_TIME_FORMAT) {
    previous = settings->time_format;
    if (settings_set_time_format(settings, data->time_format) &&
        (settings->time_format != previous)) {
      *refresh = (WatchfaceUpdateMask)(*refresh | WATCHFACE_UPDATE_TIME);
      *settings_changed = true;
    }
  }

  if (data->parsed & WATCHFACE_DATA_TEMP_UNIT) {
    previous = settings->temp_unit;
    if (settings_set_temp_unit(settings, data->temp_unit) && (settings->temp_unit != previous)) {
      *refresh = (WatchfaceUpdateMask)(*refresh | WATCHFACE_UPDATE_CLIMATE);
      *settings_changed = true;
    }
  }

  if (data->parsed & WATCHFACE_DATA_BATTERY_ORIENTATION) {
    previous = settings->battery_orientation;
    if (settings_set_battery_orientation(settings, data->battery_orientation) &&
        (settings->battery_orientation != previous)) {
      *refresh = (WatchfaceUpdateMask)(*refresh | WATCHFACE_UPDATE_BATTERY_ORIENTATION);
      *settings_changed = true;
    }
  }

  if (data->parsed & WATCHFACE_DATA_WEATHER_UPDATE_MINUTES) {
    previous = settings->battery_orientation;
    if (settings_set_weather_update_minutes(settings, data->weather_update_minutes) &&
        (settings->weather_update_minutes != previous)) {
      *settings_changed = true;
    }
  }

#ifdef PBL_HEALTH
  if (data->parsed & WATCHFACE_DATA_STEPS_GOAL) {
    previous = settings->steps_goal;
    if (settings_set_steps_goal(settings, data->steps_goal) && (settings->steps_goal != previous)) {
      *refresh = (WatchfaceUpdateMask)(*refresh | WATCHFACE_UPDATE_HEALTH);
      *settings_changed = true;
    }
  }

  if (data->parsed & WATCHFACE_DATA_HR_SAMPLE_MINUTES) {
    previous = settings->hr_sample_minutes;
    if (settings_set_hr_sample_minutes(settings, data->hr_sample_minutes) &&
        (settings->hr_sample_minutes != previous)) {
      *settings_changed = true;
    }
  }
#endif
}

static const WatchfaceDataMask c_weather_mask =
    WATCHFACE_DATA_TEMPERATURE | WATCHFACE_DATA_WEATHER_CONDITION | WATCHFACE_DATA_IS_DAY;

static void maybe_apply_weather_update(
    const WatchfaceEventData* data,
    WatchfaceUpdateMask* refresh) {
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
  climate_module_set_location(data->location);
}

static void apply_weather_or_location_data(
    const WatchfaceEventData* data,
    WatchfaceUpdateMask* refresh) {
  if (!data || !refresh) {
    return;
  }
  if (data->received & c_weather_mask) {
    maybe_apply_weather_update(data, refresh);
  }

  if (data->received & WATCHFACE_DATA_LOCATION) {
    apply_location_data(data, refresh);
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
    WatchfaceSettings* settings,
    bool* settings_changed) {
  if (!data || !settings || !settings_changed) {
    return;
  }

  WatchfaceUpdateMask refresh = WATCHFACE_UPDATE_NONE;

  apply_settings_data(data, settings, &refresh, settings_changed);
  apply_weather_or_location_data(data, &refresh);
  apply_subscribed_service_updates(data, &refresh);

#ifdef PBL_HEALTH
  apply_oneshot_health_data(data, &refresh);
#endif

  if (refresh & WATCHFACE_UPDATE_BATTERY_ORIENTATION) {
    // If a relayout is needed, this function should mark elements to be reset and redrawn
    watchface_maybe_relayout(&refresh);
  }

  if (refresh & WATCHFACE_REPAINT) {
    watchface_repaint();
  } else if (refresh != WATCHFACE_UPDATE_NONE) {
    watchface_refresh(refresh);
  }
}
