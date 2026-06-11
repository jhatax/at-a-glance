#include "layout.h"
#include "layout_rect.h"
#include "layout_stylist.h"
#include "../c/ataglance.h"

void layout_update_surface_style(WatchfaceSurfaceStyle* style, uint8_t display_mode) {
  if (!style) {
    return;
  }

  layout_stylist_update_surface_style(style, display_mode);
}

void layout_calculate_surface(int16_t face_width,
  int16_t face_height,
  uint8_t display_mode,
  WatchfaceSurface* surface) {
  if (!surface) {
    return;
  }
  // Wipe the "slate" clean before I lay things as they should be
  memset(surface, 0, sizeof(*surface));

#ifdef PBL_RECT
  layout_rect_calculate_surface(face_width, face_height, surface);
#elif defined(PBL_ROUND)
#error "Round layout architect is not implemented yet"
#else
#error "Unsupported Pebble display shape"
#endif

  layout_update_surface_style(&(surface->style), display_mode);
}
