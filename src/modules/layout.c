#include "layout.h"
#include "layout_rect.h"
#include "layout_stylist.h"

void layout_update_surface_style(
    WatchfaceSurface* surface,
    uint8_t display_mode) {
  if (!surface) {
    return;
  }

  layout_stylist_update_surface_style(
      surface->face_width,
      surface->face_height,
      display_mode,
      &surface->style);
}

void layout_calculate_surface(
    int16_t face_width,
    int16_t face_height,
    uint8_t display_mode,
    WatchfaceSurface* surface) {
  if (!surface) {
    return;
  }

  memset(surface, 0, sizeof(*surface));

#ifdef PBL_RECT
  layout_rect_calculate_surface(face_width, face_height, surface);
#endif

  layout_update_surface_style(surface, display_mode);
}
