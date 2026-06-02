#pragma once

#include <pebble.h>

#define RAIL_X 14
#define RAIL_TOP 12
#define RAIL_BOTTOM 216

#define CONTENT_X 12
#define RULE_RIGHT 188
#define RULE_VERT 110

#define PERSIST_SETTINGS 2

#define TEMP_INVALID INT16_MIN

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
  FALLBACK_MODE_DISABLED = 0,
  FALLBACK_MODE_ENABLED,
  FALLBACK_MODE_COUNT,
  FALLBACK_MODE_DEFAULT = FALLBACK_MODE_DISABLED,
} FallbackMode;

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
#define FALLBACK_MODE_VALID(value) \
  ((value) >= 0 && (value) < FALLBACK_MODE_COUNT)
#define DISPLAY_MODE_VALID(value) \
  ((value) >= 0 && (value) < DISPLAY_MODE_COUNT)
#define HR_SAMPLE_MINUTES_VALID(value) \
  ((value) >= 0 && (value) < HR_SAMPLE_MINUTES_COUNT)

typedef enum {
  BUF_DATE = 0,
  BUF_TIME,
  BUF_BPM,
  BUF_STEPS,
  BUF_BATTERY,
  BUF_TEMP,
  BUF_TOTAL_COUNT,
  BUF_CLEANUP = BUF_TOTAL_COUNT
} TextBufferId;

#define MAX_STR_LEN 16
