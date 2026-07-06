#pragma once

#include <pebble.h>

#include "layout_style.h"
#include "layout_surface.h"

#define WATCHFACE_OUTOFRANGE_TEXT "---"
#define WATCHFACE_UNINITIALIZED_TEXT_COLOR GColorWhite

#if defined(PBL_PLATFORM_EMERY) || defined(PBL_PLATFORM_GABBRO)
#define WATCHFACE_HAS_LARGE_DISPLAY 1
#else
#define WATCHFACE_HAS_LARGE_DISPLAY 0
#endif

typedef struct {
  GRect frame;
} WatchfaceIconSubstratum;

typedef struct {
  GRect frame;
  GTextAlignment alignment;
  WatchfaceFontRole font_role;
  WatchfaceColorRole color_role;
} WatchfaceTextSubstratum;

typedef struct {
  WatchfaceTextSubstratum text;
} WatchfaceTextStratum;

typedef struct {
  WatchfaceIconSubstratum icon;
  WatchfaceTextSubstratum text;
} WatchfaceTextWithIconStratum;

typedef struct {
  WatchfaceIconSubstratum icon;
  WatchfaceTextSubstratum text;
  GRect progress;
} WatchfaceTextWithIconAndProgressStratum;

typedef struct {
  const ColorPalette* palette;
  bool is_compact;
  FontBook fontbook;
} WatchfaceSurfaceStyle;

typedef struct {
  GRect track;
  GRect fill;
  GRect bolt;
} WatchfaceBatteryStratum;

typedef struct {
  int16_t face_width;
  int16_t face_height;
  WatchfaceSurfaceStyle style;
  WatchfaceTextStratum date;
  WatchfaceTextStratum time;
#ifdef PBL_HEALTH
  WatchfaceTextWithIconStratum bpm;
  WatchfaceTextWithIconAndProgressStratum steps;
#endif
  WatchfaceBatteryStratum battery;
  WatchfaceTextWithIconStratum climate;
} WatchfaceSurface;
