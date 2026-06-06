#include "weather.h"
#include "helper.h"
#include "layout.h"
#include "settings.h"

typedef enum {
  WEATHER_ICON_CLEAR = 0,
  WEATHER_ICON_PARTLY_CLOUDY,
  WEATHER_ICON_CLOUD,
  WEATHER_ICON_FOG,
  WEATHER_ICON_DRIZZLE,
  WEATHER_ICON_RAIN,
  WEATHER_ICON_HEAVY_RAIN,
  WEATHER_ICON_SLEET,
  WEATHER_ICON_SNOW,
  WEATHER_ICON_SHOWERS,
  WEATHER_ICON_HEAVY_SHOWERS,
  WEATHER_ICON_SNOW_SHOWERS,
  WEATHER_ICON_THUNDERSTORM,
  WEATHER_ICON_UNKNOWN,
} WeatherIconKind;

#define WEATHER_TEMP_MIN_CELSIUS_TENTHS -1600
#define WEATHER_TEMP_MAX_CELSIUS_TENTHS 1000
#define WEATHER_CONDITION_MIN 0
#define WEATHER_CONDITION_MAX 99

static Layer* s_weather_icon_layer;
static TextLayer* s_temperature_layer;
static char s_temperature_buffer[ATAGLANCE_MAX_STR_LEN];
static int16_t s_temp_celsius_tenths;
static int16_t s_weather_condition;
static const VisualPalette* s_weather_palette;
static bool s_weather_is_available;

static bool format_temperature(
    char* buf,
    size_t buflen,
    uint8_t temp_unit);
static inline bool weather_temperature_is_valid(int celsius_tenths);
static inline bool weather_condition_is_valid(int weather_condition);
static bool weather_condition_is_known(void);
static void weather_module_update_display(uint8_t temp_unit);

static bool format_temperature(
    char* buf,
    size_t buflen,
    uint8_t temp_unit) {
  if (!buf || buflen == 0) {
    return false;
  }

  if (s_temp_celsius_tenths == WEATHER_TEMP_INVALID) {
    const char* unit_text =
        temp_unit == TEMP_UNIT_C ? "°C" : "°F";
    snprintf(
        buf,
        buflen,
        "%s%s",
        DISPLAY_UNAVAILABLE_TEXT,
        unit_text);
    return false;
  }

  if (temp_unit == TEMP_UNIT_F) {
    int f_whole = ((s_temp_celsius_tenths * 9 + 25) / 50) + 32;
    snprintf(buf, buflen, "%d°F", f_whole);
  } else {
    int c_whole = (s_temp_celsius_tenths >= 0) ?
        (s_temp_celsius_tenths + 5) / 10 :
        (s_temp_celsius_tenths - 5) / 10;
    snprintf(buf, buflen, "%d°C", c_whole);
  }

  return true;
}

static inline bool weather_temperature_is_valid(int celsius_tenths) {
  return celsius_tenths == WEATHER_TEMP_INVALID ||
      (
          celsius_tenths >= WEATHER_TEMP_MIN_CELSIUS_TENTHS &&
          celsius_tenths <= WEATHER_TEMP_MAX_CELSIUS_TENTHS);
}

static inline bool weather_condition_is_valid(int weather_condition) {
  return weather_condition == WEATHER_CONDITION_UNKNOWN ||
      (
          weather_condition >= WEATHER_CONDITION_MIN &&
          weather_condition <= WEATHER_CONDITION_MAX);
}

static void weather_module_update_display(uint8_t temp_unit) {
  if (!s_temperature_layer || !s_weather_palette) {
    return;
  }

  bool is_temperature_available = format_temperature(
      s_temperature_buffer,
      ATAGLANCE_MAX_STR_LEN,
      temp_unit);
  GColor text_color = is_temperature_available ?
      s_weather_palette->primary_text :
      s_weather_palette->unavailable_text;

  display_update_text_layer(
      s_temperature_layer,
      s_temperature_buffer,
      text_color);

  s_weather_is_available =
      is_temperature_available &&
      weather_condition_is_known();

  if (s_weather_icon_layer) {
    layer_mark_dirty(s_weather_icon_layer);
  }
}

