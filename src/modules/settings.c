#include "settings.h"

static void settings_apply_defaults(WatchfaceSettings* settings) {
  if (!settings) {
    return;
  }

  settings->temp_unit = TEMP_UNIT_DEFAULT;
  settings->time_format = TIME_FMT_DEFAULT;
  settings->hr_sample_minutes = HR_SAMPLE_MINUTES_DEFAULT;
  settings->reserved_icon_mode = 0;
  settings->display_mode = DISPLAY_MODE_DEFAULT;
  settings->temp_celsius_tenths = SETTINGS_TEMP_INVALID;
}

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

  if (persist_exists(PERSIST_SETTINGS)) {
    persist_read_data(
        PERSIST_SETTINGS,
        settings,
        sizeof(*settings));
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

void settings_save(const WatchfaceSettings* settings) {
  if (!settings) {
    return;
  }

  persist_write_data(PERSIST_SETTINGS, settings, sizeof(*settings));
}
