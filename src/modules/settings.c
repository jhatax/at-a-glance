#include "settings.h"

// The ID / key that specifies where settings are persisted in storage
static const uint32_t c_key_current_settings = 3;
static const uint32_t c_key_prior_settings = 2;

static void settings_apply_defaults(
    WatchfaceSettings* settings) {
  if (!settings) {
    return;
  }

  *settings = (WatchfaceSettings){
      .temp_unit = TEMP_UNIT_DEFAULT,
      .display_mode = DISPLAY_MODE_DEFAULT,
      .weather_update_minutes = WEATHER_UPDATE_MINUTES_DEFAULT,
      .battery_orientation = BATTERY_ORIENTATION_DEFAULT,
#ifdef PBL_HEALTH
      .hr_sample_minutes = HR_SAMPLE_MINUTES_DEFAULT,
      .steps_goal = STEPS_GOAL_DEFAULT,
#endif
  };
}

static void settings_sanitize(
    WatchfaceSettings* settings) {
  if (!settings) {
    return;
  }
  if (!TEMP_UNIT_VALID(settings->temp_unit)) {
    settings->temp_unit = TEMP_UNIT_DEFAULT;
  }
#ifdef PBL_HEALTH
  if (!HR_SAMPLE_MINUTES_VALID(settings->hr_sample_minutes)) {
    settings->hr_sample_minutes = HR_SAMPLE_MINUTES_DEFAULT;
  }
  if (!STEPS_GOAL_VALID(settings->steps_goal)) {
    settings->steps_goal = STEPS_GOAL_DEFAULT;
  }
#endif
  if (!DISPLAY_MODE_VALID(settings->display_mode)) {
    settings->display_mode = DISPLAY_MODE_DEFAULT;
  }
  if (!WEATHER_UPDATE_MINUTES_VALID(settings->weather_update_minutes)) {
    uint8_t update = HELPER_CLAMP_TO_RANGE(
        settings->weather_update_minutes,
        WEATHER_UPDATE_MINUTES_MIN,
        WEATHER_UPDATE_MINUTES_MAX);
    update -= (update % WEATHER_UPDATE_LCM);
    settings->weather_update_minutes = update;
  }
  if (!BATTERY_ORIENTATION_VALID(settings->battery_orientation)) {
    settings->battery_orientation = BATTERY_ORIENTATION_DEFAULT;
  }
}

static void read_legacy_key(
    const uint8_t to_read,
    WatchfaceSettings* settings) {
  struct LegacySettings {
    uint8_t temp_unit;
    uint8_t time_format;
    SupportedDisplayModes display_mode;
    uint8_t weather_update_minutes;
#ifdef PBL_HEALTH
    uint8_t hr_sample_minutes;
    uint16_t steps_goal;
#endif
    uint8_t battery_orientation;
  };
  // This is a legacy settings shape; get it from storage
  // then never refer to it again, i.e. delete it.
  // Persist the new shape to storage.
  struct LegacySettings legacy = {0};
  int bytes_read = persist_read_data(c_key_prior_settings, &legacy, to_read);
  if (bytes_read != to_read) {
    APP_LOG(
        APP_LOG_LEVEL_WARNING,
        "Persisted settings read failed: result=%d expected=%d",
        bytes_read,
        to_read);
    persist_delete(c_key_prior_settings);
  } else {
    *settings = (WatchfaceSettings){
        .temp_unit = legacy.temp_unit,
        .display_mode = legacy.display_mode,
        .battery_orientation = legacy.battery_orientation,
        .weather_update_minutes = legacy.weather_update_minutes,
#ifdef PBL_HEALTH
        .hr_sample_minutes = legacy.hr_sample_minutes,
        .steps_goal = legacy.steps_goal,
#endif
    };
    if (settings_save(settings)) persist_delete(c_key_prior_settings);
  }
}

