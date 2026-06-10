#pragma once

#include <pebble.h>

#include "watchface_components.h"

void layout_stylist_update_surface_style(
    int16_t face_width,
    int16_t face_height,
    uint8_t display_mode,
    WatchfaceSurfaceStyle* style);
