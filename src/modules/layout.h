#pragma once

#include <pebble.h>

#define WATCHFACE_UNAVAILABLE_TEXT "---"

typedef enum {
  WATCHFACE_FONT_ROLE_DATE = 0,
  WATCHFACE_FONT_ROLE_TIME,
  WATCHFACE_FONT_ROLE_BPM,
  WATCHFACE_FONT_ROLE_STEPS,
  WATCHFACE_FONT_ROLE_BATTERY,
  WATCHFACE_FONT_ROLE_CLIMATE,
  WATCHFACE_FONT_ROLE_COUNT
} WatchfaceFontRole;

typedef enum {
  WATCHFACE_COLOR_ROLE_PRIMARY_TEXT = 0,
  WATCHFACE_COLOR_ROLE_UNAVAILABLE_TEXT,
  WATCHFACE_COLOR_ROLE_DATE,
  WATCHFACE_COLOR_ROLE_TIME,
  WATCHFACE_COLOR_ROLE_STEPS_ICON,
  WATCHFACE_COLOR_ROLE_DYNAMIC
} WatchfaceColorRole;

typedef struct {
  GColor background;
  GColor background_layer_background;
  GColor background_layer_line;
  GColor primary_text;
  GColor unavailable_text;
  GColor date;
  GColor time;
  GColor steps_icon;
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
  bool requires_update_proc;
  WatchfaceColorRole color_role;
} WatchfaceIconSubstratum;

typedef struct {
  GRect frame;
  bool line_enabled;
  int16_t line_x;
  int16_t line_y;
  int16_t line_width;
} WatchfaceBackgroundSubstratum;

typedef struct {
  WatchfaceTextSubstratum text;
} WatchfaceTextStratum;

typedef struct {
  WatchfaceIconSubstratum icon;
  WatchfaceTextSubstratum text;
} WatchfaceMetricStratum;

typedef struct {
  const ColorPalette* palette;
  GFont fonts[WATCHFACE_FONT_ROLE_COUNT];
} WatchfaceSurfaceStyle;

typedef struct {
  int16_t face_width;
  int16_t face_height;
  WatchfaceBackgroundSubstratum background;
  int16_t content_x;
  int16_t row_gap;
  int16_t column_gap;
  int16_t content_width_date;
  int16_t content_width_time;
  int16_t content_width_health;
  int16_t content_width_bottom;
  WatchfaceSurfaceStyle style;
  WatchfaceTextStratum date;
  WatchfaceTextStratum time;
  WatchfaceMetricStratum bpm;
  WatchfaceMetricStratum steps;
  WatchfaceMetricStratum battery;
  WatchfaceMetricStratum climate;
} WatchfaceSurface;

void layout_calculate_surface(
    int16_t face_width,
    int16_t face_height,
    uint8_t display_mode,
    WatchfaceSurface* surface);

void layout_update_surface_style(
    WatchfaceSurface* surface,
    uint8_t display_mode);

GColor layout_color_for_role(
    const ColorPalette* palette,
    WatchfaceColorRole role);

void layout_update_text_layer(
    TextLayer* layer,
    const char* text,
    GColor text_color);

int16_t layout_scale_icon_x(const GSize* bounds_size, int16_t coord);

int16_t layout_scale_icon_y(const GSize* bounds_size, int16_t coord);

int16_t layout_scale_icon_coord(
    const GSize* bounds_size,
    int16_t coord);

GPoint layout_scaled_icon_point(
    const GSize* bounds_size,
    int16_t x,
    int16_t y);
