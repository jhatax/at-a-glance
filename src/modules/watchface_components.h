#pragma once

#include <pebble.h>

#define WATCHFACE_UNAVAILABLE_TEXT "---"

typedef enum {
  // Reference display used for all layout decisions and icon styling.
  DESIGN_FACE_HEIGHT = 228,
  DESIGN_FACE_WIDTH = 200,

  // Reference size for all designed glyphs
  DESIGN_ICON_HEIGHT = 28,
  DESIGN_ICON_WIDTH = 28,
} WatchfaceDesignInputs;

typedef enum {
  WATCHFACE_FONT_ROLE_DATE = 0,
  WATCHFACE_FONT_ROLE_TIME,
#ifdef PBL_HEALTH
  WATCHFACE_FONT_ROLE_BPM,
  WATCHFACE_FONT_ROLE_STEPS,
#endif
  WATCHFACE_FONT_ROLE_BATTERY,
  WATCHFACE_FONT_ROLE_CLIMATE,
  WATCHFACE_FONT_ROLE_COUNT
} WatchfaceFontRole;

typedef enum {
  WATCHFACE_COLOR_ROLE_PRIMARY_TEXT = 0,
  WATCHFACE_COLOR_ROLE_UNAVAILABLE_TEXT,
  WATCHFACE_COLOR_ROLE_DATE,
  WATCHFACE_COLOR_ROLE_TIME
} WatchfaceColorRole;

typedef struct {
  GColor background;
  GColor background_layer_background;
  GColor background_layer_rule;
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
  GRect frame;
  bool rule_enabled;
  GRect rule;
} WatchfaceBackgroundStratum;

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
  bool is_light_mode;
  bool is_compact;
  GFont fonts[WATCHFACE_FONT_ROLE_COUNT];
  uint32_t custom_font_resource_ids[WATCHFACE_FONT_ROLE_COUNT];
} WatchfaceSurfaceStyle;

typedef struct {
  int16_t face_width;
  int16_t face_height;
  WatchfaceBackgroundStratum background;
  WatchfaceSurfaceStyle style;
  WatchfaceTextStratum date;
  WatchfaceTextStratum time;
#ifdef PBL_HEALTH
  WatchfaceTextWithIconStratum bpm;
  WatchfaceTextWithIconStratum steps;
#endif
  WatchfaceTextWithIconStratum battery;
  WatchfaceTextWithIconStratum climate;
} WatchfaceSurface;
