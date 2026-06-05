#pragma once

#include <pebble.h>

#define DISPLAY_UNAVAILABLE_TEXT "---"

typedef struct {
  GColor background;
  GColor primary_text;
  GColor unavailable_text;
  GColor date;
  GColor time;
  GColor rule;
  GColor steps_icon;
} VisualPalette;

GColor display_legible_over_background(const VisualPalette* palette);
const VisualPalette* display_get_palette(uint8_t display_mode);
void display_update_text_layer(
    TextLayer* layer,
    const char* text,
    GColor text_color);
