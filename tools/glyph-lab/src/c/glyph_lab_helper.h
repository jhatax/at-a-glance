#pragma once

#include <pebble.h>

#define HELPER_MAX(a, b) (((a) > (b)) ? (a) : (b))
#define HELPER_MIN(a, b) (((a) < (b)) ? (a) : (b))
#define HELPER_SCALE_ROUND(v, n, d) \
  (((n) == (d)) ? (v) : ((((v) * (n)) + ((d) / 2)) / (d)))

bool helper_color_equal(GColor first, GColor second);
