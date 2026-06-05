#pragma once

#include <pebble.h>
#define MAX_STR_LEN 16

typedef enum {
  BUF_DATE = 0,
  BUF_TIME,
  BUF_TEMP,
  BUF_TOTAL_COUNT,
  BUF_CLEANUP = BUF_TOTAL_COUNT
} TextBufferId;
