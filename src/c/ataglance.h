#pragma once

#include <pebble.h>

typedef enum {
  BUF_DATE = 0,
  BUF_TIME,
  BUF_TEMP,
  BUF_TOTAL_COUNT,
  BUF_CLEANUP = BUF_TOTAL_COUNT
} TextBufferId;

#define MAX_STR_LEN 16