static int16_t weather_scale_x(
    const GRect* frame,
    int16_t value) {
  return frame->origin.x + helper_scale_round(
      value,
      frame->size.w,
      ATAGLANCE_DESIGN_ICON_SIZE);
}

static int16_t weather_scale_y(
    const GRect* frame,
    int16_t value) {
  return frame->origin.y + helper_scale_round(
      value,
      frame->size.h,
      ATAGLANCE_DESIGN_ICON_SIZE);
}

static int16_t weather_scale_coord(
    const GRect* frame,
    int16_t value) {
  int16_t draw_size = helper_min(frame->size.w, frame->size.h);
  return helper_scale_round(
      value,
      draw_size,
      ATAGLANCE_DESIGN_ICON_SIZE);
}

static GPoint weather_point(
    const GRect* frame,
    int16_t x,
    int16_t y) {
  return GPoint(weather_scale_x(frame, x), weather_scale_y(frame, y));
}

static void weather_subframe(
    const GRect* frame,
    GRect* out,
    int16_t x,
    int16_t y,
    int16_t w,
    int16_t h) {
  out->origin.x = weather_scale_x(frame, x);
  out->origin.y = weather_scale_y(frame, y);
  out->size.w = helper_scale_round(
      w,
      frame->size.w,
      ATAGLANCE_DESIGN_ICON_SIZE);
  out->size.h = helper_scale_round(
      h,
      frame->size.h,
      ATAGLANCE_DESIGN_ICON_SIZE);
}

static void weather_line(
    GContext* ctx,
    const GRect* frame,
    int16_t x0,
    int16_t y0,
    int16_t x1,
    int16_t y1) {
  graphics_draw_line(
      ctx,
      weather_point(frame, x0, y0),
      weather_point(frame, x1, y1));
}

static void weather_fill_rect(
    GContext* ctx,
    const GRect* frame,
    int16_t x,
    int16_t y,
    int16_t w,
    int16_t h) {
  graphics_fill_rect(
      ctx,
      GRect(weather_scale_x(frame, x),
            weather_scale_y(frame, y),
            helper_scale_round(
                w,
                frame->size.w,
                ATAGLANCE_DESIGN_ICON_SIZE),
            helper_scale_round(
                h,
                frame->size.h,
                ATAGLANCE_DESIGN_ICON_SIZE)),
      0,
      GCornerNone);
}

static void weather_fill_circle(
    GContext* ctx,
    const GRect* frame,
    int16_t x,
    int16_t y,
    int16_t r) {
  graphics_fill_circle(
      ctx,
      weather_point(frame, x, y),
      weather_scale_coord(frame, r));
}

static void weather_draw_circle(
    GContext* ctx,
    const GRect* frame,
    int16_t x,
    int16_t y,
    int16_t r) {
  graphics_draw_circle(
      ctx,
      weather_point(frame, x, y),
      weather_scale_coord(frame, r));
}

static GColor weather_legible_color(void) {
  return display_legible_over_background(s_weather_palette);
}

static GColor weather_subtle_color(void) {
  if (!s_weather_palette ||
      gcolor_equal(s_weather_palette->background, GColorWhite)) {
    return GColorDarkGray;
  }

  return GColorLightGray;
}

static GColor weather_clear_ring_color(void) {
  return weather_subtle_color();
}

static GColor weather_clear_fill_color(void) {
  return weather_legible_color();
}

static GColor weather_color_for_kind(WeatherIconKind kind) {
  GColor fallback = weather_legible_color();

  switch (kind) {
    case WEATHER_ICON_THUNDERSTORM:
      return PBL_IF_COLOR_ELSE(GColorChromeYellow, fallback);
    case WEATHER_ICON_SNOW:
    case WEATHER_ICON_SNOW_SHOWERS:
    case WEATHER_ICON_SLEET:
      return PBL_IF_COLOR_ELSE(GColorPictonBlue, fallback);
    case WEATHER_ICON_DRIZZLE:
    case WEATHER_ICON_RAIN:
    case WEATHER_ICON_HEAVY_RAIN:
    case WEATHER_ICON_SHOWERS:
    case WEATHER_ICON_HEAVY_SHOWERS:
      return PBL_IF_COLOR_ELSE(GColorVividCerulean, fallback);
    case WEATHER_ICON_CLOUD:
    case WEATHER_ICON_PARTLY_CLOUDY:
    case WEATHER_ICON_FOG:
    case WEATHER_ICON_CLEAR:
    case WEATHER_ICON_UNKNOWN:
      return PBL_IF_COLOR_ELSE(GColorCobaltBlue, fallback);
  }

  return fallback;
}

