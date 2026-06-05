#pragma once

#include <pebble.h>
#include "ataglance.h"
#include "../modules/battery.h"
#include "../modules/display.h"
#if defined(PBL_HEALTH)
#include "../modules/health.h"
#endif
#include "../modules/layout.h"
#include "../modules/settings.h"
#include "../modules/weather.h"

typedef enum {
  BUF_DATE = 0,
  BUF_TIME,
  BUF_TEMP,
  BUF_TOTAL_COUNT,
  BUF_CLEANUP = BUF_TOTAL_COUNT
} TextBufferId;

typedef struct {
  char buffers[BUF_TOTAL_COUNT][ATAGLANCE_MAX_STR_LEN];
} WatchfaceTextState;

typedef struct {
  GFont date;
  GFont secondary_value;
  GFont battery_value;
  GFont time;
} WatchfaceFontState;

typedef struct {
  Window* window;
  TextLayer* date_layer;
  TextLayer* time_layer;
  TextLayer* temp_layer;
} WatchfaceLayerState;

typedef struct {
  WatchfaceSettings settings;
  WatchfaceLayout layout;
  const VisualPalette* palette;
  int16_t temp_celsius_tenths;
} WatchfaceRuntimeState;
