#pragma once

#include <pebble.h>

#define HELPER_MAX(a, b) (((a) > (b)) ? (a) : (b))

#define HELPER_MIN(a, b) (((a) < (b)) ? (a) : (b))

#define HELPER_CLAMP_MIN(exp, min) (((exp) > (min)) ? (exp) : (min))

#define HELPER_SCALE_ROUND(v, n, d) \
  (((n) == (d)) ? (v) : ((((v) * (n)) + ((d) / 2)) / (d)))

bool helper_tuple_to_int(Tuple* tuple, int* value);
bool helper_color_equal(GColor first, GColor second);

bool helper_replace_color_in_bitmap(GBitmap* bmp, GColor color1, GColor color2);

bool helper_swap_colors_in_bitmap(GBitmap* bmp, GColor color1, GColor color2);
