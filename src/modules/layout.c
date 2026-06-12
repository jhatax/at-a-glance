#include "layout.h"
#include "layout_architect.h"
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
  WatchfaceSurface* surface) {
  if (!surface) {
    return;
  }
  // Wipe the "slate" clean before I lay things as they should be
  memset(surface, 0, sizeof(*surface));

  // Apply blueprint for this device's shape and geometry
  architect_apply_blueprint(face_width, face_height, surface);
}
