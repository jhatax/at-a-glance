#pragma once
#ifdef PBL_RECT
#include <pebble.h>
#include "layout.h"
#include "helper.h"

typedef enum {
  DESIGN_RECT_MARGIN = 7,
  DESIGN_RECT_YTOP_OFFSET = DESIGN_RECT_MARGIN,
  DESIGN_RECT_YBOTTOM_OFFSET = DESIGN_RECT_MARGIN,
  DESIGN_RECT_ICON_TEXT_GAP = 2,
  DESIGN_RECT_TIME_Y_PERCENT = 30,
  DESIGN_RECT_BATTERY_BAND_WIDTH_PERCENT = 50,
  DESIGN_RECT_DATE_WIDTH_PERCENT = 80,
  DESIGN_RECT_BATTERY_BAND_HEIGHT = 18,
  DESIGN_RECT_BATTERY_TRACK_HEIGHT = 8,
  DESIGN_RECT_BATTERY_FILL_HEIGHT = 6,
  DESIGN_RECT_BATTERY_BOLT_WIDTH = 16,
  DESIGN_RECT_BATTERY_BOLT_HEIGHT = 16,
} DesignCommonRect;

typedef enum {
  DESIGN_RECT_REFERENCE_DATE_TEXT_HEIGHT = 28,
  DESIGN_RECT_REFERENCE_TIME_TEXT_HEIGHT = 54,
  DESIGN_RECT_REFERENCE_DATA_TEXT_HEIGHT = 20,
  DESIGN_RECT_REFERENCE_RIGHT_TEXT_X = 153,
  DESIGN_RECT_REFERENCE_CLIMATE_TEXT_WIDTH = 40,
#ifdef PBL_HEALTH
  DESIGN_RECT_REFERENCE_STEPS_TEXT_WIDTH = 40,
  DESIGN_RECT_REFERENCE_BPM_TEXT_WIDTH = 40,
#endif
} DesignReferenceRect;

typedef enum {
  DESIGN_RECT_COMPACT_ICON_WIDTH = 20,
  DESIGN_RECT_COMPACT_ICON_HEIGHT = 20,
  DESIGN_RECT_COMPACT_DATE_TEXT_HEIGHT = 20,
  DESIGN_RECT_COMPACT_TIME_TEXT_HEIGHT = 42,
  DESIGN_RECT_COMPACT_DATA_TEXT_HEIGHT = 16,
  DESIGN_RECT_COMPACT_RIGHT_TEXT_X = 106,
  DESIGN_RECT_COMPACT_CLIMATE_TEXT_WIDTH = 40,
#ifdef PBL_HEALTH
  DESIGN_RECT_COMPACT_STEPS_TEXT_WIDTH = 40,
  DESIGN_RECT_COMPACT_BPM_TEXT_WIDTH = 33,
#endif
} DesignCompactRect;

typedef struct {
  int16_t icon_w;
  int16_t icon_h;
  int16_t date_text_height;
  int16_t time_text_height;
  int16_t data_text_height;
  int16_t icon_text_pair_height;
  int16_t right_text_x;
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

static const LayoutBlueprint c_rect_reference_blueprint = {
  .icon_w = DESIGN_ICON_WIDTH,
  .icon_h = DESIGN_ICON_HEIGHT,
  .date_text_height = DESIGN_RECT_REFERENCE_DATE_TEXT_HEIGHT,
  .time_text_height = DESIGN_RECT_REFERENCE_TIME_TEXT_HEIGHT,
  .data_text_height = DESIGN_RECT_REFERENCE_DATA_TEXT_HEIGHT,
  .icon_text_pair_height = HELPER_MAX((int)DESIGN_ICON_HEIGHT, (int)DESIGN_RECT_REFERENCE_DATA_TEXT_HEIGHT),
  .right_text_x = DESIGN_RECT_REFERENCE_RIGHT_TEXT_X,
  .climate_text_width = DESIGN_RECT_REFERENCE_CLIMATE_TEXT_WIDTH,
#ifdef PBL_HEALTH
  .steps_text_width = DESIGN_RECT_REFERENCE_STEPS_TEXT_WIDTH,
  .bpm_text_width = DESIGN_RECT_REFERENCE_BPM_TEXT_WIDTH,
#endif
};

static const LayoutBlueprint c_rect_compact_blueprint = {
  .icon_w = DESIGN_RECT_COMPACT_ICON_WIDTH,
  .icon_h = DESIGN_RECT_COMPACT_ICON_HEIGHT,
  .date_text_height = DESIGN_RECT_COMPACT_DATE_TEXT_HEIGHT,
  .time_text_height = DESIGN_RECT_COMPACT_TIME_TEXT_HEIGHT,
  .data_text_height = DESIGN_RECT_COMPACT_DATA_TEXT_HEIGHT,
  .icon_text_pair_height = HELPER_MAX(DESIGN_RECT_COMPACT_ICON_HEIGHT, DESIGN_RECT_COMPACT_DATA_TEXT_HEIGHT),
  .right_text_x = DESIGN_RECT_COMPACT_RIGHT_TEXT_X,
  .climate_text_width = DESIGN_RECT_COMPACT_CLIMATE_TEXT_WIDTH,
#ifdef PBL_HEALTH
  .steps_text_width = DESIGN_RECT_COMPACT_STEPS_TEXT_WIDTH,
  .bpm_text_width = DESIGN_RECT_COMPACT_BPM_TEXT_WIDTH,
#endif
};
#endif
