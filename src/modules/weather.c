#include "weather.h"

static const int16_t c_icon_size_as_drawn = 28;
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
static inline void draw_scaled_line(
    GContext* ctx,
    const GSize* bounds_size,
    int16_t x0,
    int16_t y0,
    int16_t x1,
    int16_t y1);
static inline GPoint get_scaled_point(
    const GSize* bounds_size,
    int16_t x,
    int16_t y);
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
static void draw_weather_drizzle(
    GContext* ctx,
    const GSize* bounds_size);
static void draw_weather_snowflake(
    GContext* ctx,
    const GSize* bounds_size,
    int16_t center_x,
    int16_t center_y);
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
static WeatherIconKind get_weather_icon_kind(
    int16_t weather_condition);
static void draw_weather_icon(
    GContext* ctx,
    const GSize* bounds_size,
    WeatherIconKind icon_kind);
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

static inline void draw_scaled_line(
    GContext* ctx,
    const GSize* bounds_size,
    int16_t x0,
    int16_t y0,
    int16_t x1,
    int16_t y1) {
  graphics_draw_line(ctx,
                     get_scaled_point(bounds_size, x0, y0),
                     get_scaled_point(bounds_size, x1, y1));
}

static inline GPoint get_scaled_point(
    const GSize* bounds_size,
    int16_t x,
    int16_t y) {
  return GPoint(scale_icon_x(bounds_size, x),
                scale_icon_y(bounds_size, y));
}

static void draw_weather_sun(
    GContext* ctx,
    const GSize* bounds_size,
    int16_t center_x,
    int16_t center_y,
    int16_t radius) {
  GPoint center = get_scaled_point(bounds_size, center_x, center_y);
  int16_t scaled_radius = scale_icon_coord(bounds_size, radius);

  graphics_context_set_stroke_width(ctx, 2);
  graphics_fill_circle(ctx, center, scaled_radius);
  draw_scaled_line(ctx, bounds_size, center_x, 2, center_x, 6);
  draw_scaled_line(ctx, bounds_size, center_x, 22, center_x, 26);
  draw_scaled_line(ctx, bounds_size, 2, center_y, 6, center_y);
  draw_scaled_line(ctx, bounds_size, 22, center_y, 26, center_y);
}

static void draw_weather_cloud(
    GContext* ctx,
    const GSize* bounds_size,
    int16_t x,
    int16_t y) {
  int cloud_x = scale_icon_x(bounds_size, x);
  int cloud_y = scale_icon_y(bounds_size, y);
  int cloud_w = scale_icon_x(bounds_size, 18);
  int cloud_h = scale_icon_y(bounds_size, 7);
  int small_radius = scale_icon_coord(bounds_size, 4);
  int medium_radius = scale_icon_coord(bounds_size, 5);
  int large_radius = scale_icon_coord(bounds_size, 6);

  graphics_fill_circle(ctx,
                       get_scaled_point(bounds_size, x + 5, y),
                       medium_radius);
  graphics_fill_circle(ctx,
                       get_scaled_point(bounds_size, x + 11, y - 2),
                       large_radius);
  graphics_fill_circle(ctx,
                       get_scaled_point(bounds_size, x + 16, y + 1),
                       small_radius);
  graphics_fill_rect(ctx, GRect(cloud_x,
                                cloud_y,
                                cloud_w,
                                cloud_h),
                     0,
                     GCornerNone);
}

static void draw_weather_rain(
    GContext* ctx,
    const GSize* bounds_size,
    bool heavy) {
  graphics_context_set_stroke_width(ctx, 2);
  draw_scaled_line(ctx, bounds_size, 9, 20, 7, 25);
  draw_scaled_line(ctx, bounds_size, 15, 20, 13, 25);
  if (heavy) {
    draw_scaled_line(ctx, bounds_size, 21, 20, 19, 25);
  }
}

static void draw_weather_drizzle(
    GContext* ctx,
    const GSize* bounds_size) {
  int radius = scale_icon_coord(bounds_size, 1);

  if (radius < 1) {
    radius = 1;
  }

  graphics_fill_circle(ctx,
                       get_scaled_point(bounds_size, 9, 23),
                       radius);
  graphics_fill_circle(ctx,
                       get_scaled_point(bounds_size, 15, 23),
                       radius);
  graphics_fill_circle(ctx,
                       get_scaled_point(bounds_size, 21, 23),
                       radius);
}

