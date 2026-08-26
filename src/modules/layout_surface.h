#pragma once

#include <pebble.h>

typedef struct {
  GRect track;
  GRect fill;
  GRect bolt;
  bool is_vertical;
} LayoutBatteryStratum;

typedef struct {
  GRect icon;
  GRect text;
} LayoutMetricWithIcon;

typedef struct {
  LayoutMetricWithIcon steps;
  GRect progress;
} LayoutMetricPairWithProgress;

typedef struct {
  GRect time;
  GRect horiz_rule;
  GRect date;
  LayoutBatteryStratum battery;
  LayoutMetricWithIcon climate;
  GRect location;
  GRect bt_icon;
#ifdef PBL_HEALTH
  LayoutMetricPairWithProgress steps_layer;
  LayoutMetricWithIcon bpm;
#endif
} CalculatedLayout;
