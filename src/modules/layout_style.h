#pragma once
#include <pebble.h>

// Fonts
#define DESIGN_FONT_COMPACT_TIME_TEXT FONT_KEY_BITHAM_34_MEDIUM_NUMBERS
#define DESIGN_FONT_COMPACT_PRIMARY_TEXT FONT_KEY_LECO_20_BOLD_NUMBERS
#define DESIGN_FONT_COMPACT_DATE_TEXT FONT_KEY_LECO_20_BOLD_NUMBERS

#define DESIGN_FONT_TIME_TEXT FONT_KEY_ROBOTO_BOLD_SUBSET_49
#define DESIGN_FONT_PRIMARY_TEXT FONT_KEY_LECO_26_BOLD_NUMBERS_AM_PM
#define DESIGN_FONT_DATE_TEXT FONT_KEY_LECO_26_BOLD_NUMBERS_AM_PM

typedef enum {
  WATCHFACE_FONT_ROLE_TIME = 0,
  WATCHFACE_FONT_ROLE_DATE,
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
  GColor date_text;
  GColor time_text;
} ColorPalette;

typedef struct {
  GFont chosen_fonts[WATCHFACE_FONT_ROLE_COUNT];
  uint32_t custom_font_resource_ids[WATCHFACE_FONT_ROLE_COUNT];
  uint8_t custom_fonts_loaded_count;
  GFont custom_fonts[WATCHFACE_FONT_ROLE_COUNT];
} FontBook;
