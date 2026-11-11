#pragma once

#include <pebble.h>

#include "helper_computations.h"

bool helper_replace_color_in_bitmap(
    GBitmap* bmp,
    GColor color1,
    GColor color2);
