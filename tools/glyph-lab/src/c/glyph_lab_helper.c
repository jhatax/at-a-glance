#include "glyph_lab_helper.h"

bool helper_color_equal(GColor first, GColor second) {
  return first.argb == second.argb;
}

static int helper_get_colors_in_bitmap(GBitmap* bmp) {
  if (!bmp) {
    return 0;
  }

  switch (gbitmap_get_format(bmp)) {
    case GBitmapFormat1BitPalette:
      return 2;
    case GBitmapFormat2BitPalette:
      return 4;
    case GBitmapFormat4BitPalette:
      return 16;
    default:
      return 0;
  }
}

bool helper_replace_color_in_bitmap(
    GBitmap* bmp,
    GColor original_color,
    GColor new_color) {
  if (!bmp) {
    return false;
  }

  int colors_in_bitmap = helper_get_colors_in_bitmap(bmp);
  if (colors_in_bitmap == 0) {
    return false;
  }

  GColor* palette = gbitmap_get_palette(bmp);
  if (!palette) {
    return false;
  }

  for (int i = 0; i < colors_in_bitmap; ++i) {
    if (helper_color_equal(palette[i], original_color)) {
      palette[i] = new_color;
      return true;
    }
  }

  return false;
}
