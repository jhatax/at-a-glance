#include "weather.h"
#include "helper.h"

static const int16_t c_weather_condition_unknown = -1;

typedef enum {
  WEATHER_ICON_CLEAR = 0,
  WEATHER_ICON_CLOUD,
  WEATHER_ICON_FOG,
  WEATHER_ICON_DRIZZLE,
  WEATHER_ICON_RAIN,
  WEATHER_ICON_FROZEN_RAIN,
  WEATHER_ICON_SNOW,
  WEATHER_ICON_SHOWERS,
  WEATHER_ICON_SNOW_SHOWERS,
  WEATHER_ICON_THUNDERSTORM,
  WEATHER_ICON_UNKNOWN,
} WeatherIconKind;

static Layer* s_weather_icon_layer;
static int16_t s_weather_condition;
static GColor s_weather_icon_color;
static const VisualPalette* s_weather_palette;
static bool s_weather_is_available;

static void draw_weather_sun(
    GContext* ctx,
    const GSize* bounds_size,
    int16_t center_x,
    int16_t center_y,
    int16_t radius);
static void draw_weather_cloud(
    GContext* ctx,
    const GSize* bounds_size,
    int16_t x,
    int16_t y);
static void draw_weather_rain(
    GContext* ctx,
    const GSize* bounds_size,
    bool heavy);
static void draw_weather_drop(
    GContext* ctx,
    const GSize* bounds_size,
    int16_t x,
    int16_t y);
static void draw_weather_drizzle(
    GContext* ctx,
    const GSize* bounds_size);
static void draw_weather_sleet(
    GContext* ctx,
    const GSize* bounds_size);
static void draw_weather_snowflake(
    GContext* ctx,
    const GSize* bounds_size,
    int16_t center_x,
    int16_t center_y);
static void draw_weather_snow_dots(
    GContext* ctx,
    const GSize* bounds_size);
static void draw_weather_lightning(
    GContext* ctx,
    const GSize* bounds_size);
static inline void draw_weather_clear_icon(
    GContext* ctx,
    const GSize* bounds_size);
static inline void draw_weather_cloud_icon(
    GContext* ctx,
    const GSize* bounds_size);
static inline void draw_weather_fog_icon(
    GContext* ctx,
    const GSize* bounds_size);
static inline void draw_weather_drizzle_icon(
    GContext* ctx,
    const GSize* bounds_size);
static inline void draw_weather_rain_icon(
    GContext* ctx,
    const GSize* bounds_size);
static inline void draw_weather_frozen_rain_icon(
    GContext* ctx,
    const GSize* bounds_size);
static inline void draw_weather_snow_icon(
    GContext* ctx,
    const GSize* bounds_size);
static inline void draw_weather_showers_icon(
    GContext* ctx,
    const GSize* bounds_size);
static inline void draw_weather_snow_showers_icon(
    GContext* ctx,
    const GSize* bounds_size);
static inline void draw_weather_thunderstorm_icon(
    GContext* ctx,
    const GSize* bounds_size);
static inline void draw_weather_unknown_icon(
    GContext* ctx,
    const GSize* bounds_size);
static inline void draw_weather_unavailable_icon(
    GContext* ctx,
    const GSize* bounds_size);
static WeatherIconKind get_weather_icon_kind(
    int16_t weather_condition);
static void draw_weather_icon(
    GContext* ctx,
    const GSize* bounds_size,
    WeatherIconKind icon_kind);
static void weather_icon_update_proc(Layer* layer, GContext* ctx);

