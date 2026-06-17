#pragma once
#include <pebble.h>
#include "layout.h"
#include "helper.h"

typedef enum {
#if defined(PBL_RECT)
  DESIGN_MARGIN = 6,
#elif defined(PBL_ROUND)
  DESIGN_MARGIN = 15,
#endif
  DESIGN_YTOP_OFFSET = DESIGN_MARGIN,
  DESIGN_YBOTTOM_OFFSET = DESIGN_MARGIN,
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
  DESIGN_REFERENCE_DATE_TEXT_HEIGHT = 26,
  DESIGN_REFERENCE_TIME_TEXT_HEIGHT = 54,
  DESIGN_REFERENCE_DATA_TEXT_HEIGHT = 26,
  DESIGN_REFERENCE_CLIMATE_TEXT_WIDTH = 40,
#ifdef PBL_HEALTH
  DESIGN_REFERENCE_STEPS_TEXT_WIDTH = 40,
  DESIGN_REFERENCE_BPM_TEXT_WIDTH = 40,
#endif
} DesignReference;

typedef enum {
  DESIGN_COMPACT_TIME_Y_PERCENT = 25,
  DESIGN_COMPACT_ICON_WIDTH = 20,
  DESIGN_COMPACT_ICON_HEIGHT = 20,
  DESIGN_COMPACT_DATE_TEXT_HEIGHT = 16,
  DESIGN_COMPACT_TIME_TEXT_HEIGHT = 36,
  DESIGN_COMPACT_DATA_TEXT_HEIGHT = 16,
  DESIGN_COMPACT_RIGHT_TEXT_X = 106,
  DESIGN_COMPACT_CLIMATE_TEXT_WIDTH = 40,
#ifdef PBL_HEALTH
  DESIGN_COMPACT_STEPS_TEXT_WIDTH = 40,
  DESIGN_COMPACT_BPM_TEXT_WIDTH = 33,
#endif
} DesignCompact;

typedef struct {
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
