#pragma once

#include <pebble.h>

#include "watchface_components.h"

bool layout_watchface_initialize(
    int16_t face_width,
    int16_t face_height,
    WatchfaceSurface* surface);

void layout_update_watchface_style(
    WatchfaceSurfaceStyle* style,
    uint8_t display_mode);
