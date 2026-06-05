#include "settings.h"

// Settings Related Decisions
// The ID / key that specifies where in storage I can find the
// current storage_version
const uint32_t c_key_storage_version = 3;

// For this product, we have decided to persist settings
// and start with persisted settings across product launches
const int32_t c_value_storage_version = 1;

// The ID / key that specifies where settings are persisted in storage
const uint32_t c_key_persisted_settings = 2;

void settings_apply_defaults(WatchfaceSettings* settings) {
  if (!settings) {
    return;
  }

  settings->temp_unit = TEMP_UNIT_DEFAULT;
  settings->time_format = TIME_FMT_DEFAULT;
  settings->hr_sample_minutes = HR_SAMPLE_MINUTES_DEFAULT;
  settings->display_mode = DISPLAY_MODE_DEFAULT;
  settings->temp_celsius_tenths = WEATHER_TEMP_INVALID;
}

static void settings_sanitize(WatchfaceSettings* settings) {
  if (!settings) {
    return;
  }
  if (!TEMP_UNIT_VALID(settings->temp_unit)) {
    settings->temp_unit = TEMP_UNIT_DEFAULT;
  }
  if (!TIME_FORMAT_VALID(settings->time_format)) {
    settings->time_format = TIME_FMT_DEFAULT;
  }
  if (!HR_SAMPLE_MINUTES_VALID(settings->hr_sample_minutes)) {
    settings->hr_sample_minutes = HR_SAMPLE_MINUTES_DEFAULT;
  }
  if (!DISPLAY_MODE_VALID(settings->display_mode)) {
    settings->display_mode = DISPLAY_MODE_DEFAULT;
  }
}

static void settings_read_stored(WatchfaceSettings* settings) {
  if (!settings) {
    return;
  }

  if (!persist_exists(c_key_persisted_settings) ||
      !persist_exists(c_key_storage_version)) {
    settings_apply_defaults(settings);
    return;
  }

  int stored_size = persist_get_size(c_key_persisted_settings);
  if (stored_size <= 0) {
    settings_apply_defaults(settings);
    return;
  }

  WatchfaceSettings stored = {0};
  stored.temp_celsius_tenths = WEATHER_TEMP_INVALID;

  int read_size = stored_size;
  if (read_size > (int)sizeof(stored)) {
    read_size = (int)sizeof(stored);
  }

  if (persist_read_data(c_key_persisted_settings, &stored, read_size) > 0) {
    *settings = stored;
  }
}

// This "API" block is called by other modules or main
uint8_t settings_get_hr_sample_minutes(uint8_t hr_sample_minutes) {
  switch ((HrSampleMinutes)hr_sample_minutes) {
    case HR_SAMPLE_MINUTES_10:
      return 10;
    case HR_SAMPLE_MINUTES_15:
      return 15;
    case HR_SAMPLE_MINUTES_30:
      return 30;
    case HR_SAMPLE_MINUTES_60:
      return 60;
    case HR_SAMPLE_MINUTES_120:
      return 120;
    default:
      return 10;
  }
}

void settings_load(WatchfaceSettings* settings) {
  if (!settings) {
    return;
  }

  settings_apply_defaults(settings);
  settings_read_stored(settings);
  settings_sanitize(settings);
}

void settings_save(const WatchfaceSettings* settings) {
  if (!settings) {
    return;
  }

  persist_write_data(c_key_persisted_settings, settings, sizeof(*settings));
  persist_write_int(c_key_storage_version, c_value_storage_version);
}
// End "API" block
