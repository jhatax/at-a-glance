#pragma once

#include <pebble.h>

#define WATCHFACE_UNAVAILABLE_TEXT "---"

typedef enum {
  WATCHFACE_FONT_ROLE_TIME = 0,
  WATCHFACE_FONT_ROLE_TEXT,
  WATCHFACE_FONT_ROLE_COUNT
} WatchfaceFontRole;

typedef enum {
  WATCHFACE_COLOR_ROLE_PRIMARY_TEXT = 0,
  WATCHFACE_COLOR_ROLE_UNAVAILABLE_TEXT,
  WATCHFACE_COLOR_ROLE_DATE,
  WATCHFACE_COLOR_ROLE_TIME
} WatchfaceColorRole;

typedef struct {
  bool is_light_mode;
  GColor background;
  GColor primary_text;
  GColor unavailable_text;
  GColor date;
  GColor time;
} ColorPalette;

typedef struct {
  GRect frame;
  GTextAlignment alignment;
  WatchfaceFontRole font_role;
  WatchfaceColorRole color_role;
} WatchfaceTextSubstratum;

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
  WatchfaceTextSubstratum text;
} WatchfaceTextStratum;

// If we only ever need an icon, then we will have
// WatchfaceIconStratum.

typedef struct {
  WatchfaceIconSubstratum icon;
  WatchfaceTextSubstratum text;
} WatchfaceTextWithIconStratum;

typedef struct {
  const ColorPalette* palette;
  bool is_compact;
  // These two MUST ALWAYS BE THE SAME SIZE
  GFont system_fonts[WATCHFACE_FONT_ROLE_COUNT];
  uint32_t custom_font_resource_ids[WATCHFACE_FONT_ROLE_COUNT];
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
