#include "climate_glyphs.h"
#include "../c/ataglance.h"
#include "helper.h"
#include "substratum_renderer.h"

typedef enum {
  WEATHER_ICON_CLEAR = 0,
  WEATHER_ICON_SUNNY,
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

typedef enum {
  WEATHER_ICON_CENTER_X = ATAGLANCE_DESIGN_ICON_WIDTH / 2,
  WEATHER_ICON_CENTER_Y = ATAGLANCE_DESIGN_ICON_HEIGHT / 2,
} WeatherIconGeometry;

static int16_t weather_scale_x(const GRect* frame, int16_t value) {
  return frame->origin.x + helper_scale_round(
      value,
      frame->size.w,
      ATAGLANCE_DESIGN_ICON_WIDTH);
}

static int16_t weather_scale_y(const GRect* frame, int16_t value) {
  return frame->origin.y + helper_scale_round(
      value,
      frame->size.h,
      ATAGLANCE_DESIGN_ICON_HEIGHT
  );
}

static GPoint weather_point(const GRect* frame, int16_t x, int16_t y) {
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
      ATAGLANCE_DESIGN_ICON_WIDTH);
  out->size.h = helper_scale_round(
      h,
      frame->size.h,
      ATAGLANCE_DESIGN_ICON_HEIGHT);
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
                ATAGLANCE_DESIGN_ICON_WIDTH),
            helper_scale_round(
                h,
                frame->size.h,
                ATAGLANCE_DESIGN_ICON_HEIGHT)),
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
      substratum_renderer_scale_icon_coord(&frame->size, r));
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
      substratum_renderer_scale_icon_coord(&frame->size, r));
}

static GColor weather_subtle_color(
    const ColorPalette* palette) {
  if (!palette || helper_color_equal(palette->background, GColorWhite)) {
    return GColorDarkGray;
  }

  return GColorLightGray;
}

static GColor weather_clear_ring_color(
    const ColorPalette* palette) {
  return weather_subtle_color(palette);
}

static GColor weather_clear_fill_color(
    const ColorPalette* palette) {
  return gcolor_legible_over(palette->background);
}

