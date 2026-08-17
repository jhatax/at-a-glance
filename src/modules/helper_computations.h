#pragma once

#include <pebble.h>

#define HELPER_MAX(a, b) (((a) > (b)) ? (a) : (b))

#define HELPER_MIN(a, b) (((a) < (b)) ? (a) : (b))

#define HELPER_VALUE_IN_RANGE(value, min, max) (((value) >= (min)) && ((value) <= (max)))
#define HELPER_CLAMP_MIN(exp, min) HELPER_MAX((exp), (min))
#define HELPER_CLAMP_MAX(exp, max) HELPER_MIN((exp), (max))
#define HELPER_CLAMP_TO_RANGE(exp, min, max) \
  HELPER_CLAMP_MAX((HELPER_CLAMP_MIN((exp), (min))), (max))

#define HELPER_IF_ELSE(exp, a, b) ((exp) ? (a) : (b))

#define HELPER_SCALE_ROUND(v, n, d) \
  (((n) == (d))                     \
       ? (v)                        \
       : ((((v) >= 0) ? ((((v) * (n)) + ((d) / 2)) / (d)) : ((((v) * (n)) - ((d) / 2)) / (d)))))

#define HELPER_ROUND_UP(v, d) (HELPER_SCALE_ROUND((v), 1, (d)))

#define HELPER_COLOR_EQUAL(c1, c2) ((((GColor)(c1)).argb) == (((GColor)(c2)).argb))