static void draw_weather_sun(
    GContext* ctx,
    const GRect* frame,
    GColor color) {
  graphics_context_set_fill_color(ctx, color);
  graphics_context_set_stroke_color(ctx, color);
  graphics_context_set_stroke_width(ctx, 2);
  weather_fill_circle(ctx, frame, 10, 10, 5);
  weather_line(ctx, frame, 10, 0, 10, 3);
  weather_line(ctx, frame, 10, 17, 10, 20);
  weather_line(ctx, frame, 0, 10, 3, 10);
  weather_line(ctx, frame, 17, 10, 20, 10);
  weather_line(ctx, frame, 3, 3, 6, 6);
  weather_line(ctx, frame, 14, 6, 17, 3);
  weather_line(ctx, frame, 3, 17, 6, 14);
  weather_line(ctx, frame, 14, 14, 17, 17);
}

static void draw_weather_cloud(
    GContext* ctx,
    const GRect* frame,
    GColor color) {
  graphics_context_set_stroke_color(ctx, color);
  graphics_context_set_fill_color(ctx, s_weather_palette->background);
  graphics_context_set_stroke_width(ctx, 2);

  weather_draw_circle(ctx, frame, 8, 16, 5);
  weather_draw_circle(ctx, frame, 14, 11, 6);
  weather_draw_circle(ctx, frame, 19, 15, 5);

  weather_fill_circle(ctx, frame, 8, 16, 4);
  weather_fill_circle(ctx, frame, 14, 11, 5);
  weather_fill_circle(ctx, frame, 19, 15, 4);
  weather_fill_rect(ctx, frame, 6, 13, 15, 7);

  weather_line(ctx, frame, 4, 20, 23, 20);
}

static void draw_weather_clear_icon(
    GContext* ctx,
    const GRect* frame) {
  graphics_context_set_stroke_color(ctx, weather_clear_ring_color());
  graphics_context_set_stroke_width(ctx, 1);
  weather_draw_circle(ctx, frame, 14, 14, 11);

  graphics_context_set_fill_color(ctx, weather_clear_fill_color());
  weather_fill_circle(ctx, frame, 14, 14, 9);
}

static void draw_weather_partly_cloudy_icon(
    GContext* ctx,
    const GRect* frame) {
  GRect cloud_frame;
  GRect sun_frame;
  GColor color = weather_color_for_kind(WEATHER_ICON_PARTLY_CLOUDY);

  weather_subframe(frame, &sun_frame, 0, 0, 22, 22);
  draw_weather_sun(ctx, &sun_frame, weather_legible_color());

  weather_subframe(frame, &cloud_frame, 5, 10, 23, 17);
  draw_weather_cloud(ctx, &cloud_frame, color);
}

static void draw_weather_fog_icon(
    GContext* ctx,
    const GRect* frame) {
  graphics_context_set_stroke_color(ctx, weather_color_for_kind(
      WEATHER_ICON_FOG));
  graphics_context_set_stroke_width(ctx, 2);
  weather_line(ctx, frame, 5, 10, 23, 10);
  weather_line(ctx, frame, 2, 15, 26, 15);
  weather_line(ctx, frame, 7, 20, 21, 20);
}

static void draw_weather_drizzle_icon(
    GContext* ctx,
    const GRect* frame) {
  graphics_context_set_stroke_color(ctx, weather_color_for_kind(
      WEATHER_ICON_DRIZZLE));
  graphics_context_set_stroke_width(ctx, 1);
  weather_line(ctx, frame, 7, 7, 5, 14);
  weather_line(ctx, frame, 14, 6, 12, 13);
  weather_line(ctx, frame, 21, 7, 19, 14);
}

