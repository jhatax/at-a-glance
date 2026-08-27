#pragma once

#include <pebble.h>

#include "layout_style.h"

#define WATCHFACE_OUTOFRANGE_TEXT "---"
#define WATCHFACE_UNINITIALIZED_TEXT_COLOR GColorWhite

typedef GRect WatchfaceFrame;

typedef struct {
  WatchfaceFrame frame;
  GTextAlignment alignment;
  WatchfaceFontRole font_role;
  WatchfaceColorRole color_role;
} WatchfaceTextSubstratum;

typedef WatchfaceTextSubstratum WatchfaceTextStratum;

typedef struct {
  WatchfaceFrame icon;
  WatchfaceTextSubstratum text;
} WatchfaceTextWithIconStratum;

typedef struct {
  WatchfaceFrame icon;
  WatchfaceTextSubstratum text;
  WatchfaceFrame progress;
} WatchfaceTextWithIconAndProgressStratum;

typedef struct {
  const ColorPalette* palette;
  FontBook fontbook;
} WatchfaceSurfaceStyle;

typedef struct {
  WatchfaceFrame track;
  WatchfaceFrame fill;
  WatchfaceFrame bolt;
  bool is_vertical;
} WatchfaceBatteryStratum;

typedef struct {
  int16_t face_width;
  int16_t face_height;
  WatchfaceSurfaceStyle style;
  WatchfaceTextStratum time;
  WatchfaceFrame horiz_rule;
  WatchfaceTextStratum date;
#ifdef PBL_HEALTH
  WatchfaceTextWithIconStratum bpm;
  WatchfaceTextWithIconAndProgressStratum steps;
#endif
  WatchfaceBatteryStratum battery;
  WatchfaceTextWithIconStratum climate;
  WatchfaceTextStratum location;
  WatchfaceFrame bt_icon;
} WatchfaceSurface;
