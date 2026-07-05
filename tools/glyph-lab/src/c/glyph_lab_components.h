#pragma once

#include <pebble.h>

typedef enum {
  DESIGN_ICON_HEIGHT = 28,
  DESIGN_ICON_WIDTH = 28,
} GlyphDesignInputs;

typedef struct {
  GColor background;
  GColor background_layer_background;
  GColor background_layer_rule;
  GColor primary_text;
  GColor outofrange_text;
  GColor date;
  GColor time;
} ColorPalette;
