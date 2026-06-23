#pragma once

#include <pebble.h>

#include "watchface_components.h"

typedef struct {
  WatchfaceTextSubstratum text;
} WatchfaceTextStratum;

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

// Architect
bool layout_watchface_initialize(
    int16_t face_width,
    int16_t face_height,
    WatchfaceSurface* surface);

// Stylist: Palette
void layout_watchface_update_palette(
    WatchfaceSurfaceStyle* style,
    uint8_t display_mode);

// Stylist: Fonts
bool layout_watchface_initialize_fonts(
    WatchfaceSurfaceStyle* style);

bool layout_watchface_load_custom_fonts(
    WatchfaceSurfaceStyle* style,
    GFont* custom_fonts);

void layout_watchface_unload_custom_fonts(
    GFont* custom_fonts);
