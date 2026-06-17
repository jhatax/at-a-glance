#include "helper.h"
#include <limits.h>

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

static int helper_get_colors_in_bmp(GBitmap* bmp) {
  if (!bmp) {
    return 0;
  }

  switch (gbitmap_get_format(bmp)) {
    case GBitmapFormat1BitPalette:
      return 2;
      break;
    case GBitmapFormat2BitPalette:
      return 4;
      break;
    case GBitmapFormat4BitPalette:
      return 16;
      break;
    default:
      return 0; // PNG isn't palettized
  }
}

bool helper_replace_color_in_bitmap(GBitmap* bmp, GColor originalColor, GColor newColor) {
  bool replaced = false;
  if (!bmp) {
    return replaced;
  }

  // 1. Determine the exact palette size mathematically based on the format
  int num_colors = helper_get_colors_in_bmp(bmp);
  if (num_colors == 0) {
    return replaced;
  }

  GColor* palette = gbitmap_get_palette(bmp);
  if (!palette) {
    return replaced;
  }

  // 2. Loop through the calculated palette allocation block
  for (int i = 0; i < num_colors; ++i) {
    if (helper_color_equal(palette[i], originalColor)) {
      palette[i] = newColor;
      replaced = true;
      break;
    }
  }

  return replaced;
}

bool helper_swap_colors_in_bitmap(GBitmap* bmp, GColor color1, GColor color2) {
  bool swapped = false;
  if (!bmp) {
    return swapped;
  }

  if (helper_color_equal(color1, color2)) {
    // Same color, so sure, they were swapped with each other
    return true;
  }

  // 1. Determine the exact palette size mathematically based on the format
  int num_colors = helper_get_colors_in_bmp(bmp);
  if (num_colors == 0) {
    return swapped;
  }

  GColor* palette = gbitmap_get_palette(bmp);
  if (!palette) {
    return swapped;
  }
  int pos_1 = -1;
  int pos_2 = -1;
  int i = 0;
  GColor current = {0};
  // 2. Loop through the calculated palette allocation block
  while ((i<num_colors) && ((pos_1 < 0) || (pos_2 <0))) {
    current = palette[i];
    if (helper_color_equal(current, color1)) {
      pos_1 = i;
    } else if (helper_color_equal(current, color2)) {
      pos_2 = i;
    }
    ++i;
  }

  if((pos_1 >= 0) && (pos_2 >= 0)) {
    // They cannot be at the same position because we established
    // that they are different colors before starting
    GColor toSwap = palette[pos_1];
    palette[pos_1] = palette[pos_2];
    palette[pos_2] = toSwap;
    swapped = true;
  }

  return swapped;
}