static void draw_weather_rain_marks(
    GContext* ctx,
    const GRect* frame,
    GColor color,
    bool heavy) {
  graphics_context_set_stroke_color(ctx, color);
  graphics_context_set_stroke_width(ctx, 2);

  if (heavy) {
    weather_line(ctx, frame, 6, 3, 2, 25);
    weather_line(ctx, frame, 12, 2, 8, 24);
    weather_line(ctx, frame, 18, 4, 14, 26);
    weather_line(ctx, frame, 24, 2, 20, 24);
    return;
  }

  weather_line(ctx, frame, 8, 7, 5, 21);
  weather_line(ctx, frame, 15, 7, 12, 21);
  weather_line(ctx, frame, 22, 7, 19, 21);
}

static void draw_weather_rain_icon(
    GContext* ctx,
    const GRect* frame,
    bool heavy) {
  draw_weather_rain_marks(
      ctx,
      frame,
      weather_color_for_kind(heavy ?
          WEATHER_ICON_HEAVY_RAIN : WEATHER_ICON_RAIN),
      heavy);
}

static void draw_weather_snowflake(
    GContext* ctx,
    const GRect* frame,
    GColor color) {
  graphics_context_set_stroke_color(ctx, color);
  graphics_context_set_fill_color(ctx, color);
  graphics_context_set_stroke_width(ctx, 2);
  weather_line(ctx, frame, 5, 14, 23, 14);
  weather_line(ctx, frame, 14, 5, 14, 23);
  weather_line(ctx, frame, 7, 7, 21, 21);
  weather_line(ctx, frame, 7, 21, 21, 7);
  weather_fill_circle(ctx, frame, 14, 14, 2);
}

static void draw_weather_sleet_icon(
    GContext* ctx,
    const GRect* frame) {
  GRect snow_frame;
  GColor color = weather_color_for_kind(WEATHER_ICON_SLEET);

  graphics_context_set_stroke_color(ctx, color);
  graphics_context_set_stroke_width(ctx, 2);
  weather_line(ctx, frame, 7, 8, 4, 18);
  weather_line(ctx, frame, 12, 10, 9, 20);

  weather_subframe(frame, &snow_frame, 14, 6, 12, 18);
  draw_weather_snowflake(ctx, &snow_frame, color);
}

static void draw_weather_snow_showers_icon(
    GContext* ctx,
    const GRect* frame) {
  GRect cloud_frame;
  GRect snow_frame;
  GColor color = weather_color_for_kind(WEATHER_ICON_SNOW_SHOWERS);

  weather_subframe(frame, &cloud_frame, 0, 2, 20, 15);
  draw_weather_cloud(ctx, &cloud_frame, color);

  weather_subframe(frame, &snow_frame, 10, 8, 18, 20);
  draw_weather_snowflake(ctx, &snow_frame, color);
}

static void draw_weather_showers_icon(
    GContext* ctx,
    const GRect* frame,
    bool heavy) {
  GRect cloud_frame;
  GRect rain_frame;
  GColor color = weather_color_for_kind(heavy ?
      WEATHER_ICON_HEAVY_SHOWERS : WEATHER_ICON_SHOWERS);

  weather_subframe(frame, &cloud_frame, 0, heavy ? 1 : 2, 20, 15);
  draw_weather_cloud(ctx, &cloud_frame, color);

  weather_subframe(frame, &rain_frame, 10, heavy ? 8 : 10, 18, 20);
  draw_weather_rain_marks(ctx, &rain_frame, color, heavy);
}