static void draw_weather_sun(
    GContext* ctx,
    const GSize* bounds_size,
    int16_t center_x,
    int16_t center_y,
    int16_t radius) {
  GPoint center = helper_get_scaled_icon_point(
      bounds_size,
      center_x,
      center_y);
  int16_t scaled_radius = helper_scale_icon_coord(
      bounds_size,
      radius);

  graphics_context_set_stroke_width(ctx, 2);
  graphics_fill_circle(ctx, center, scaled_radius);
  helper_draw_scaled_icon_line(
      ctx,
      bounds_size,
      center_x,
      center_y - radius - 6,
      center_x,
      center_y - radius - 2);
  helper_draw_scaled_icon_line(
      ctx,
      bounds_size,
      center_x,
      center_y + radius + 2,
      center_x,
      center_y + radius + 6);
  helper_draw_scaled_icon_line(
      ctx,
      bounds_size,
      center_x - radius - 6,
      center_y,
      center_x - radius - 2,
      center_y);
  helper_draw_scaled_icon_line(
      ctx,
      bounds_size,
      center_x + radius + 2,
      center_y,
      center_x + radius + 6,
      center_y);
  helper_draw_scaled_icon_line(
      ctx,
      bounds_size,
      center_x - radius - 4,
      center_y - radius - 4,
      center_x - radius - 1,
      center_y - radius - 1);
  helper_draw_scaled_icon_line(
      ctx,
      bounds_size,
      center_x + radius + 1,
      center_y - radius - 1,
      center_x + radius + 4,
      center_y - radius - 4);
  helper_draw_scaled_icon_line(
      ctx,
      bounds_size,
      center_x - radius - 4,
      center_y + radius + 4,
      center_x - radius - 1,
      center_y + radius + 1);
  helper_draw_scaled_icon_line(
      ctx,
      bounds_size,
      center_x + radius + 1,
      center_y + radius + 1,
      center_x + radius + 4,
      center_y + radius + 4);
}

static void draw_weather_cloud(
    GContext* ctx,
    const GSize* bounds_size,
    int16_t x,
    int16_t y) {
  int upper_x = helper_scale_icon_x(bounds_size, x + 3);
  int upper_y = helper_scale_icon_y(bounds_size, y + 3);
  int upper_w = helper_scale_icon_x(bounds_size, 17);
  int upper_h = helper_scale_icon_y(bounds_size, 8);
  int base_x = helper_scale_icon_x(bounds_size, x + 1);
  int base_y = helper_scale_icon_y(bounds_size, y + 8);
  int base_w = helper_scale_icon_x(bounds_size, 22);
  int base_h = helper_scale_icon_y(bounds_size, 5);
  int left_radius = helper_scale_icon_coord(bounds_size, 5);
  int crown_radius = helper_scale_icon_coord(bounds_size, 7);
  int right_radius = helper_scale_icon_coord(bounds_size, 5);

  graphics_fill_circle(ctx,
                       helper_get_scaled_icon_point(
                           bounds_size,
                           x + 6,
                           y + 6),
                       left_radius);
  graphics_fill_circle(ctx,
                       helper_get_scaled_icon_point(
                           bounds_size,
                           x + 12,
                           y + 3),
                       crown_radius);
  graphics_fill_circle(ctx,
                       helper_get_scaled_icon_point(
                           bounds_size,
                           x + 19,
                           y + 7),
                       right_radius);
  graphics_fill_rect(ctx, GRect(upper_x,
                                upper_y,
                                upper_w,
                                upper_h),
                     0,
                     GCornerNone);
  graphics_fill_rect(ctx, GRect(base_x,
                                base_y,
                                base_w,
                                base_h),
                     0,
                     GCornerNone);
}

static void draw_weather_drop(
    GContext* ctx,
    const GSize* bounds_size,
    int16_t x,
    int16_t y) {
  int radius = helper_scale_icon_coord(bounds_size, 1);

  if (radius < 1) {
    radius = 1;
  }

  graphics_context_set_stroke_width(ctx, 2);
  helper_draw_scaled_icon_line(
      ctx,
      bounds_size,
      x + 1,
      y,
      x - 1,
      y + 5);
  graphics_fill_circle(
      ctx,
      helper_get_scaled_icon_point(bounds_size, x - 1, y + 5),
      radius);
}

static void draw_weather_rain(
    GContext* ctx,
    const GSize* bounds_size,
    bool heavy) {
  draw_weather_drop(ctx, bounds_size, 8, 19);
  draw_weather_drop(ctx, bounds_size, 15, 20);
  if (heavy) {
    draw_weather_drop(ctx, bounds_size, 22, 19);
  }
}

