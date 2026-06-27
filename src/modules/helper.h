#pragma once

#include <pebble.h>

#define HELPER_MAX(a, b) (((a) > (b)) ? (a) : (b))

#define HELPER_MIN(a, b) (((a) < (b)) ? (a) : (b))

#define HELPER_VALID_DESIGN_X(x) (((x) >= 0) && ((x) < WATCHFACE_ICON_GRID_WIDTH))
#define HELPER_VALID_DESIGN_Y(y) (((y) >= 0) && ((y) < WATCHFACE_ICON_GRID_HEIGHT))

#define HELPER_CLAMP_MIN(exp, min) HELPER_MAX((exp), (min))
#define HELPER_CLAMP_MAX(exp, max) HELPER_MIN((exp), (max))
#define HELPER_CLAMP_TO_RANGE(exp, min, max)                                                       \
  HELPER_CLAMP_MAX((HELPER_CLAMP_MIN((exp), (min))), (max))

#define HELPER_IF_ELSE(exp, a, b) ((exp) ? (a) : (b))

#define HELPER_ROUND_UP(v, d) ((v) + ((d) / 2)) / (d)

#define HELPER_SCALE_ROUND(v, n, d) (((n) == (d)) ? (v) : ((((v) * (n)) + ((d) / 2)) / (d)))

#define HELPER_COLOR_EQUAL(c1, c2) ((((GColor)c1).argb) == (((GColor)c2).argb))

#define MODULE_PALETTE_LOADED(pal) (!(HELPER_COLOR_EQUAL(((pal).normal), ((pal).background))))

bool helper_tuple_to_int(Tuple *tuple, int *value);

bool helper_replace_color_in_bitmap(GBitmap *bmp, GColor color1, GColor color2);

bool helper_swap_colors_in_bitmap(GBitmap *bmp, GColor color1, GColor color2);
