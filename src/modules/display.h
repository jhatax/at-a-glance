#pragma once

#include <pebble.h>

typedef struct {
  GColor background;
  GColor primary_text;
  GColor unavailable_text;
  GColor date;
  GColor time;
  GColor rule;
  GColor steps_icon;
} VisualPalette;

static inline GColor display_legible_over_background(
    const VisualPalette* palette) {
  if (!palette) {
    return GColorWhite;
  }

  return gcolor_legible_over(palette->background);
}

const VisualPalette* display_get_palette(uint8_t display_mode);
const char* display_unavailable_text(void);
void display_update_text_layer(
    TextLayer* layer,
    const char* text,
    GColor text_color);
