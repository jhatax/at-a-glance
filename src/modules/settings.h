#pragma once

#include <pebble.h>

typedef enum {
  TEMP_UNIT_F = 0,
  TEMP_UNIT_C,
  TEMP_UNIT_COUNT,
  TEMP_UNIT_DEFAULT = TEMP_UNIT_F,
} TempUnit;

typedef enum {
  TIME_FMT_24 = 0,
  TIME_FMT_12,
  TIME_FMT_COUNT,
  TIME_FMT_DEFAULT = TIME_FMT_24,
} TimeFormat;

typedef enum {
  DISPLAY_MODE_DARK = 0,
  DISPLAY_MODE_LIGHT,
  DISPLAY_MODE_COUNT,
  DISPLAY_MODE_DEFAULT = DISPLAY_MODE_DARK,
} DisplayMode;

typedef enum {
  HR_SAMPLE_MINUTES_10 = 0,
  HR_SAMPLE_MINUTES_15,
  HR_SAMPLE_MINUTES_30,
  HR_SAMPLE_MINUTES_60,
  HR_SAMPLE_MINUTES_120,
  HR_SAMPLE_MINUTES_COUNT,
  HR_SAMPLE_MINUTES_DEFAULT = HR_SAMPLE_MINUTES_10,
} HrSampleMinutes;

#define TEMP_UNIT_VALID(value) \
  ((value) >= 0 && (value) < TEMP_UNIT_COUNT)
#define TIME_FORMAT_VALID(value) \
  ((value) >= 0 && (value) < TIME_FMT_COUNT)
#define DISPLAY_MODE_VALID(value) \
  ((value) >= 0 && (value) < DISPLAY_MODE_COUNT)
#define HR_SAMPLE_MINUTES_VALID(value) \
  ((value) >= 0 && (value) < HR_SAMPLE_MINUTES_COUNT)

typedef struct {
  // Add persisted fields at the bottom
  uint8_t temp_unit;
  uint8_t time_format;
  uint8_t hr_sample_minutes;
  uint8_t display_mode;
} WatchfaceSettings;

void settings_load(WatchfaceSettings* settings);
void settings_save(const WatchfaceSettings* settings);
uint8_t settings_get_hr_sample_minutes(uint8_t hr_sample_minutes);
void settings_apply_defaults(WatchfaceSettings* settings);
