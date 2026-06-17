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
  WEATHER_ICON_GRID_W = 28,
  WEATHER_ICON_GRID_H = 28,
  WEATHER_ICON_CENTER_X = WEATHER_ICON_GRID_W / 2,
  WEATHER_ICON_CENTER_Y = WEATHER_ICON_GRID_H / 2,
} WeatherIconGeometry;

static int16_t weather_scale_x(const GRect* frame, int16_t value) {
  return frame->origin.x + HELPER_SCALE_ROUND(
      value,
      frame->size.w,
      WEATHER_ICON_GRID_W);
}

static int16_t weather_scale_y(const GRect* frame, int16_t value) {
  return frame->origin.y + HELPER_SCALE_ROUND(
      value,
      frame->size.h,
      WEATHER_ICON_GRID_H
  );
}

// TODO: Audit every weather_subframe() call site. Nested frame composition can
// hide clipping and overlap errors when a child frame is tuned by eye instead
// of derived from a stable glyph contract, as in partly-cloudy cloud/sun
// placement.
static void weather_subframe(
    const GRect* frame,
    GRect* out,
    int16_t x,
    int16_t y,
    int16_t w,
    int16_t h) {
  out->origin.x = weather_scale_x(frame, x);
  out->origin.y = weather_scale_y(frame, y);
  out->size.w = HELPER_SCALE_ROUND(
      w,
      frame->size.w,
      WEATHER_ICON_GRID_W);
  out->size.h = HELPER_SCALE_ROUND(
      h,
      frame->size.h,
      WEATHER_ICON_GRID_H);
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
  GColor fallback = palette->primary_text;

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
      return PBL_IF_COLOR_ELSE(GColorCobaltBlue, fallback);
    case WEATHER_ICON_CLOUD:
    case WEATHER_ICON_PARTLY_CLOUDY:
    case WEATHER_ICON_FOG:
    case WEATHER_ICON_CLEAR:
      return PBL_IF_COLOR_ELSE(GColorCobaltBlue, fallback);
    case WEATHER_ICON_UNKNOWN:
      return palette->unavailable_text;
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
  substratum_renderer_fill_scaled_circle_in_frame(
      ctx,
      frame,
      WEATHER_ICON_CENTER_X,
      WEATHER_ICON_CENTER_Y,
      8);
  substratum_renderer_draw_scaled_line_in_frame(
      ctx,
      frame,
      WEATHER_ICON_CENTER_X,
      WEATHER_ICON_CENTER_Y - 13,
      WEATHER_ICON_CENTER_X,
      WEATHER_ICON_CENTER_Y - 10);
  substratum_renderer_draw_scaled_line_in_frame(
      ctx,
      frame,
      WEATHER_ICON_CENTER_X,
      WEATHER_ICON_CENTER_Y + 10,
      WEATHER_ICON_CENTER_X,
      WEATHER_ICON_CENTER_Y + 13);
  substratum_renderer_draw_scaled_line_in_frame(
      ctx,
      frame,
      WEATHER_ICON_CENTER_X - 13,
      WEATHER_ICON_CENTER_Y,
      WEATHER_ICON_CENTER_X - 10,
      WEATHER_ICON_CENTER_Y);
  substratum_renderer_draw_scaled_line_in_frame(
      ctx,
      frame,
      WEATHER_ICON_CENTER_X + 10,
      WEATHER_ICON_CENTER_Y,
      WEATHER_ICON_CENTER_X + 13,
      WEATHER_ICON_CENTER_Y);
  substratum_renderer_draw_scaled_line_in_frame(
      ctx,
      frame,
      WEATHER_ICON_CENTER_X - 11,
      WEATHER_ICON_CENTER_Y - 11,
      WEATHER_ICON_CENTER_X - 9,
      WEATHER_ICON_CENTER_Y - 9);
  substratum_renderer_draw_scaled_line_in_frame(
      ctx,
      frame,
      WEATHER_ICON_CENTER_X + 9,
      WEATHER_ICON_CENTER_Y - 9,
      WEATHER_ICON_CENTER_X + 11,
      WEATHER_ICON_CENTER_Y - 11);
  substratum_renderer_draw_scaled_line_in_frame(
      ctx,
      frame,
      WEATHER_ICON_CENTER_X - 11,
      WEATHER_ICON_CENTER_Y + 11,
      WEATHER_ICON_CENTER_X - 9,
      WEATHER_ICON_CENTER_Y + 9);
  substratum_renderer_draw_scaled_line_in_frame(
      ctx,
      frame,
      WEATHER_ICON_CENTER_X + 9,
      WEATHER_ICON_CENTER_Y + 9,
      WEATHER_ICON_CENTER_X + 11,
      WEATHER_ICON_CENTER_Y + 11);
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
  // Cloud contract: three lobes plus a shared body fill, tuned for compact
  // displays so the silhouette reads as one cloud instead of three circles
  // with a heavy lower-right bias after scaling.
  const int16_t left_cloud_center_x = 8;
  const int16_t left_cloud_center_y = 14;
  const int16_t left_cloud_radius = 4;
  const int16_t center_cloud_center_x = 14;
  const int16_t center_cloud_center_y = 10;
  const int16_t center_cloud_radius = 5;
  const int16_t right_cloud_center_x = 19;
  const int16_t right_cloud_center_y = 14;
  const int16_t right_cloud_radius = 4;
  const int16_t cloud_fill_left = 6;
  const int16_t cloud_fill_top = 12;
  const int16_t cloud_fill_right = 21;
  const int16_t cloud_fill_bottom = 18;
  const int16_t cloud_base_left = 4;
  const int16_t cloud_base_right = 23;
  const int16_t cloud_base_y = 18;
  GColor color = weather_color_for_kind(icon_kind, palette);
  GColor cloudFillColor = is_filled ? color : palette->background;

  graphics_context_set_stroke_color(ctx, color);
  graphics_context_set_fill_color(ctx, cloudFillColor);
  graphics_context_set_stroke_width(ctx, 2);

  substratum_renderer_draw_scaled_circle_in_frame(
      ctx,
      frame,
      left_cloud_center_x,
      left_cloud_center_y,
      left_cloud_radius);
  substratum_renderer_draw_scaled_circle_in_frame(
      ctx,
      frame,
      center_cloud_center_x,
      center_cloud_center_y,
      center_cloud_radius);
  substratum_renderer_draw_scaled_circle_in_frame(
      ctx,
      frame,
      right_cloud_center_x,
      right_cloud_center_y,
      right_cloud_radius);

  substratum_renderer_fill_scaled_circle_in_frame(
      ctx,
      frame,
      left_cloud_center_x,
      left_cloud_center_y,
      3);
  substratum_renderer_fill_scaled_circle_in_frame(
      ctx,
      frame,
      center_cloud_center_x,
      center_cloud_center_y,
      4);
  substratum_renderer_fill_scaled_circle_in_frame(
      ctx,
      frame,
      right_cloud_center_x,
      right_cloud_center_y,
      3);
  substratum_renderer_fill_scaled_rect_from_corners_in_frame(
      ctx,
      frame,
      cloud_fill_left,
      cloud_fill_top,
      cloud_fill_right,
      cloud_fill_bottom);

  substratum_renderer_draw_scaled_line_in_frame(
      ctx,
      frame,
      cloud_base_left,
      cloud_base_y,
      cloud_base_right,
      cloud_base_y);
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
  substratum_renderer_draw_scaled_circle_in_frame(
      ctx,
      frame,
      WEATHER_ICON_CENTER_X,
      WEATHER_ICON_CENTER_Y,
      12);

  graphics_context_set_fill_color(ctx, weather_clear_fill_color(palette));
  substratum_renderer_fill_scaled_circle_in_frame(
      ctx,
      frame,
      WEATHER_ICON_CENTER_X,
      WEATHER_ICON_CENTER_Y,
      10);
}

