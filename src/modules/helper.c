#include "helper.h"
#include <limits.h>

int16_t helper_max(int16_t a, int16_t b) {
  return (a > b) ? a : b;
}

int16_t helper_min(int16_t a, int16_t b) {
  return (a < b) ? a : b;
}

int16_t helper_scale_round(
    int16_t input_value,
    int16_t numerator,
    int16_t denominator) {
  return ((input_value * numerator) + (denominator / 2)) /
      denominator;
}

bool helper_tuple_to_int(Tuple* tuple, int* value) {
  if (!tuple || !value) {
    return false;
  }

  switch (tuple->type) {
    case TUPLE_INT:
      *value = (int)tuple->value->int32;
      return true;

    case TUPLE_CSTRING: {
        const char* text = tuple->value->cstring;
        int parsed = 0;
        if (!text || text[0] == '\0') {
          return false;
        }
        for (const char* p = text; *p; ++p) {
          if (*p < '0' || *p > '9') {
            return false;
          }
          int digit = *p - '0';
          if (parsed > (INT_MAX - digit) / 10) {
            return false;
          }
          parsed = (parsed * 10) + digit;
        }
        *value = parsed;
        return true;
      }

    default:
      return false;
  }
}

bool helper_color_equal(GColor first, GColor second) {
  return first.argb == second.argb;
}
