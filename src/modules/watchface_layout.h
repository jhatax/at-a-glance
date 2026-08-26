#pragma once

#include <pebble.h>

#include "watchface_components.h"

// Architect
bool layout_watchface_prepare(
    int16_t face_width,
    int16_t face_height,
    bool is_battery_vertical,
    WatchfaceSurface* surface);

void relayout_battery_bolt_bticon(
    int16_t face_width,
    int16_t face_height,
    bool is_battery_vertical,
    WatchfaceSurface* surface);

// Stylist: Palette
void layout_watchface_update_palette(
    WatchfaceSurfaceStyle* style,
    SupportedDisplayModes display_mode);

// Stylist: Fonts
bool layout_watchface_initialize_fonts(FontBook* fontbook);

bool layout_watchface_load_custom_fonts(FontBook* fontbook);

void layout_watchface_unload_custom_fonts(FontBook* fontbook);
