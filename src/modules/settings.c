#include "settings.h"

// The ID / key that specifies where settings are persisted in storage
static const uint32_t c_key_persisted_settings = 2;
static const int c_settings_data_size = sizeof(WatchfaceSettings);

static void settings_apply_defaults(
  WatchfaceSettings* settings) {
  if (!settings) {
    return;
  }

  *settings = (WatchfaceSettings){
    .temp_unit = TEMP_UNIT_DEFAULT,
    .time_format = TIME_FMT_DEFAULT,
    .display_mode = DISPLAY_MODE_DEFAULT,
    .weather_update_minutes = WEATHER_UPDATE_MINUTES_DEFAULT,
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
  if (!TIME_FORMAT_VALID(settings->time_format)) {
    settings->time_format = TIME_FMT_DEFAULT;
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
    settings->weather_update_minutes = WEATHER_UPDATE_MINUTES_DEFAULT;
  }
}

static void settings_read_stored(
  WatchfaceSettings* settings) {
  if (!settings) {
    return;
  }

  if (!persist_exists(c_key_persisted_settings)) {
    return;
  }

  int stored_size = persist_get_size(c_key_persisted_settings);
  if (stored_size <= 0) {
    return;
  }

  WatchfaceSettings stored = *settings;

  int read_size = stored_size;
  if (read_size > c_settings_data_size) {
    APP_LOG(APP_LOG_LEVEL_WARNING,
      "Persisted settings truncated: size=%d max=%d",
      stored_size,
      c_settings_data_size);
    read_size = c_settings_data_size;
  } else if (read_size < c_settings_data_size) {
    APP_LOG(APP_LOG_LEVEL_WARNING,
      "Persisted settings partial: size=%d expected=%d",
      stored_size,
      c_settings_data_size);
  }

  int bytes_read = persist_read_data(c_key_persisted_settings, &stored, read_size);
  if (bytes_read != read_size) {
    APP_LOG(APP_LOG_LEVEL_WARNING,
      "Persisted settings read failed: result=%d expected=%d",
      bytes_read,
      read_size);
    return;
  }

  *settings = stored;
}

// This "API" block is called by other modules or main
void settings_load(
  WatchfaceSettings* settings) {
  if (!settings) {
    return;
  }

  settings_apply_defaults(settings);
  settings_read_stored(settings);
  settings_sanitize(settings);
}

bool settings_save(
  const WatchfaceSettings* settings) {
  if (!settings) {
    return false;
  }

  int bytes_written = persist_write_data(c_key_persisted_settings, settings, c_settings_data_size);
  if (bytes_written != (int)sizeof(*settings)) {
    APP_LOG(APP_LOG_LEVEL_WARNING,
      "Persisted settings write failed: result=%d expected=%d",
      bytes_written,
      c_settings_data_size);
    return false;
  }

  return true;
}
// End "API" block
