#pragma once
#include <pebble.h>

#include "watchface_components.h"

void layout_calculate_surface(
    int16_t face_width,
    int16_t face_height,
    uint8_t display_mode,
    WatchfaceSurface* surface);

void layout_update_surface_style(
    WatchfaceSurfaceStyle* surface,
    uint8_t display_mode);