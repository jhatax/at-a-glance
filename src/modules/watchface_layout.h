#pragma once

#include <pebble.h>

#include "watchface_components.h"

#if (defined(PBL_PLATFORM_EMERY) || defined(PBL_PLATFORM_GABBRO))
#define IS_LARGE_DISPLAY 1
#else
#define IS_LARGE_DISPLAY 0
#endif

// Architect
bool layout_watchface_prepare(
    int16_t face_width,
    int16_t face_height,
    WatchfaceSurface* surface);

// Stylist: Palette
void layout_watchface_update_palette(
    WatchfaceSurfaceStyle* style,
    SupportedDisplayModes display_mode);

// Stylist: Fonts
bool layout_watchface_initialize_fonts(FontBook* fontbook);

bool layout_watchface_load_custom_fonts(FontBook* fontbook);

void layout_watchface_unload_custom_fonts(FontBook* fontbook);
