#pragma once

#include <pebble.h>

enum {
  WATCHFACE_ICON_GRID_WIDTH = 28,
  WATCHFACE_ICON_GRID_HEIGHT = 28,
};

// Blueprints
enum {
  DESIGN_MARGIN = 8,
  DESIGN_ICON_TEXT_GAP = 2,
  DESIGN_TIME_Y_PERCENT = 27,
  DESIGN_BATTERY_BAND_WIDTH_PERCENT = 75,
  DESIGN_BATTERY_BAND_HEIGHT = 18,
  DESIGN_BATTERY_TRACK_HEIGHT = 8,
  DESIGN_BATTERY_FILL_HEIGHT = 6,
  DESIGN_BATTERY_BOLT_WIDTH = 18,
  DESIGN_BATTERY_BOLT_HEIGHT = 18,
  DESIGN_STEPS_PROGRESS_HEIGHT = 4,
};

enum {
  DESIGN_FULL_FACE_WIDTH = 200,
  DESIGN_FULL_FACE_HEIGHT = 228,
  DESIGN_FULL_MARGIN = DESIGN_MARGIN, // Same for both emery & gabbro
  DESIGN_FULL_ICON_WIDTH = WATCHFACE_ICON_GRID_WIDTH,
  DESIGN_FULL_ICON_HEIGHT = WATCHFACE_ICON_GRID_HEIGHT,
  DESIGN_FULL_DATE_TEXT_HEIGHT = 28,
  DESIGN_FULL_TIME_TEXT_HEIGHT = 60,
  DESIGN_FULL_DATA_TEXT_HEIGHT = 28,
  DESIGN_FULL_CLIMATE_TEXT_WIDTH = 65,
#ifdef PBL_HEALTH
  DESIGN_FULL_STEPS_TEXT_WIDTH = 65,
  DESIGN_FULL_BPM_TEXT_WIDTH = 45,
#endif
};

enum {
  DESIGN_COMPACT_MARGIN = DESIGN_MARGIN,
  DESIGN_COMPACT_TIME_Y_PERCENT = 24,
  DESIGN_COMPACT_ICON_WIDTH = 18,
  DESIGN_COMPACT_ICON_HEIGHT = 18,
  DESIGN_COMPACT_DATE_TEXT_HEIGHT = 18,
  DESIGN_COMPACT_TIME_TEXT_HEIGHT = 40,
  DESIGN_COMPACT_DATA_TEXT_HEIGHT = 18,
  DESIGN_COMPACT_CLIMATE_TEXT_WIDTH = 50,
#ifdef PBL_HEALTH
  DESIGN_COMPACT_STEPS_TEXT_WIDTH = 50,
  DESIGN_COMPACT_BPM_TEXT_WIDTH = 40,
#endif
};

typedef struct {
  GRect frame;
  bool is_enabled;
} WatchfaceIconSubstratum;

typedef struct {
  GRect track;
  GRect fill;
  GRect bolt;
} WatchfaceBatteryStratum;

typedef struct {
  GRect icon;
  GRect text;
} WatchfaceMetricWithIcon;

typedef struct {
  WatchfaceMetricWithIcon steps;
  GRect progress;
} MetricPairWithProgress;

typedef struct {
  GRect time;
  GRect date;
  WatchfaceBatteryStratum battery;
  WatchfaceMetricWithIcon climate;
#ifdef PBL_HEALTH
  MetricPairWithProgress steps_layer;
  WatchfaceMetricWithIcon bpm;
#endif
} CalculatedLayout;
