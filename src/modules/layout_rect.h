#pragma once

#include <pebble.h>
#include "layout.h"

void layout_rect_calculate_surface(
    int16_t face_width,
    int16_t face_height,
    WatchfaceSurface* surface);