static void settings_read_stored(
    WatchfaceSettings* settings) {
  if (!settings) {
    return;
  }

  if (!persist_exists(c_key_current_settings) && !persist_exists(c_key_prior_settings)) {
    return;
  }

  uint16_t to_read = 0;
  if (persist_exists(c_key_current_settings)) {
    to_read = persist_get_size(c_key_current_settings);
  }
  if (to_read <= 0 && persist_exists(c_key_prior_settings)) {
    to_read = persist_get_size(c_key_prior_settings);
  }

  uint16_t bytes_read = 0;
  uint16_t expected_size = sizeof(WatchfaceSettings);
  if (to_read > expected_size && persist_exists(c_key_prior_settings)) {
    read_legacy_key(to_read, settings);
  } else if (to_read == expected_size && persist_exists(c_key_current_settings)) {
    bytes_read = persist_read_data(c_key_current_settings, settings, to_read);
    if (bytes_read != to_read) {
      APP_LOG(
          APP_LOG_LEVEL_WARNING,
          "Persisted settings read failed: result=%d expected=%d",
          bytes_read,
          to_read);
      memset(settings, 0, sizeof(WatchfaceSettings));
      persist_delete(c_key_current_settings);
    } else if (persist_exists(c_key_prior_settings)) {
      persist_delete(c_key_prior_settings);
    }
  } else {
    // Not equal to or greater than the expected size. delete truncated keys if they exist
    if (persist_exists(c_key_prior_settings)) persist_delete(c_key_prior_settings);
    if (persist_exists(c_key_current_settings)) persist_delete(c_key_current_settings);
  }
}

// This "API" block is called by other modules or main
bool settings_set_display_mode(
    WatchfaceSettings* settings,
    int display_mode) {
  if (!settings || !DISPLAY_MODE_VALID(display_mode)) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "Settings rejected DISPLAY_MODE: value=%d", display_mode);
    return false;
  }

  settings->display_mode = (SupportedDisplayModes)display_mode;
  return true;
}

bool settings_set_temp_unit(
    WatchfaceSettings* settings,
    int temp_unit) {
  if (!settings || !TEMP_UNIT_VALID(temp_unit)) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "Settings rejected TEMP_UNIT: value=%d", temp_unit);
    return false;
  }

  settings->temp_unit = (uint8_t)temp_unit;
  return true;
}

bool settings_set_weather_update_minutes(
    WatchfaceSettings* settings,
    int minutes) {
  if (!settings) {
    return false;
  }
  if (!WEATHER_UPDATE_MINUTES_VALID(minutes)) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "Settings rejected WEATHER_UPDATE_MINUTES: value=%d", minutes);
    return false;
  }

  settings->weather_update_minutes = (uint8_t)minutes;
  return true;
}

bool settings_set_battery_orientation(
    WatchfaceSettings* settings,
    int orientation) {
  if (!settings || !BATTERY_ORIENTATION_VALID(orientation)) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "Settings rejected BATTERY_ORIENTATION: value=%d", orientation);
    return false;
  }

  settings->battery_orientation = (uint8_t)orientation;
  return true;
}

#ifdef PBL_HEALTH
bool settings_set_hr_sample_minutes(
    WatchfaceSettings* settings,
    int minutes) {
  if (!settings || !HR_SAMPLE_MINUTES_VALID(minutes)) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "Settings rejected HR_SAMPLE_MINUTES: value=%d", minutes);
    return false;
  }

  settings->hr_sample_minutes = (uint8_t)minutes;
  return true;
}

bool settings_set_steps_goal(
    WatchfaceSettings* settings,
    int goal) {
  if (!settings || !STEPS_GOAL_VALID(goal)) {
    return false;
  }

  settings->steps_goal = (uint16_t)goal;
  return true;
}
#endif

void settings_load(
    WatchfaceSettings* settings) {
  if (!settings) {
    return;
  }

  settings_apply_defaults(settings);
  settings_read_stored(settings);
  settings_sanitize(settings);
}

// It is critical that you are deliberately frugal with writing to persisted storage
// This is on the watchface and Pebble has specific documentation about persistent writes.
// Pebble provides no guarantee that identical data is internally deduplicated.
bool settings_save(
    const WatchfaceSettings* settings) {
  if (!settings) {
    return false;
  }
  uint16_t expected_size = sizeof(WatchfaceSettings);
  uint16_t bytes_written = persist_write_data(c_key_current_settings, settings, expected_size);
  if (bytes_written != (int)sizeof(*settings)) {
    APP_LOG(
        APP_LOG_LEVEL_WARNING,
        "Persisted settings write failed: result=%d expected=%d",
        bytes_written,
        expected_size);
    if (persist_exists(c_key_current_settings)) persist_delete(c_key_current_settings);
    return false;
  }

  return true;
}
// End "API" block
