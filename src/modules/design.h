#pragma once
#include <pebble.h>
#include "layout.h"
#include "helper.h"

typedef enum {
  DESIGN_ICON_TEXT_GAP = 2,
  DESIGN_TIME_Y_PERCENT = 27,
  DESIGN_BATTERY_BAND_WIDTH_PERCENT = 70,
  DESIGN_BATTERY_BAND_HEIGHT = 16,
  DESIGN_BATTERY_TRACK_HEIGHT = 6,
  DESIGN_BATTERY_FILL_HEIGHT = 4,
  DESIGN_BATTERY_BOLT_WIDTH = 16,
  DESIGN_BATTERY_BOLT_HEIGHT = 16,
} DesignCommon;

typedef enum {
  DESIGN_FULL_FACE_WIDTH = 200,
  DESIGN_FULL_FACE_HEIGHT = 228,
  DESIGN_FULL_MARGIN = 6,
  DESIGN_FULL_ICON_WIDTH = 26,
  DESIGN_FULL_ICON_HEIGHT = 26,
  DESIGN_FULL_DATE_TEXT_HEIGHT = 24,
  DESIGN_FULL_TIME_TEXT_HEIGHT = 50,
  DESIGN_FULL_DATA_TEXT_HEIGHT = 24,
  DESIGN_FULL_CLIMATE_TEXT_WIDTH = 48,
#ifdef PBL_HEALTH
  DESIGN_FULL_STEPS_TEXT_WIDTH = 48,
  DESIGN_FULL_BPM_TEXT_WIDTH = 40,
#endif
} DesignFull;

typedef enum {
  DESIGN_COMPACT_MARGIN = 2,
  DESIGN_COMPACT_TIME_Y_PERCENT = 25,
  DESIGN_COMPACT_ICON_WIDTH = 16,
  DESIGN_COMPACT_ICON_HEIGHT = 16,
  DESIGN_COMPACT_DATE_TEXT_HEIGHT = 20,
  DESIGN_COMPACT_TIME_TEXT_HEIGHT = 38,
  DESIGN_COMPACT_DATA_TEXT_HEIGHT = 20,
  DESIGN_COMPACT_RIGHT_TEXT_X = 106,
  DESIGN_COMPACT_CLIMATE_TEXT_WIDTH = 37,
#ifdef PBL_HEALTH
  DESIGN_COMPACT_STEPS_TEXT_WIDTH = 40,
  DESIGN_COMPACT_BPM_TEXT_WIDTH = 33,
#endif
} DesignCompact;

typedef struct {
  int16_t margin;
  int16_t icon_w;
  int16_t icon_h;
  int16_t time_y_percent;
  int16_t time_text_height;
  int16_t date_text_height;
  int16_t data_text_height;
  int16_t icon_text_pair_height;
  int16_t climate_text_width;
#ifdef PBL_HEALTH
  int16_t steps_text_width;
  int16_t bpm_text_width;
#endif
} LayoutBlueprint;

typedef struct {
  GRect icon;
  GRect text;
} CalculatedMetricPair;

typedef struct {
  GRect time;
  GRect date;
  WatchfaceBatteryStratum battery;
  CalculatedMetricPair climate;
#ifdef PBL_HEALTH
  CalculatedMetricPair steps;
  CalculatedMetricPair bpm;
#endif
} CalculatedLayout;
