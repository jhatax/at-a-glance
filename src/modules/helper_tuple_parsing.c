#include "helper_tuple_parsing.h"

#include <stdio.h>
#include <string.h>

bool helper_tuple_to_int(
    Tuple* tuple,
    int* value) {
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
        if (parsed > (INT16_MAX - digit) / 10) {
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

bool helper_tuple_to_string(
    Tuple* tuple,
    char* value,
    uint8_t max_len) {
  if (!tuple || !value || !max_len) {
    return false;
  }

  if (TUPLE_CSTRING == tuple->type) {
    memset(value, 0, max_len * sizeof(*value));
    const char* text = tuple->value->cstring;
    if (!text) {
      return false;
    }
    // If the empty string is sent, that means location couldn't be found
    snprintf(value, max_len, "%s", text);
    return true;
  }

  return false;
}