static void draw_weather_snowflake(
    GContext* ctx,
    const GSize* bounds_size,
    int16_t center_x,
    int16_t center_y) {
  int x = scale_icon_x(bounds_size, center_x);
  int y = scale_icon_y(bounds_size, center_y);
  int r = scale_icon_coord(bounds_size, 3);

  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_line(ctx, GPoint(x - r, y), GPoint(x + r, y));
  graphics_draw_line(ctx, GPoint(x, y - r), GPoint(x, y + r));
  graphics_draw_line(ctx, GPoint(x - r, y - r),
                     GPoint(x + r, y + r));
  graphics_draw_line(ctx, GPoint(x - r, y + r),
                     GPoint(x + r, y - r));
}

static inline void draw_weather_clear_icon(
    GContext* ctx,
    const GSize* bounds_size) {
  draw_weather_sun(ctx, bounds_size, 14, 14, 6);
}

static inline void draw_weather_cloud_icon(
    GContext* ctx,
    const GSize* bounds_size) {
  draw_weather_cloud(ctx, bounds_size, 5, 14);
}

static inline void draw_weather_fog_icon(
    GContext* ctx,
    const GSize* bounds_size) {
  draw_weather_cloud(ctx, bounds_size, 5, 11);
  graphics_context_set_stroke_width(ctx, 2);
  draw_scaled_line(ctx, bounds_size, 5, 20, 23, 20);
  draw_scaled_line(ctx, bounds_size, 8, 24, 20, 24);
}

static inline void draw_weather_drizzle_icon(
    GContext* ctx,
    const GSize* bounds_size) {
  draw_weather_cloud(ctx, bounds_size, 5, 11);
  draw_weather_drizzle(ctx, bounds_size);
}

static inline void draw_weather_rain_icon(
    GContext* ctx,
    const GSize* bounds_size) {
  draw_weather_cloud(ctx, bounds_size, 5, 11);
  draw_weather_rain(ctx, bounds_size, true);
}

static inline void draw_weather_frozen_rain_icon(
    GContext* ctx,
    const GSize* bounds_size) {
  draw_weather_cloud(ctx, bounds_size, 5, 10);
  draw_weather_rain(ctx, bounds_size, false);
  draw_weather_snowflake(ctx, bounds_size, 21, 23);
}

static inline void draw_weather_snow_icon(
    GContext* ctx,
    const GSize* bounds_size) {
  draw_weather_cloud(ctx, bounds_size, 5, 10);
  draw_weather_snowflake(ctx, bounds_size, 9, 23);
  draw_weather_snowflake(ctx, bounds_size, 19, 23);
}

static inline void draw_weather_showers_icon(
    GContext* ctx,
    const GSize* bounds_size) {
  draw_weather_sun(ctx, bounds_size, 8, 8, 3);
  draw_weather_cloud(ctx, bounds_size, 6, 11);
  draw_weather_rain(ctx, bounds_size, true);
}

static inline void draw_weather_snow_showers_icon(
    GContext* ctx,
    const GSize* bounds_size) {
  draw_weather_sun(ctx, bounds_size, 8, 8, 3);
  draw_weather_cloud(ctx, bounds_size, 6, 10);
  draw_weather_snowflake(ctx, bounds_size, 10, 23);
  draw_weather_snowflake(ctx, bounds_size, 20, 23);
}

static inline void draw_weather_thunderstorm_icon(
    GContext* ctx,
    const GSize* bounds_size) {
  draw_weather_cloud(ctx, bounds_size, 5, 9);
  graphics_context_set_stroke_width(ctx, 2);
  draw_scaled_line(ctx, bounds_size, 15, 17, 11, 23);
  draw_scaled_line(ctx, bounds_size, 11, 23, 18, 22);
  draw_scaled_line(ctx, bounds_size, 18, 22, 14, 27);
}

static inline void draw_weather_unknown_icon(
    GContext* ctx,
    const GSize* bounds_size) {
  graphics_context_set_stroke_width(ctx, 2);
  draw_scaled_line(ctx, bounds_size, 11, 9, 14, 6);
  draw_scaled_line(ctx, bounds_size, 14, 6, 18, 8);
  draw_scaled_line(ctx, bounds_size, 18, 8, 18, 12);
  draw_scaled_line(ctx, bounds_size, 18, 12, 14, 16);
  draw_scaled_line(ctx, bounds_size, 14, 16, 14, 20);
  graphics_fill_circle(ctx,
                       get_scaled_point(bounds_size, 14, 24),
                       1);
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
  graphics_context_set_fill_color(ctx, s_weather_icon_background_color);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  if (!s_weather_is_available) {
    return;
  }

  graphics_context_set_fill_color(ctx, s_weather_icon_color);
  graphics_context_set_stroke_color(ctx, s_weather_icon_color);

  draw_weather_icon(
      ctx,
      &bounds.size,
      get_weather_icon_kind(s_weather_condition));
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