static void draw_weather_bolt_icon(
    GContext* ctx,
    const GRect* frame) {
  GColor color = weather_color_for_kind(WEATHER_ICON_THUNDERSTORM);
  GColor border_color = PBL_IF_COLOR_ELSE(
      weather_subtle_color(),
      weather_legible_color());

  graphics_context_set_stroke_color(ctx, border_color);
  graphics_context_set_stroke_width(ctx, 1);
  weather_line(ctx, frame, 17, 4, 9, 15);
  weather_line(ctx, frame, 9, 15, 15, 15);
  weather_line(ctx, frame, 15, 15, 10, 25);
  weather_line(ctx, frame, 10, 25, 22, 11);
  weather_line(ctx, frame, 22, 11, 16, 11);
  weather_line(ctx, frame, 16, 11, 17, 4);

  graphics_context_set_stroke_color(ctx, color);
  graphics_context_set_stroke_width(ctx, 2);
  weather_line(ctx, frame, 17, 4, 9, 15);
  weather_line(ctx, frame, 9, 15, 15, 15);
  weather_line(ctx, frame, 15, 15, 10, 25);
  weather_line(ctx, frame, 10, 25, 22, 11);
  weather_line(ctx, frame, 22, 11, 16, 11);
  weather_line(ctx, frame, 16, 11, 17, 4);
}

static void draw_weather_unavailable_icon(
    GContext* ctx,
    const GRect* frame) {
  draw_weather_cloud(ctx, frame, s_weather_palette->unavailable_text);

  graphics_context_set_stroke_color(ctx, weather_legible_color());
  graphics_context_set_stroke_width(ctx, 3);
  weather_line(ctx, frame, 5, 3, 24, 26);
}

static bool weather_condition_is_known(void) {
  return s_weather_condition != WEATHER_CONDITION_UNKNOWN;
}

