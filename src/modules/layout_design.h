#pragma once
#include <pebble.h>
#include "watchface_components.h"

// Fonts
#define DESIGN_FONT_TIME_COMPACT FONT_KEY_BITHAM_34_MEDIUM_NUMBERS
#define DESIGN_FONT_COMPACT_PRIMARY_TEXT FONT_KEY_GOTHIC_14

#define DESIGN_FONT_TIME_FULL FONT_KEY_ROBOTO_BOLD_SUBSET_49
#define DESIGN_FONT_PRIMARY_TEXT FONT_KEY_GOTHIC_18

// Blueprints
typedef enum {
  #if defined(PBL_RECT)
  DESIGN_MARGIN = 4,
#elif defined(PBL_ROUND)
  DESIGN_MARGIN = 20,
#endif
  DESIGN_YTOP_OFFSET = DESIGN_MARGIN,
  DESIGN_YBOTTOM_OFFSET = DESIGN_MARGIN,
  DESIGN_ICON_TEXT_GAP = 1,
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
  DESIGN_FULL_ICON_WIDTH = 24,
  DESIGN_FULL_ICON_HEIGHT = 24,
  DESIGN_FULL_DATE_TEXT_HEIGHT = 20,
  DESIGN_FULL_TIME_TEXT_HEIGHT = 50,
  DESIGN_FULL_DATA_TEXT_HEIGHT = 20,
  DESIGN_FULL_CLIMATE_TEXT_WIDTH = 46,
#ifdef PBL_HEALTH
  DESIGN_FULL_STEPS_TEXT_WIDTH = 46,
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
  DESIGN_COMPACT_CLIMATE_TEXT_WIDTH = 39,
#ifdef PBL_HEALTH
  DESIGN_COMPACT_STEPS_TEXT_WIDTH = 39,
  DESIGN_COMPACT_BPM_TEXT_WIDTH = 39,
#endif
} DesignCompact;

// Blueprint support structures
typedef struct {
  int16_t margin;
  int16_t icon_w;
  int16_t icon_h;
  int16_t time_y_percent;
  int16_t time_text_height;
  int16_t date_text_height;
  int16_t data_text_height;
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