static void draw_weather_drizzle(
    GContext* ctx,
    const GSize* bounds_size) {
  graphics_context_set_stroke_width(ctx, 1);
  helper_draw_scaled_icon_line(ctx, bounds_size, 8, 22, 7, 24);
  helper_draw_scaled_icon_line(ctx, bounds_size, 14, 22, 13, 24);
  helper_draw_scaled_icon_line(ctx, bounds_size, 20, 22, 19, 24);
}

static void draw_weather_sleet(
    GContext* ctx,
    const GSize* bounds_size) {
  int radius = helper_scale_icon_coord(bounds_size, 2);

  if (radius < 1) {
    radius = 1;
  }

  draw_weather_drop(ctx, bounds_size, 8, 19);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_circle(
      ctx,
      helper_get_scaled_icon_point(bounds_size, 17, 23),
      radius);
  graphics_draw_circle(
      ctx,
      helper_get_scaled_icon_point(bounds_size, 23, 22),
      radius);
}

static void draw_weather_snowflake(
    GContext* ctx,
    const GSize* bounds_size,
    int16_t center_x,
    int16_t center_y) {
  int x = helper_scale_icon_x(bounds_size, center_x);
  int y = helper_scale_icon_y(bounds_size, center_y);
  int r = helper_scale_icon_coord(bounds_size, 4);
  int branch = helper_scale_icon_coord(bounds_size, 2);

  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_line(ctx, GPoint(x - r, y), GPoint(x + r, y));
  graphics_draw_line(ctx, GPoint(x, y - r), GPoint(x, y + r));
  graphics_draw_line(ctx, GPoint(x - r, y - r),
                     GPoint(x + r, y + r));
  graphics_draw_line(ctx, GPoint(x - r, y + r),
                     GPoint(x + r, y - r));
  graphics_draw_line(ctx, GPoint(x - r, y),
                     GPoint(x - r + branch, y - branch));
  graphics_draw_line(ctx, GPoint(x + r, y),
                     GPoint(x + r - branch, y + branch));
}

static void draw_weather_snow_dots(
    GContext* ctx,
    const GSize* bounds_size) {
  int radius = helper_scale_icon_coord(bounds_size, 1);

  if (radius < 1) {
    radius = 1;
  }

  graphics_fill_circle(
      ctx,
      helper_get_scaled_icon_point(bounds_size, 7, 24),
      radius);
  graphics_fill_circle(
      ctx,
      helper_get_scaled_icon_point(bounds_size, 22, 24),
      radius);
}

static void draw_weather_lightning(
    GContext* ctx,
    const GSize* bounds_size) {
  graphics_context_set_stroke_width(ctx, 1);
  helper_draw_scaled_icon_line(ctx, bounds_size, 15, 16, 19, 16);
  helper_draw_scaled_icon_line(ctx, bounds_size, 14, 17, 18, 17);
  helper_draw_scaled_icon_line(ctx, bounds_size, 14, 18, 17, 18);
  helper_draw_scaled_icon_line(ctx, bounds_size, 13, 19, 17, 19);
  helper_draw_scaled_icon_line(ctx, bounds_size, 13, 20, 21, 20);
  helper_draw_scaled_icon_line(ctx, bounds_size, 12, 21, 20, 21);
  helper_draw_scaled_icon_line(ctx, bounds_size, 12, 22, 18, 22);
  helper_draw_scaled_icon_line(ctx, bounds_size, 11, 23, 18, 23);
  helper_draw_scaled_icon_line(ctx, bounds_size, 11, 24, 17, 24);
  helper_draw_scaled_icon_line(ctx, bounds_size, 10, 25, 16, 25);
  helper_draw_scaled_icon_line(ctx, bounds_size, 10, 26, 15, 26);
}

static inline void draw_weather_clear_icon(
    GContext* ctx,
    const GSize* bounds_size) {
  draw_weather_sun(ctx, bounds_size, 14, 14, 5);
}

static inline void draw_weather_cloud_icon(
    GContext* ctx,
    const GSize* bounds_size) {
  draw_weather_cloud(ctx, bounds_size, 2, 9);
}

