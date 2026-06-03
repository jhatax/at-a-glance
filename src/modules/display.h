#pragma once

#include <pebble.h>

typedef struct {
  GColor background;
  GColor primary_text;
  GColor available_text_background;
  GColor unavailable_text;
  GColor unavailable_text_background;
  GColor date;
  GColor time;
  GColor rule;
  GColor steps_icon;
} VisualPalette;

const VisualPalette* display_get_palette(uint8_t display_mode);
const char* display_unavailable_text(void);
void display_update_text_layer(
    TextLayer* layer,
    const char* text,
    GColor text_color,
    GColor background_color);
