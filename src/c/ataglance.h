#pragma once

#include <pebble.h>

#define RAIL_X 14
#define RAIL_TOP 12
#define RAIL_BOTTOM 216

#define CONTENT_X 12

#define RULE_RIGHT 188
#define RULE_VERT 140

#define PERSIST_SETTINGS 2

#define TEMP_UNIT_F 0
#define TEMP_UNIT_C 1
#define TIME_FMT_24 0
#define TIME_FMT_12 1

#define TEMP_INVALID INT16_MIN

typedef enum {
  HR_SAMPLE_MINUTES_10 = 10,
  HR_SAMPLE_MINUTES_15 = 15,
  HR_SAMPLE_MINUTES_30 = 30,
  HR_SAMPLE_MINUTES_60 = 60,
  HR_SAMPLE_MINUTES_120 = 120,
} HrSampleMinutes;

#define HR_SAMPLE_MINUTES_DEFAULT HR_SAMPLE_MINUTES_10

typedef enum {
  BUF_DATE = 0,
  BUF_TIME,
  BUF_BPM,
  BUF_STEPS,
  BUF_BATTERY,
  BUF_TEMP,
  BUF_CLEANUP
} TextBufferId;

#define TOTAL_BUFFERS ((size_t)BUF_CLEANUP)
#define MAX_STR_LEN 16