static WeatherIconKind get_weather_icon_kind(
    int16_t weather_condition) {
  if (weather_condition < 0) {
    return WEATHER_ICON_UNKNOWN;
  }
  if (weather_condition <= 1) {
    return WEATHER_ICON_CLEAR;
  }
  if (weather_condition == 2) {
    return WEATHER_ICON_PARTLY_CLOUDY;
  }
  if (weather_condition == 3) {
    return WEATHER_ICON_CLOUD;
  }
  if (weather_condition <= 48) {
    return WEATHER_ICON_FOG;
  }
  if (weather_condition <= 55) {
    return WEATHER_ICON_DRIZZLE;
  }
  if (weather_condition <= 57) {
    return WEATHER_ICON_SLEET;
  }
  if (weather_condition <= 63) {
    return WEATHER_ICON_RAIN;
  }
  if (weather_condition <= 65) {
    return WEATHER_ICON_HEAVY_RAIN;
  }
  if (weather_condition <= 67) {
    return WEATHER_ICON_SLEET;
  }
  if (weather_condition <= 77) {
    return WEATHER_ICON_SNOW;
  }
  if (weather_condition <= 81) {
    return WEATHER_ICON_SHOWERS;
  }
  if (weather_condition <= 82) {
    return WEATHER_ICON_HEAVY_SHOWERS;
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
    const GRect* frame,
    WeatherIconKind icon_kind) {
  switch (icon_kind) {
    case WEATHER_ICON_CLEAR:
      draw_weather_clear_icon(ctx, frame);
      break;
    case WEATHER_ICON_PARTLY_CLOUDY:
      draw_weather_partly_cloudy_icon(ctx, frame);
      break;
    case WEATHER_ICON_CLOUD:
      draw_weather_cloud(
          ctx,
          frame,
          weather_color_for_kind(WEATHER_ICON_CLOUD));
      break;
    case WEATHER_ICON_FOG:
      draw_weather_fog_icon(ctx, frame);
      break;
    case WEATHER_ICON_DRIZZLE:
      draw_weather_drizzle_icon(ctx, frame);
      break;
    case WEATHER_ICON_RAIN:
      draw_weather_rain_icon(ctx, frame, false);
      break;
    case WEATHER_ICON_HEAVY_RAIN:
      draw_weather_rain_icon(ctx, frame, true);
      break;
    case WEATHER_ICON_SLEET:
      draw_weather_sleet_icon(ctx, frame);
      break;
    case WEATHER_ICON_SNOW:
      draw_weather_snowflake(
          ctx,
          frame,
          weather_color_for_kind(WEATHER_ICON_SNOW));
      break;
    case WEATHER_ICON_SHOWERS:
      draw_weather_showers_icon(ctx, frame, false);
      break;
    case WEATHER_ICON_HEAVY_SHOWERS:
      draw_weather_showers_icon(ctx, frame, true);
      break;
    case WEATHER_ICON_SNOW_SHOWERS:
      draw_weather_snow_showers_icon(ctx, frame);
      break;
    case WEATHER_ICON_THUNDERSTORM:
      draw_weather_bolt_icon(ctx, frame);
      break;
    case WEATHER_ICON_UNKNOWN:
      draw_weather_unavailable_icon(ctx, frame);
      break;
  }
}

static void weather_icon_update_proc(Layer* layer, GContext* ctx) {
  if (!layer || !ctx || !s_weather_palette) {
    return;
  }

  GRect bounds = layer_get_bounds(layer);

  graphics_context_set_fill_color(ctx, s_weather_palette->background);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  if (!s_weather_is_available) {
    draw_weather_unavailable_icon(ctx, &bounds);
    return;
  }

  draw_weather_icon(
      ctx,
      &bounds,
      get_weather_icon_kind(s_weather_condition));
}

void weather_module_init(void) {
  s_weather_icon_layer = NULL;
  s_temperature_layer = NULL;
  s_temperature_buffer[0] = '\0';
  s_temp_celsius_tenths = WEATHER_TEMP_INVALID;
  s_weather_condition = WEATHER_CONDITION_UNKNOWN;
  s_weather_palette = NULL;
  s_weather_is_available = false;
}

bool weather_module_create(
    Layer* root,
    const WatchfaceLayout* layout,
    uint8_t temp_unit,
    const VisualPalette* palette) {
  if (!root || !layout || !palette) {
    return false;
  }

  s_weather_palette = palette;
  s_temperature_layer = text_layer_create(layout->temp_text_frame);
  if (!s_temperature_layer) {
    APP_LOG(APP_LOG_LEVEL_ERROR,
            "Failed to create weather temperature layer");
    return false;
  }

  text_layer_set_background_color(s_temperature_layer, GColorClear);
  text_layer_set_font(
      s_temperature_layer,
      fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(
      s_temperature_layer,
      GTextAlignmentLeft);
  layer_add_child(root, text_layer_get_layer(s_temperature_layer));

  s_weather_icon_layer = layer_create(layout->weather_icon_frame);
  if (!s_weather_icon_layer) {
    weather_module_update_display(temp_unit);
    return true;
  }

  layer_set_update_proc(
      s_weather_icon_layer,
      weather_icon_update_proc);
  layer_add_child(root, s_weather_icon_layer);
  weather_module_update_display(temp_unit);
  return true;
}

void weather_module_destroy(void) {
  if (s_weather_icon_layer) {
    layer_destroy(s_weather_icon_layer);
    s_weather_icon_layer = NULL;
  }
  if (s_temperature_layer) {
    text_layer_destroy(s_temperature_layer);
    s_temperature_layer = NULL;
  }

  s_temperature_buffer[0] = '\0';
}

void weather_module_refresh(
    uint8_t temp_unit,
    const VisualPalette* palette) {
  if (palette) {
    s_weather_palette = palette;
  }

  weather_module_update_display(temp_unit);
}

void weather_module_set_temperature(
    int celsius_tenths,
    uint8_t temp_unit,
    const VisualPalette* palette) {
  if (!weather_temperature_is_valid(celsius_tenths)) {
    APP_LOG(APP_LOG_LEVEL_WARNING,
            "Weather temperature invalid: value=%d",
            celsius_tenths);
    celsius_tenths = WEATHER_TEMP_INVALID;
  }

  s_temp_celsius_tenths = celsius_tenths;
  weather_module_refresh(temp_unit, palette);
}

void weather_module_set_condition(
    int weather_condition,
    uint8_t temp_unit,
    const VisualPalette* palette) {
  if (!weather_condition_is_valid(weather_condition)) {
    APP_LOG(APP_LOG_LEVEL_WARNING,
            "Weather condition invalid: value=%d",
            weather_condition);
    weather_condition = WEATHER_CONDITION_UNKNOWN;
  }

  s_weather_condition = weather_condition;
  weather_module_refresh(temp_unit, palette);
}