static GColor weather_color_for_kind(
    WeatherIconKind kind,
    const ColorPalette* palette) {
  GColor fallback = gcolor_legible_over(palette->background);

  switch (kind) {
    case WEATHER_ICON_THUNDERSTORM:
    case WEATHER_ICON_SUNNY:
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

static void draw_weather_outline_sun(
    GContext* ctx,
    const GRect* frame,
    const ColorPalette* palette,
    bool is_outline) {
  GColor color = weather_color_for_kind(WEATHER_ICON_SUNNY, palette);
  GColor sunFillColor = is_outline ? palette->background : color;
  graphics_context_set_fill_color(ctx, sunFillColor);
  graphics_context_set_stroke_color(ctx, color);
  graphics_context_set_stroke_width(ctx, 2);
  weather_fill_circle(
      ctx,
      frame,
      WEATHER_ICON_CENTER_X,
      WEATHER_ICON_CENTER_Y,
      6);
  weather_line(
      ctx,
      frame,
      WEATHER_ICON_CENTER_X,
      WEATHER_ICON_CENTER_Y - 13,
      WEATHER_ICON_CENTER_X,
      WEATHER_ICON_CENTER_Y - 9);
  weather_line(
      ctx,
      frame,
      WEATHER_ICON_CENTER_X,
      WEATHER_ICON_CENTER_Y + 9,
      WEATHER_ICON_CENTER_X,
      WEATHER_ICON_CENTER_Y + 13);
  weather_line(
      ctx,
      frame,
      WEATHER_ICON_CENTER_X - 13,
      WEATHER_ICON_CENTER_Y,
      WEATHER_ICON_CENTER_X - 9,
      WEATHER_ICON_CENTER_Y);
  weather_line(
      ctx,
      frame,
      WEATHER_ICON_CENTER_X + 9,
      WEATHER_ICON_CENTER_Y,
      WEATHER_ICON_CENTER_X + 13,
      WEATHER_ICON_CENTER_Y);
  weather_line(
      ctx,
      frame,
      WEATHER_ICON_CENTER_X - 10,
      WEATHER_ICON_CENTER_Y - 10,
      WEATHER_ICON_CENTER_X - 7,
      WEATHER_ICON_CENTER_Y - 7);
  weather_line(
      ctx,
      frame,
      WEATHER_ICON_CENTER_X + 7,
      WEATHER_ICON_CENTER_Y - 7,
      WEATHER_ICON_CENTER_X + 10,
      WEATHER_ICON_CENTER_Y - 10);
  weather_line(
      ctx,
      frame,
      WEATHER_ICON_CENTER_X - 10,
      WEATHER_ICON_CENTER_Y + 10,
      WEATHER_ICON_CENTER_X - 7,
      WEATHER_ICON_CENTER_Y + 7);
  weather_line(
      ctx,
      frame,
      WEATHER_ICON_CENTER_X + 7,
      WEATHER_ICON_CENTER_Y + 7,
      WEATHER_ICON_CENTER_X + 10,
      WEATHER_ICON_CENTER_Y + 10);
}

static void draw_weather_sun(
    GContext* ctx,
    const GRect* frame,
    const ColorPalette* palette) {
      draw_weather_outline_sun(ctx, frame, palette, false);
}

static void draw_weather_filled_cloud(
    GContext* ctx,
    const GRect* frame,
    const ColorPalette* palette,
    WeatherIconKind icon_kind,
    bool is_filled) {
  GColor color = weather_color_for_kind(icon_kind, palette);
  GColor cloudFillColor = is_filled ? color : palette->background;

  graphics_context_set_stroke_color(ctx, color);
  graphics_context_set_fill_color(ctx, cloudFillColor);
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

static void draw_weather_cloud(
    GContext* ctx,
    const GRect* frame,
    const ColorPalette* palette,
    WeatherIconKind icon_kind) {
      draw_weather_filled_cloud(ctx, frame, palette, icon_kind, false);
}

static void draw_weather_clear_icon(
    GContext* ctx,
    const GRect* frame,
    const ColorPalette* palette) {
  graphics_context_set_stroke_color(ctx, weather_clear_ring_color(palette));
  graphics_context_set_stroke_width(ctx, 1);
  weather_draw_circle(
      ctx,
      frame,
      WEATHER_ICON_CENTER_X,
      WEATHER_ICON_CENTER_Y,
      11);

  graphics_context_set_fill_color(ctx, weather_clear_fill_color(palette));
  weather_fill_circle(
      ctx,
      frame,
      WEATHER_ICON_CENTER_X,
      WEATHER_ICON_CENTER_Y,
      9);
}

static void draw_weather_partly_cloudy_icon(
    GContext* ctx,
    const GRect* frame,
    const ColorPalette* palette) {
  GRect cloud_frame;
  GRect sun_frame;
  weather_subframe(frame, &sun_frame, 0, 0, 22, 22);
  draw_weather_outline_sun(ctx, &sun_frame, palette, true);

  weather_subframe(frame, &cloud_frame, 5, 10, 23, 17);
  draw_weather_filled_cloud(ctx, &cloud_frame, palette, WEATHER_ICON_PARTLY_CLOUDY, true);
}

static void draw_weather_fog_icon(
    GContext* ctx,
    const GRect* frame,
    const ColorPalette* palette) {
  graphics_context_set_stroke_color(ctx, weather_color_for_kind(
      WEATHER_ICON_FOG,
      palette));
  graphics_context_set_stroke_width(ctx, 2);
  weather_line(ctx, frame, 5, 10, 23, 10);
  weather_line(ctx, frame, 2, 15, 26, 15);
  weather_line(ctx, frame, 7, 20, 21, 20);
}

static void draw_weather_drizzle_icon(
    GContext* ctx,
    const GRect* frame,
    const ColorPalette* palette) {
  graphics_context_set_stroke_color(ctx, weather_color_for_kind(
      WEATHER_ICON_DRIZZLE,
      palette));
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
    bool heavy,
    const ColorPalette* palette) {
  draw_weather_rain_marks(
      ctx,
      frame,
      weather_color_for_kind(heavy ?
          WEATHER_ICON_HEAVY_RAIN : WEATHER_ICON_RAIN,
          palette),
      heavy);
}

static void draw_weather_snowflake(
    GContext* ctx,
    const GRect* frame,
    GColor color) {
  graphics_context_set_stroke_color(ctx, color);
  graphics_context_set_fill_color(ctx, color);
  graphics_context_set_stroke_width(ctx, 2);
  weather_line(
      ctx,
      frame,
      WEATHER_ICON_CENTER_X - 9,
      WEATHER_ICON_CENTER_Y,
      WEATHER_ICON_CENTER_X + 9,
      WEATHER_ICON_CENTER_Y);
  weather_line(
      ctx,
      frame,
      WEATHER_ICON_CENTER_X,
      WEATHER_ICON_CENTER_Y - 9,
      WEATHER_ICON_CENTER_X,
      WEATHER_ICON_CENTER_Y + 9);
  weather_line(
      ctx,
      frame,
      WEATHER_ICON_CENTER_X - 7,
      WEATHER_ICON_CENTER_Y - 7,
      WEATHER_ICON_CENTER_X + 7,
      WEATHER_ICON_CENTER_Y + 7);
  weather_line(
      ctx,
      frame,
      WEATHER_ICON_CENTER_X - 7,
      WEATHER_ICON_CENTER_Y + 7,
      WEATHER_ICON_CENTER_X + 7,
      WEATHER_ICON_CENTER_Y - 7);
  weather_fill_circle(
      ctx,
      frame,
      WEATHER_ICON_CENTER_X,
      WEATHER_ICON_CENTER_Y,
      2);
}

static void draw_weather_sleet_icon(
    GContext* ctx,
    const GRect* frame,
    const ColorPalette* palette) {
  GRect snow_frame;
  GColor color = weather_color_for_kind(WEATHER_ICON_SLEET, palette);

  graphics_context_set_stroke_color(ctx, color);
  graphics_context_set_stroke_width(ctx, 2);
  weather_line(ctx, frame, 7, 8, 4, 18);
  weather_line(ctx, frame, 12, 10, 9, 20);

  weather_subframe(frame, &snow_frame, 14, 6, 12, 18);
  draw_weather_snowflake(ctx, &snow_frame, color);
}

static void draw_weather_snow_showers_icon(
    GContext* ctx,
    const GRect* frame,
    const ColorPalette* palette) {
  GRect cloud_frame;
  GRect snow_frame;
  GColor color = weather_color_for_kind(
      WEATHER_ICON_SNOW_SHOWERS,
      palette);

  weather_subframe(frame, &cloud_frame, 0, 2, 20, 15);
  draw_weather_cloud(ctx, &cloud_frame, palette, WEATHER_ICON_SNOW_SHOWERS);

  weather_subframe(frame, &snow_frame, 10, 8, 18, 20);
  draw_weather_snowflake(ctx, &snow_frame, color);
}

static void draw_weather_showers_icon(
    GContext* ctx,
    const GRect* frame,
    bool heavy,
    const ColorPalette* palette) {
  GRect cloud_frame;
  GRect rain_frame;
  WeatherIconKind kind = heavy ? WEATHER_ICON_HEAVY_SHOWERS : WEATHER_ICON_SHOWERS;
  GColor color = weather_color_for_kind(kind, palette);

  weather_subframe(frame, &cloud_frame, 0, heavy ? 1 : 2, 20, 15);
  draw_weather_cloud(ctx, &cloud_frame, palette, kind);

  weather_subframe(frame, &rain_frame, 10, heavy ? 8 : 10, 18, 20);
  draw_weather_rain_marks(ctx, &rain_frame, color, heavy);
}

static void draw_weather_bolt_icon(
    GContext* ctx,
    const GRect* frame,
    const ColorPalette* palette) {
  GColor color = weather_color_for_kind(
      WEATHER_ICON_THUNDERSTORM,
      palette);
  GColor border_color = PBL_IF_COLOR_ELSE(
      weather_subtle_color(palette),
      gcolor_legible_over(palette->background));

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
    const GRect* frame,
    const ColorPalette* palette) {
  draw_weather_cloud(ctx, frame, palette, WEATHER_ICON_UNKNOWN);

  graphics_context_set_stroke_color(
      ctx,
      gcolor_legible_over(palette->background));
  graphics_context_set_stroke_width(ctx, 3);
  weather_line(ctx, frame, 5, 3, 24, 26);
}

static WeatherIconKind get_weather_icon_kind(int16_t weather_condition) {
  if (weather_condition < 0) {
    return WEATHER_ICON_UNKNOWN;
  }
  if (weather_condition == 0) {
    return WEATHER_ICON_CLEAR;
  }
  if (weather_condition == 1) {
    return WEATHER_ICON_SUNNY;
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

void draw_climate_icon(
    GContext* ctx,
    const GRect* frame,
    int16_t weather_condition,
    const ColorPalette* palette) {
  if (!ctx || !frame || !palette) {
    return;
  }

  WeatherIconKind icon_kind = get_weather_icon_kind(weather_condition);

  switch (icon_kind) {
    case WEATHER_ICON_CLEAR:
      draw_weather_clear_icon(ctx, frame, palette);
      break;
    case WEATHER_ICON_SUNNY:
      draw_weather_sun(ctx, frame, palette);
      break;
    case WEATHER_ICON_PARTLY_CLOUDY:
      draw_weather_partly_cloudy_icon(ctx, frame, palette);
      break;
    case WEATHER_ICON_CLOUD:
      draw_weather_cloud(ctx, frame, palette, icon_kind);
      break;
    case WEATHER_ICON_FOG:
      draw_weather_fog_icon(ctx, frame, palette);
      break;
    case WEATHER_ICON_DRIZZLE:
      draw_weather_drizzle_icon(ctx, frame, palette);
      break;
    case WEATHER_ICON_RAIN:
      draw_weather_rain_icon(ctx, frame, false, palette);
      break;
    case WEATHER_ICON_HEAVY_RAIN:
      draw_weather_rain_icon(ctx, frame, true, palette);
      break;
    case WEATHER_ICON_SLEET:
      draw_weather_sleet_icon(ctx, frame, palette);
      break;
    case WEATHER_ICON_SNOW:
      draw_weather_snowflake(
          ctx,
          frame,
          weather_color_for_kind(WEATHER_ICON_SNOW, palette));
      break;
    case WEATHER_ICON_SHOWERS:
      draw_weather_showers_icon(ctx, frame, false, palette);
      break;
    case WEATHER_ICON_HEAVY_SHOWERS:
      draw_weather_showers_icon(ctx, frame, true, palette);
      break;
    case WEATHER_ICON_SNOW_SHOWERS:
      draw_weather_snow_showers_icon(ctx, frame, palette);
      break;
    case WEATHER_ICON_THUNDERSTORM:
      draw_weather_bolt_icon(ctx, frame, palette);
      break;
    case WEATHER_ICON_UNKNOWN:
      draw_weather_unavailable_icon(ctx, frame, palette);
      break;
  }
}