static inline void draw_weather_fog_icon(
    GContext* ctx,
    const GSize* bounds_size) {
  draw_weather_cloud(ctx, bounds_size, 2, 6);
  graphics_context_set_stroke_width(ctx, 2);
  helper_draw_scaled_icon_line(ctx, bounds_size, 3, 18, 25, 18);
  helper_draw_scaled_icon_line(ctx, bounds_size, 6, 22, 26, 22);
  helper_draw_scaled_icon_line(ctx, bounds_size, 2, 26, 20, 26);
}

static inline void draw_weather_drizzle_icon(
    GContext* ctx,
    const GSize* bounds_size) {
  draw_weather_cloud(ctx, bounds_size, 2, 7);
  draw_weather_drizzle(ctx, bounds_size);
}

static inline void draw_weather_rain_icon(
    GContext* ctx,
    const GSize* bounds_size) {
  draw_weather_cloud(ctx, bounds_size, 2, 6);
  draw_weather_rain(ctx, bounds_size, true);
}

static inline void draw_weather_frozen_rain_icon(
    GContext* ctx,
    const GSize* bounds_size) {
  draw_weather_cloud(ctx, bounds_size, 2, 6);
  draw_weather_sleet(ctx, bounds_size);
}

static inline void draw_weather_snow_icon(
    GContext* ctx,
    const GSize* bounds_size) {
  draw_weather_cloud(ctx, bounds_size, 2, 6);
  draw_weather_snowflake(ctx, bounds_size, 15, 22);
  draw_weather_snow_dots(ctx, bounds_size);
}

static inline void draw_weather_showers_icon(
    GContext* ctx,
    const GSize* bounds_size) {
  draw_weather_sun(ctx, bounds_size, 9, 9, 3);
  draw_weather_cloud(ctx, bounds_size, 3, 7);
  draw_weather_rain(ctx, bounds_size, true);
}

static inline void draw_weather_snow_showers_icon(
    GContext* ctx,
    const GSize* bounds_size) {
  draw_weather_sun(ctx, bounds_size, 9, 9, 3);
  draw_weather_cloud(ctx, bounds_size, 3, 6);
  draw_weather_snowflake(ctx, bounds_size, 15, 22);
  draw_weather_snow_dots(ctx, bounds_size);
}

static inline void draw_weather_thunderstorm_icon(
    GContext* ctx,
    const GSize* bounds_size) {
  draw_weather_cloud(ctx, bounds_size, 2, 5);
  draw_weather_lightning(ctx, bounds_size);
}

static inline void draw_weather_unknown_icon(
    GContext* ctx,
    const GSize* bounds_size) {
  graphics_context_set_stroke_width(ctx, 2);
  helper_draw_scaled_icon_line(ctx, bounds_size, 11, 9, 14, 6);
  helper_draw_scaled_icon_line(ctx, bounds_size, 14, 6, 18, 8);
  helper_draw_scaled_icon_line(ctx, bounds_size, 18, 8, 18, 12);
  helper_draw_scaled_icon_line(ctx, bounds_size, 18, 12, 14, 16);
  helper_draw_scaled_icon_line(ctx, bounds_size, 14, 16, 14, 20);
  graphics_fill_circle(ctx,
                       helper_get_scaled_icon_point(
                           bounds_size,
                           14,
                           24),
                       1);
}

static inline void draw_weather_unavailable_icon(
    GContext* ctx,
    const GSize* bounds_size) {
  graphics_context_set_stroke_width(ctx, 2);

  graphics_draw_circle(
      ctx,
      helper_get_scaled_icon_point(bounds_size, 9, 15),
      helper_scale_icon_coord(bounds_size, 4));
  graphics_draw_circle(
      ctx,
      helper_get_scaled_icon_point(bounds_size, 15, 12),
      helper_scale_icon_coord(bounds_size, 6));
  graphics_draw_circle(
      ctx,
      helper_get_scaled_icon_point(bounds_size, 21, 15),
      helper_scale_icon_coord(bounds_size, 4));
  helper_draw_scaled_icon_line(ctx, bounds_size, 5, 18, 11, 18);
  helper_draw_scaled_icon_line(ctx, bounds_size, 19, 18, 25, 18);
  helper_draw_scaled_icon_line(ctx, bounds_size, 23, 7, 6, 24);
}

