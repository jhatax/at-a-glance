#pragma once

#include <pebble.h>

#define WATCHFACE_UNAVAILABLE_TEXT "---"
#define WATCHFACE_UNINITIALIZED_TEXT_COLOR PBL_IF_COLOR_ELSE(GColorLightGray, GColorWhite)

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

enum {
  WATCHFACE_ICON_GRID_WIDTH = 28,
  WATCHFACE_ICON_GRID_HEIGHT = 28,
};

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