static void draw_weather_partly_cloudy_icon(
    GContext* ctx,
    const GRect* frame,
    const ColorPalette* palette) {
  GRect cloud_frame;
  GRect clear_frame;
  weather_subframe(frame, &clear_frame, 0, 0, 22, 22);
  draw_weather_clear_icon(ctx, &clear_frame, palette);

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
  substratum_renderer_draw_scaled_line_in_frame(ctx, frame, 5, 10, 23, 10);
  substratum_renderer_draw_scaled_line_in_frame(ctx, frame, 2, 15, 26, 15);
  substratum_renderer_draw_scaled_line_in_frame(ctx, frame, 7, 20, 21, 20);
}

static void draw_weather_drizzle_icon(
    GContext* ctx,
    const GRect* frame,
    const ColorPalette* palette) {
  // Drizzle contract: three heavier, staggered marks so the glyph does not
  // collapse into dots on compact monochrome displays.
  graphics_context_set_stroke_color(ctx, weather_color_for_kind(
      WEATHER_ICON_DRIZZLE,
      palette));
  graphics_context_set_stroke_width(ctx, 2);
  substratum_renderer_draw_scaled_line_in_frame(ctx, frame, 7, 5, 4, 16);
  substratum_renderer_draw_scaled_line_in_frame(ctx, frame, 14, 10, 11, 21);
  substratum_renderer_draw_scaled_line_in_frame(ctx, frame, 21, 15, 18, 26);
}

