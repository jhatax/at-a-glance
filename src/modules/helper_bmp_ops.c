#include "helper.h"

static int helper_get_colors_in_bmp(
    GBitmap* bmp) {
  if (!bmp) {
    return 0;
  }

  // Bitmap palette mutation only works for palettized PNG resources.
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
      return 0;  // PNG isn't palettized
  }
}

bool helper_replace_color_in_bitmap(
    GBitmap* bmp,
    GColor originalColor,
    GColor newColor) {
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
  for (uint8_t i = 0; i < num_colors; ++i) {
    if (HELPER_COLOR_EQUAL(palette[i], originalColor)) {
      palette[i] = newColor;
      replaced = true;
      break;
    }
  }

  return replaced;
}
