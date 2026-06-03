#pragma once

#include <pebble.h>

#define PERSIST_SETTINGS 2

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
  ICON_FALLBACK_MODE_DISABLED = 0,
  ICON_FALLBACK_MODE_ENABLED,
  ICON_FALLBACK_MODE_COUNT,
  ICON_FALLBACK_MODE_DEFAULT = ICON_FALLBACK_MODE_DISABLED,
} IconFallbackMode;

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
#define ICON_FALLBACK_MODE_VALID(value) \
  ((value) >= 0 && (value) < ICON_FALLBACK_MODE_COUNT)
#define DISPLAY_MODE_VALID(value) \
  ((value) >= 0 && (value) < DISPLAY_MODE_COUNT)
#define HR_SAMPLE_MINUTES_VALID(value) \
  ((value) >= 0 && (value) < HR_SAMPLE_MINUTES_COUNT)

typedef struct {
  uint8_t temp_unit;
  uint8_t time_format;
  uint8_t hr_sample_minutes;
  uint8_t icon_fallback_mode;
  uint8_t display_mode;
  int16_t temp_celsius_tenths;
} WatchfaceSettings;

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

typedef struct {
  GColor background;
  GColor primary_text;
  GColor available_text_background;
  GColor unavailable_text;
  GColor unavailable_text_background;
  GColor date;
  GColor time;
  GColor rule;
  GColor steps_icon;
} VisualPalette;

typedef struct {
  GRect background_frame;
  GRect date_frame;
  GRect time_frame;
  GRect bpm_icon_frame;
  GRect bpm_text_frame;
  GRect steps_icon_frame;
  GRect steps_text_frame;
  GRect weather_icon_frame;
  GRect temp_text_frame;
  GRect battery_icon_frame;
  GRect battery_text_frame;
  int16_t rule_left;
  int16_t rule_y;
  int16_t rule_right;
  int16_t content_x;
  int16_t row_gap;
  int16_t column_gap;
  int16_t content_width_date;
  int16_t content_width_time;
  int16_t content_width_health;
  int16_t content_width_environment;
} WatchfaceLayout;
