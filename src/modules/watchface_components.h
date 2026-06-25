#pragma once

#include "layout_style.h"
#include "layout_surface.h"
#include <pebble.h>

#define WATCHFACE_UNAVAILABLE_TEXT "---"
#define WATCHFACE_UNINITIALIZED_TEXT_COLOR PBL_IF_COLOR_ELSE(GColorLightGray, GColorWhite)

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
  const ColorPalette *palette;
  bool is_compact;
  // These two MUST ALWAYS BE THE SAME SIZE
  FontBook fontbook;
} WatchfaceSurfaceStyle;

typedef struct {
  int16_t face_width;
  int16_t face_height;
  WatchfaceSurfaceStyle style;
  WatchfaceTextStratum date;
  WatchfaceTextStratum time;
#ifdef PBL_HEALTH
  WatchfaceTextWithIconStratum bpm;
  WatchfaceTextWithIconStratum steps;
#endif
  WatchfaceBatteryStratum battery;
  WatchfaceTextWithIconStratum climate;
} WatchfaceSurface;
