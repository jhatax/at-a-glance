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
