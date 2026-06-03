#include "weather.h"

static const int16_t c_icon_size_as_drawn = 28;
static const int16_t c_weather_condition_unknown = -1;

static Layer* s_weather_icon_layer;
static int16_t s_weather_condition;
static GColor s_weather_icon_color;
static GColor s_weather_icon_background_color;
static const VisualPalette* s_weather_palette;
static bool s_weather_is_available;

static inline int16_t get_icon_draw_size(const GSize* bounds_size);
static inline int16_t scale_icon_coord(
    const GSize* bounds_size,
    int16_t coord);
static inline int16_t scale_icon_x(
    const GSize* bounds_size,
    int16_t coord);
static inline int16_t scale_icon_y(
    const GSize* bounds_size,
    int16_t coord);
static void weather_icon_update_proc(Layer* layer, GContext* ctx);

static inline int16_t get_icon_draw_size(const GSize* bounds_size) {
  if (!bounds_size) {
    return 0;
  }

  return bounds_size->w < bounds_size->h ?
      bounds_size->w : bounds_size->h;
}

static inline int16_t scale_icon_coord(
    const GSize* bounds_size,
    int16_t coord) {
  int16_t draw_size = get_icon_draw_size(bounds_size);
  return (coord * draw_size) / c_icon_size_as_drawn;
}

static inline int16_t scale_icon_x(
    const GSize* bounds_size,
    int16_t coord) {
  if (!bounds_size) {
    return 0;
  }

  return (coord * bounds_size->w) / c_icon_size_as_drawn;
}

static inline int16_t scale_icon_y(
    const GSize* bounds_size,
    int16_t coord) {
  if (!bounds_size) {
    return 0;
  }

  return (coord * bounds_size->h) / c_icon_size_as_drawn;
}

static void weather_icon_update_proc(Layer* layer, GContext* ctx) {
  if (!layer || !ctx) {
    return;
  }

  GRect bounds = layer_get_bounds(layer);
  int cloud_x = scale_icon_x(&bounds.size, 5);
  int cloud_y = scale_icon_y(&bounds.size, 14);
  int cloud_w = scale_icon_x(&bounds.size, 18);
  int cloud_h = scale_icon_y(&bounds.size, 7);
  GPoint sun_center = GPoint(
      scale_icon_x(&bounds.size, 14),
      scale_icon_y(&bounds.size, 14));
  int sun_radius = scale_icon_coord(&bounds.size, 6);

  graphics_context_set_fill_color(ctx, s_weather_icon_background_color);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  if (!s_weather_is_available) {
    return;
  }

  graphics_context_set_fill_color(ctx, s_weather_icon_color);
  graphics_context_set_stroke_color(ctx, s_weather_icon_color);

  if (s_weather_condition == 0) {
    graphics_context_set_stroke_width(ctx, 2);
    graphics_fill_circle(ctx, sun_center, sun_radius);
    graphics_draw_line(ctx, GPoint(sun_center.x, 2),
                       GPoint(sun_center.x, 6));
    graphics_draw_line(ctx, GPoint(sun_center.x, 22),
                       GPoint(sun_center.x, 26));
    graphics_draw_line(ctx, GPoint(2, sun_center.y),
                       GPoint(6, sun_center.y));
    graphics_draw_line(ctx, GPoint(22, sun_center.y),
                       GPoint(26, sun_center.y));
    return;
  }

  graphics_fill_circle(ctx, GPoint(cloud_x + 5, cloud_y), 5);
  graphics_fill_circle(ctx, GPoint(cloud_x + 11, cloud_y - 2), 6);
  graphics_fill_circle(ctx, GPoint(cloud_x + 16, cloud_y + 1), 4);
  graphics_fill_rect(ctx, GRect(cloud_x,
                                cloud_y,
                                cloud_w,
                                cloud_h),
                     0,
                     GCornerNone);
}

void weather_icon_init(void) {
  s_weather_icon_layer = NULL;
  s_weather_condition = c_weather_condition_unknown;
  s_weather_icon_color = GColorClear;
  s_weather_icon_background_color = GColorClear;
  s_weather_palette = NULL;
  s_weather_is_available = false;
}

void weather_icon_create(
    Layer* root,
    const GRect* frame,
    const VisualPalette* palette) {
  if (!root || !frame || !palette) {
    return;
  }

  s_weather_palette = palette;
  s_weather_icon_color = palette->unavailable_text;
  s_weather_icon_background_color =
      palette->unavailable_text_background;

  s_weather_icon_layer = layer_create(*frame);
  if (s_weather_icon_layer) {
    layer_set_update_proc(
        s_weather_icon_layer,
        weather_icon_update_proc);
    layer_add_child(root, s_weather_icon_layer);
  }
}

void weather_icon_destroy(void) {
  if (s_weather_icon_layer) {
    layer_destroy(s_weather_icon_layer);
    s_weather_icon_layer = NULL;
  }
}

void weather_icon_set_condition(int16_t weather_condition) {
  s_weather_condition = weather_condition;
}

void weather_icon_update_display(
    bool is_temperature_available,
    const VisualPalette* palette) {
  if (palette) {
    s_weather_palette = palette;
  }
  if (!s_weather_palette) {
    return;
  }

  s_weather_is_available =
      is_temperature_available &&
      s_weather_condition != c_weather_condition_unknown;

  s_weather_icon_color = s_weather_is_available ?
      PBL_IF_COLOR_ELSE(
          GColorChromeYellow,
          s_weather_palette->primary_text) :
      s_weather_palette->unavailable_text;
  s_weather_icon_background_color = s_weather_is_available ?
      s_weather_palette->background :
      s_weather_palette->unavailable_text_background;

  weather_icon_mark_dirty();
}

void weather_icon_mark_dirty(void) {
  if (s_weather_icon_layer) {
    layer_mark_dirty(s_weather_icon_layer);
  }
}