static void draw_weather_rain_marks(
    GContext* ctx,
    const GRect* frame,
    GColor color,
    bool heavy) {
  graphics_context_set_stroke_color(ctx, color);
  graphics_context_set_stroke_width(ctx, 2);

  if (heavy) {
    substratum_renderer_draw_scaled_line_in_frame(ctx, frame, 6, 3, 2, 25);
    substratum_renderer_draw_scaled_line_in_frame(ctx, frame, 12, 2, 8, 24);
    substratum_renderer_draw_scaled_line_in_frame(ctx, frame, 18, 4, 14, 26);
    substratum_renderer_draw_scaled_line_in_frame(ctx, frame, 24, 2, 20, 24);
    return;
  }

  substratum_renderer_draw_scaled_line_in_frame(ctx, frame, 8, 7, 5, 21);
  substratum_renderer_draw_scaled_line_in_frame(ctx, frame, 15, 7, 12, 21);
  substratum_renderer_draw_scaled_line_in_frame(ctx, frame, 22, 7, 19, 21);
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
  substratum_renderer_draw_scaled_line_in_frame(
      ctx,
      frame,
      WEATHER_ICON_CENTER_X - 9,
      WEATHER_ICON_CENTER_Y,
      WEATHER_ICON_CENTER_X + 9,
      WEATHER_ICON_CENTER_Y);
  substratum_renderer_draw_scaled_line_in_frame(
      ctx,
      frame,
      WEATHER_ICON_CENTER_X,
      WEATHER_ICON_CENTER_Y - 9,
      WEATHER_ICON_CENTER_X,
      WEATHER_ICON_CENTER_Y + 9);
  substratum_renderer_draw_scaled_line_in_frame(
      ctx,
      frame,
      WEATHER_ICON_CENTER_X - 7,
      WEATHER_ICON_CENTER_Y - 7,
      WEATHER_ICON_CENTER_X + 7,
      WEATHER_ICON_CENTER_Y + 7);
  substratum_renderer_draw_scaled_line_in_frame(
      ctx,
      frame,
      WEATHER_ICON_CENTER_X - 7,
      WEATHER_ICON_CENTER_Y + 7,
      WEATHER_ICON_CENTER_X + 7,
      WEATHER_ICON_CENTER_Y - 7);
  substratum_renderer_fill_scaled_circle_in_frame(
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
  substratum_renderer_draw_scaled_line_in_frame(ctx, frame, 7, 8, 4, 18);
  substratum_renderer_draw_scaled_line_in_frame(ctx, frame, 12, 10, 9, 20);

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

static void draw_weather_unavailable_icon(
    GContext* ctx,
    const GRect* frame,
    const ColorPalette* palette) {
  draw_weather_cloud(ctx, frame, palette, WEATHER_ICON_UNKNOWN);
  substratum_renderer_draw_unavailable_slash(
      ctx,
      &frame->size,
      palette->primary_text);
}

static WeatherIconKind get_weather_icon_kind(int16_t weather_condition) {
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

void draw_climate_icon(
    GContext* ctx,
    const GRect* frame,
    int16_t weather_condition,
    bool is_day,
    const ColorPalette* palette) {
  if (!ctx || !frame || !palette) {
    return;
  }

  WeatherIconKind icon_kind = get_weather_icon_kind(weather_condition);

  switch (icon_kind) {
    case WEATHER_ICON_CLEAR:
      if (is_day) {
        draw_weather_sun(ctx, frame, palette);
      } else {
        draw_weather_clear_icon(ctx, frame, palette);
      }
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
      substratum_renderer_draw_filled_bolt_in_frame(
          ctx,
          frame,
          weather_color_for_kind(WEATHER_ICON_THUNDERSTORM, palette));
      break;
    case WEATHER_ICON_UNKNOWN:
      draw_weather_unavailable_icon(ctx, frame, palette);
      break;
  }
}