static WeatherIconKind get_weather_icon_kind(
    int16_t weather_condition) {
  if (weather_condition == 0) {
    return WEATHER_ICON_CLEAR;
  }
  if (weather_condition < 0) {
    return WEATHER_ICON_UNKNOWN;
  }
  if (weather_condition <= 3) {
    return WEATHER_ICON_CLOUD;
  }
  if (weather_condition <= 48) {
    return WEATHER_ICON_FOG;
  }
  if (weather_condition <= 55) {
    return WEATHER_ICON_DRIZZLE;
  }
  if (weather_condition <= 57) {
    return WEATHER_ICON_FROZEN_RAIN;
  }
  if (weather_condition <= 65) {
    return WEATHER_ICON_RAIN;
  }
  if (weather_condition <= 67) {
    return WEATHER_ICON_FROZEN_RAIN;
  }
  if (weather_condition <= 75) {
    return WEATHER_ICON_SNOW;
  }
  if (weather_condition <= 77) {
    return WEATHER_ICON_SNOW;
  }
  if (weather_condition <= 82) {
    return WEATHER_ICON_SHOWERS;
  }
  if (weather_condition <= 86) {
    return WEATHER_ICON_SNOW_SHOWERS;
  }
  if (weather_condition <= 99) {
    return WEATHER_ICON_THUNDERSTORM;
  }

  return WEATHER_ICON_UNKNOWN;
}

static void draw_weather_icon(
    GContext* ctx,
    const GSize* bounds_size,
    WeatherIconKind icon_kind) {
  switch (icon_kind) {
    case WEATHER_ICON_CLEAR:
      draw_weather_clear_icon(ctx, bounds_size);
      break;
    case WEATHER_ICON_CLOUD:
      draw_weather_cloud_icon(ctx, bounds_size);
      break;
    case WEATHER_ICON_FOG:
      draw_weather_fog_icon(ctx, bounds_size);
      break;
    case WEATHER_ICON_DRIZZLE:
      draw_weather_drizzle_icon(ctx, bounds_size);
      break;
    case WEATHER_ICON_RAIN:
      draw_weather_rain_icon(ctx, bounds_size);
      break;
    case WEATHER_ICON_FROZEN_RAIN:
      draw_weather_frozen_rain_icon(ctx, bounds_size);
      break;
    case WEATHER_ICON_SNOW:
      draw_weather_snow_icon(ctx, bounds_size);
      break;
    case WEATHER_ICON_SHOWERS:
      draw_weather_showers_icon(ctx, bounds_size);
      break;
    case WEATHER_ICON_SNOW_SHOWERS:
      draw_weather_snow_showers_icon(ctx, bounds_size);
      break;
    case WEATHER_ICON_THUNDERSTORM:
      draw_weather_thunderstorm_icon(ctx, bounds_size);
      break;
    case WEATHER_ICON_UNKNOWN:
      draw_weather_unknown_icon(ctx, bounds_size);
      break;
  }
}

static void weather_icon_update_proc(Layer* layer, GContext* ctx) {
  if (!layer || !ctx) {
    return;
  }

  GRect bounds = layer_get_bounds(layer);
  if (!s_weather_palette) {
    return;
  }

  graphics_context_set_fill_color(ctx, s_weather_palette->background);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  graphics_context_set_fill_color(ctx, s_weather_icon_color);
  graphics_context_set_stroke_color(ctx, s_weather_icon_color);

  if (!s_weather_is_available) {
    draw_weather_unavailable_icon(ctx, &bounds.size);
    return;
  }

  draw_weather_icon(
      ctx,
      &bounds.size,
      get_weather_icon_kind(s_weather_condition));
}

void weather_icon_init(void) {
  s_weather_icon_layer = NULL;
  s_weather_condition = c_weather_condition_unknown;
  s_weather_icon_color = GColorClear;
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

  weather_icon_mark_dirty();
}

void weather_icon_mark_dirty(void) {
  if (s_weather_icon_layer) {
    layer_mark_dirty(s_weather_icon_layer);
  }
}
