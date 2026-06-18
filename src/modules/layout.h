#pragma once

#include <pebble.h>

#include "watchface_components.h"

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
