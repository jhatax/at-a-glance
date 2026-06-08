#pragma once

#include <pebble.h>

int16_t helper_max(int16_t a, int16_t b);

int16_t helper_min(int16_t a, int16_t b);

int16_t helper_scale_round(
    int16_t input_value,
    int16_t numerator,
    int16_t denominator);

bool helper_tuple_to_int(Tuple* tuple, int* value);
bool helper_color_equal(GColor first, GColor second);
