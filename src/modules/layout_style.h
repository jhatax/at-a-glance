#pragma once
#include <pebble.h>

typedef enum {
  WATCHFACE_FONT_ROLE_TIME = 0,
  WATCHFACE_FONT_ROLE_DATE,
  WATCHFACE_FONT_ROLE_LOCATION,
  WATCHFACE_FONT_ROLE_TEXT,
  WATCHFACE_FONT_ROLE_COUNT
} WatchfaceFontRole;

typedef enum {
  WATCHFACE_COLOR_ROLE_PRIMARY_TEXT = 0,
  WATCHFACE_COLOR_ROLE_OUTOFRANGE_TEXT,
  WATCHFACE_COLOR_ROLE_DATE,
  WATCHFACE_COLOR_ROLE_TIME
} WatchfaceColorRole;

typedef struct {
  bool is_light_mode;
  GColor background;
  GColor primary_text;
  GColor outofrange_text;
  GColor date_text;
  GColor time_text;
} ColorPalette;

typedef struct {
  GFont chosen_fonts[WATCHFACE_FONT_ROLE_COUNT];
  uint32_t custom_font_resource_ids[WATCHFACE_FONT_ROLE_COUNT];
  GFont custom_fonts[WATCHFACE_FONT_ROLE_COUNT];
  uint8_t custom_fonts_loaded_count;
} FontBook;
