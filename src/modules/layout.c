#include "layout.h"
#include "helper.h"
#include "layout_rect.h"
#include "layout_stylist.h"
#include "../c/ataglance.h"

static bool is_valid_design_x_coord(int16_t x, int16_t design_width) {
  return (x >=0 && x <=design_width);
}

static bool is_valid_design_y_coord(int16_t y, int16_t design_height) {
  return (y >=0 && y <=design_height);
}

int16_t layout_scale_icon_x(
  const GSize* size,
  int16_t coord) {
  if (!size || !is_valid_design_x_coord(coord, ATAGLANCE_DESIGN_ICON_WIDTH)) {
    return 0;
  }

  return helper_scale_round(
      coord,
      size->w,
      ATAGLANCE_DESIGN_ICON_WIDTH);
}

int16_t layout_scale_icon_y(
  const GSize* size,
  int16_t coord) {
  if (!size || !is_valid_design_y_coord(coord, ATAGLANCE_DESIGN_ICON_HEIGHT)) {
    return 0;
  }

  return helper_scale_round(
      coord,
      size->h,
      ATAGLANCE_DESIGN_ICON_HEIGHT
  );
}

int16_t layout_scale_icon_coord(const GSize* size, int16_t coord) {
  if (!size) {
    return 0;
  }

  if (!(is_valid_design_x_coord(coord, ATAGLANCE_DESIGN_ICON_WIDTH)
    || is_valid_design_y_coord(coord, ATAGLANCE_DESIGN_ICON_HEIGHT))) {
    return 0;
  }

  int16_t chosen_dimension = helper_min(size->w, size->h);
  int16_t design_dimension = (chosen_dimension == size->w) ?
    ATAGLANCE_DESIGN_ICON_WIDTH : ATAGLANCE_DESIGN_ICON_HEIGHT;
  return helper_scale_round(coord, chosen_dimension, design_dimension);
}

GPoint layout_scaled_icon_point(const GSize* size, int16_t x, int16_t y) {
  // 1. Guard Clause: Invalid coordinates always fall back to (0,0)
 if (!(is_valid_design_x_coord(x, ATAGLANCE_DESIGN_ICON_WIDTH) &&
   is_valid_design_y_coord(y, ATAGLANCE_DESIGN_ICON_HEIGHT))) {
   return GPoint(0, 0);
 }

 // 2. Rule: If size is provided, calculate the scaled point
 if (size) {
   return GPoint(
       layout_scale_icon_x(size, x),
       layout_scale_icon_y(size, y)
   );
 }

 // 3. Rule: If size is NULL (and coordinates are valid), return unscaled point
 return GPoint(x, y);
}

GColor layout_color_for_role(
    const ColorPalette* palette,
    WatchfaceColorRole role) {
  if (!palette) {
    return GColorWhite;
  }

  switch (role) {
    case WATCHFACE_COLOR_ROLE_PRIMARY_TEXT:
      return palette->primary_text;
    case WATCHFACE_COLOR_ROLE_UNAVAILABLE_TEXT:
      return palette->unavailable_text;
    case WATCHFACE_COLOR_ROLE_DATE:
      return palette->date;
    case WATCHFACE_COLOR_ROLE_TIME:
      return palette->time;
    case WATCHFACE_COLOR_ROLE_STEPS_ICON:
      return palette->steps_icon;
    case WATCHFACE_COLOR_ROLE_DYNAMIC:
      return palette->primary_text;
    default:
      return palette->primary_text;
  }
}

void layout_update_text_layer(
    TextLayer* layer,
    const char* text,
    GColor text_color) {
  if (!layer || !text) {
    return;
  }

  text_layer_set_background_color(layer, GColorClear);
  text_layer_set_text_color(layer, text_color);
  text_layer_set_text(layer, text);
}

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
