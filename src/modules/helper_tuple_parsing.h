#pragma once

#include <pebble.h>

bool helper_tuple_to_int(
    Tuple* tuple,
    int* value);

bool helper_tuple_to_string(
    Tuple* tuple,
    char* value,
    uint8_t max_len);
