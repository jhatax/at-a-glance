#include "helper.h"

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
