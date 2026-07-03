#include "climate_glyphs.h"
#include "helper.h"
#include "substratum_renderer.h"
#include "watchface_debug.h"
#include <stdint.h>

typedef enum {
  WEATHER_ICON_CLEAR = 0,
  WEATHER_ICON_SUNNY,
  WEATHER_ICON_PARTLY_CLOUDY,
  WEATHER_ICON_CLOUD,
  WEATHER_ICON_FOG,
  WEATHER_ICON_DRIZZLE,
  WEATHER_ICON_RAIN,
  WEATHER_ICON_HEAVY_RAIN,
  WEATHER_ICON_SLEET_DRIZZLE,
  WEATHER_ICON_SLEET_HEAVY,
  WEATHER_ICON_SNOW,
  WEATHER_ICON_SHOWERS,
  WEATHER_ICON_HEAVY_SHOWERS,
  WEATHER_ICON_SNOW_SHOWERS,
  WEATHER_ICON_THUNDERSTORM,
  WEATHER_ICON_UNKNOWN,
} WeatherIconKind;

enum {
  WEATHER_ICON_GRID_W = SUBSTRATUM_RENDERER_ICON_GRID_W,
  WEATHER_ICON_GRID_H = SUBSTRATUM_RENDERER_ICON_GRID_H,
  WEATHER_ICON_CENTER_X = WEATHER_ICON_GRID_W / 2,
  WEATHER_ICON_CENTER_Y = WEATHER_ICON_GRID_H / 2,
};

static void weather_subframe(const GRect *frame, GRect *out, int16_t x, int16_t y, int16_t w,
                             int16_t h) {
  if (!frame || !out) {
    return;
  }

  // Set the point to be as it would have been in the reference design
  // Once you've set this up, you can scale the point's coordinates
  // to the frame's width and height
  out->origin.x = x;
  out->origin.y = y;
  substratum_renderer_scale_icon_point_in_frame(frame, &out->origin);
  out->size.w = HELPER_SCALE_ROUND(w, frame->size.w, WEATHER_ICON_GRID_W);
  out->size.h = HELPER_SCALE_ROUND(h, frame->size.h, WEATHER_ICON_GRID_H);
}

static GColor weather_clear_ring_color(const ClimatePalette *climate_palette, bool is_day) {
  if (!climate_palette) {
    return GColorWhite;
  }
  return HELPER_IF_ELSE(is_day, climate_palette->sun, climate_palette->clear_ring);
}

static GColor weather_clear_fill_color(const ClimatePalette *climate_palette, bool is_day) {
  if (!climate_palette) {
    return GColorWhite;
  }
  return HELPER_IF_ELSE(is_day, climate_palette->sun, climate_palette->clear_fill);
}

static GColor weather_color_for_kind(WeatherIconKind kind, const ClimatePalette *climate_palette) {
  if (!climate_palette) {
    return GColorWhite;
  }
  switch (kind) {
  case WEATHER_ICON_THUNDERSTORM:
  case WEATHER_ICON_SUNNY:
    return climate_palette->sun;
  case WEATHER_ICON_SNOW:
  case WEATHER_ICON_SNOW_SHOWERS:
  case WEATHER_ICON_SLEET_DRIZZLE:
  case WEATHER_ICON_SLEET_HEAVY:
    return climate_palette->cold;
  case WEATHER_ICON_DRIZZLE:
  case WEATHER_ICON_RAIN:
  case WEATHER_ICON_HEAVY_RAIN:
  case WEATHER_ICON_SHOWERS:
  case WEATHER_ICON_HEAVY_SHOWERS:
  case WEATHER_ICON_CLOUD:
  case WEATHER_ICON_PARTLY_CLOUDY:
  case WEATHER_ICON_FOG:
    return climate_palette->cloud;
  case WEATHER_ICON_CLEAR:
    return climate_palette->normal;
  case WEATHER_ICON_UNKNOWN:
    return climate_palette->unknown;
  }

  return climate_palette->normal;
}

static void draw_weather_outline_sun(GContext *ctx, const GRect *frame,
                                     const ClimatePalette *climate_palette, bool is_outline) {
  if (!ctx || !frame || !climate_palette) {
    return;
  }
  GColor color = weather_color_for_kind(WEATHER_ICON_SUNNY, climate_palette);
  GColor sunFillColor = HELPER_IF_ELSE(is_outline, climate_palette->background, color);
  graphics_context_set_fill_color(ctx, sunFillColor);
  graphics_context_set_stroke_color(ctx, color);
  graphics_context_set_stroke_width(
      ctx, SUBSTRATUM_RENDERER_ICON_STROKE_WIDTH(HELPER_MIN(frame->size.w, frame->size.h), 3));
  substratum_renderer_draw_scaled_circle_in_frame(ctx, frame, WEATHER_ICON_CENTER_X,
                                                  WEATHER_ICON_CENTER_Y, 6);
  substratum_renderer_draw_scaled_line_in_frame(ctx, frame, WEATHER_ICON_CENTER_X,
                                                WEATHER_ICON_CENTER_Y - 13, WEATHER_ICON_CENTER_X,
                                                WEATHER_ICON_CENTER_Y - 9);
  substratum_renderer_draw_scaled_line_in_frame(ctx, frame, WEATHER_ICON_CENTER_X,
                                                WEATHER_ICON_CENTER_Y + 9, WEATHER_ICON_CENTER_X,
                                                WEATHER_ICON_CENTER_Y + 13);
  substratum_renderer_draw_scaled_line_in_frame(ctx, frame, WEATHER_ICON_CENTER_X - 13,
                                                WEATHER_ICON_CENTER_Y, WEATHER_ICON_CENTER_X - 9,
                                                WEATHER_ICON_CENTER_Y);
  substratum_renderer_draw_scaled_line_in_frame(ctx, frame, WEATHER_ICON_CENTER_X + 9,
                                                WEATHER_ICON_CENTER_Y, WEATHER_ICON_CENTER_X + 13,
                                                WEATHER_ICON_CENTER_Y);
  substratum_renderer_draw_scaled_line_in_frame(
      ctx, frame, WEATHER_ICON_CENTER_X - 10, WEATHER_ICON_CENTER_Y - 10, WEATHER_ICON_CENTER_X - 7,
      WEATHER_ICON_CENTER_Y - 7);
  substratum_renderer_draw_scaled_line_in_frame(
      ctx, frame, WEATHER_ICON_CENTER_X + 7, WEATHER_ICON_CENTER_Y - 7, WEATHER_ICON_CENTER_X + 10,
      WEATHER_ICON_CENTER_Y - 10);
  substratum_renderer_draw_scaled_line_in_frame(
      ctx, frame, WEATHER_ICON_CENTER_X - 10, WEATHER_ICON_CENTER_Y + 10, WEATHER_ICON_CENTER_X - 7,
      WEATHER_ICON_CENTER_Y + 7);
  substratum_renderer_draw_scaled_line_in_frame(
      ctx, frame, WEATHER_ICON_CENTER_X + 7, WEATHER_ICON_CENTER_Y + 7, WEATHER_ICON_CENTER_X + 10,
      WEATHER_ICON_CENTER_Y + 10);
}

static void draw_weather_sun(GContext *ctx, const GRect *frame,
                             const ClimatePalette *climate_palette) {
  if (!ctx || !frame || !climate_palette) {
    return;
  }
  draw_weather_outline_sun(ctx, frame, climate_palette, false);
}

static void draw_weather_filled_cloud(GContext *ctx, const GRect *frame,
                                      const ClimatePalette *climate_palette,
                                      WeatherIconKind icon_kind, bool is_filled) {
  if (!ctx || !frame || !climate_palette) {
    return;
  }
  // Cloud contract: three lobes plus a shared body fill, tuned for compact
  // displays so the silhouette reads as one cloud instead of three circles
  // with a heavy lower-right bias after scaling.
  const int16_t left_cloud_center_x = 7;
  const int16_t left_cloud_center_y = 18;
  const int16_t left_cloud_radius = 4;
  const int16_t center_cloud_center_x = 14;
  const int16_t center_cloud_center_y = 12;
  const int16_t center_cloud_radius = 6;
  const int16_t right_cloud_center_x = 21;
  const int16_t right_cloud_center_y = 18;
  const int16_t right_cloud_radius = 4;
  const int16_t cloud_fill_left = 1;
  const int16_t cloud_fill_top = 20;
  const int16_t cloud_fill_right = 23;
  const int16_t cloud_fill_bottom = 23;
  const int16_t cloud_base_left = 2;
  const int16_t cloud_base_right = 22;
  const int16_t cloud_base_y = 22;
  GColor color = weather_color_for_kind(icon_kind, climate_palette);
  GColor cloudFillColor = HELPER_IF_ELSE(is_filled, color, climate_palette->background);

  graphics_context_set_stroke_color(ctx, color);
  graphics_context_set_fill_color(ctx, cloudFillColor);

  int stroke_width =
      SUBSTRATUM_RENDERER_ICON_STROKE_WIDTH(HELPER_MIN(frame->size.w, frame->size.h), 3);
  int fill_offset = stroke_width >> 1;

  graphics_context_set_stroke_width(ctx, stroke_width);

  substratum_renderer_draw_scaled_circle_in_frame(ctx, frame, left_cloud_center_x,
                                                  left_cloud_center_y, left_cloud_radius);
  substratum_renderer_draw_scaled_circle_in_frame(ctx, frame, center_cloud_center_x,
                                                  center_cloud_center_y, center_cloud_radius);
  substratum_renderer_draw_scaled_circle_in_frame(ctx, frame, right_cloud_center_x,
                                                  right_cloud_center_y, right_cloud_radius);

  substratum_renderer_fill_scaled_circle_in_frame(ctx, frame, left_cloud_center_x,
                                                  left_cloud_center_y, 4 - fill_offset);
  substratum_renderer_fill_scaled_circle_in_frame(ctx, frame, center_cloud_center_x,
                                                  center_cloud_center_y, 6 - fill_offset);
  substratum_renderer_fill_scaled_circle_in_frame(ctx, frame, right_cloud_center_x,
                                                  right_cloud_center_y, 4 - fill_offset);
  substratum_renderer_fill_scaled_rect_from_corners_in_frame(
      ctx, frame, cloud_fill_left, cloud_fill_top, cloud_fill_right, cloud_fill_bottom);

  substratum_renderer_draw_scaled_line_in_frame(ctx, frame, cloud_base_left, cloud_base_y,
                                                cloud_base_right, cloud_base_y);
}

static void draw_weather_cloud(GContext *ctx, const GRect *frame,
                               const ClimatePalette *climate_palette, WeatherIconKind icon_kind) {
  if (!ctx || !frame || !climate_palette) {
    return;
  }
  draw_weather_filled_cloud(ctx, frame, climate_palette, icon_kind, false);
}

static void draw_weather_clear_icon(GContext *ctx, const GRect *frame,
                                    const ClimatePalette *climate_palette, bool is_day,
                                    bool is_filled) {
  if (!ctx || !frame || !climate_palette) {
    return;
  }
  // Inner-ring filled, outer ring dithered
  // vs. Inner-ring dithered, outer-ring filled
  int16_t filled_radius = HELPER_IF_ELSE((is_filled), 10, 13);
  int16_t dithered_radius = HELPER_IF_ELSE((is_filled), 13, 10);

  // Draw the filled circle first
  graphics_context_set_fill_color(ctx, weather_clear_fill_color(climate_palette, is_day));
  substratum_renderer_fill_scaled_circle_in_frame(ctx, frame, WEATHER_ICON_CENTER_X,
                                                  WEATHER_ICON_CENTER_Y, filled_radius);

  // Draw the dithered circle next
  graphics_context_set_stroke_color(ctx, weather_clear_ring_color(climate_palette, is_day));
  graphics_context_set_stroke_width(ctx, 1);
  substratum_renderer_draw_scaled_circle_in_frame(ctx, frame, WEATHER_ICON_CENTER_X,
                                                  WEATHER_ICON_CENTER_Y, dithered_radius);

  // Now, if the filled radius is larger than the dithered radius, you're
  // going to have to fill the inside of the dithered circle with the background
  // color
  if (filled_radius > dithered_radius) {
    graphics_context_set_fill_color(ctx, climate_palette->background);
    substratum_renderer_fill_scaled_circle_in_frame(ctx, frame, WEATHER_ICON_CENTER_X,
                                                    WEATHER_ICON_CENTER_Y, dithered_radius - 3);
  }
}

static void draw_weather_partly_cloudy_icon(GContext *ctx, const GRect *frame,
                                            const ClimatePalette *climate_palette, bool is_day) {
  if (!ctx || !frame || !climate_palette) {
    return;
  }
  GRect sub_frame = {0};

  weather_subframe(frame, &sub_frame, 10, 0, 18, 18);
  draw_weather_clear_icon(ctx, &sub_frame, climate_palette, is_day, false);

  weather_subframe(frame, &sub_frame, 0, 3, 24, 24);
  draw_weather_filled_cloud(ctx, &sub_frame, climate_palette, WEATHER_ICON_PARTLY_CLOUDY, false);
}

static void draw_weather_fog_icon(GContext *ctx, const GRect *frame,
                                  const ClimatePalette *climate_palette) {
  if (!ctx || !frame || !climate_palette) {
    return;
  }
  graphics_context_set_stroke_color(ctx, weather_color_for_kind(WEATHER_ICON_FOG, climate_palette));
  // Post visual audit: Design decision to use the same stroke width on all canvas sizes
  graphics_context_set_stroke_width(ctx, 2);
  substratum_renderer_draw_scaled_line_in_frame(ctx, frame, 5, 6, 23, 6);
  substratum_renderer_draw_scaled_line_in_frame(ctx, frame, 2, 14, 26, 14);
  substratum_renderer_draw_scaled_line_in_frame(ctx, frame, 7, 22, 21, 22);
}

static void draw_weather_drizzle_icon(GContext *ctx, const GRect *frame,
                                      const ClimatePalette *climate_palette) {
  // Drizzle contract: three heavier, staggered marks so the glyph does not
  // collapse into dots on compact monochrome displays.
  if (!ctx || !frame || !climate_palette) {
    return;
  }
  graphics_context_set_stroke_color(ctx,
                                    weather_color_for_kind(WEATHER_ICON_DRIZZLE, climate_palette));

  // Post visual audit: Design decision to use the same stroke width on all canvas sizes
  graphics_context_set_stroke_width(ctx, 2);
  substratum_renderer_draw_scaled_line_in_frame(ctx, frame, 7, 5, 4, 13);
  substratum_renderer_draw_scaled_line_in_frame(ctx, frame, 14, 10, 11, 18);
  substratum_renderer_draw_scaled_line_in_frame(ctx, frame, 21, 15, 18, 23);
}

static void draw_weather_rain_marks(GContext *ctx, const GRect *frame, GColor color, bool heavy) {
  if (!ctx || !frame) {
    return;
  }

  graphics_context_set_stroke_color(ctx, color);
  graphics_context_set_stroke_width(
      ctx, SUBSTRATUM_RENDERER_ICON_STROKE_WIDTH(HELPER_MIN(frame->size.w, frame->size.h), 2));
  if (heavy) {
    substratum_renderer_draw_scaled_line_in_frame(ctx, frame, 4, 1, 1, 27);
    substratum_renderer_draw_scaled_line_in_frame(ctx, frame, 11, 1, 8, 27);
    substratum_renderer_draw_scaled_line_in_frame(ctx, frame, 18, 1, 15, 27);
    substratum_renderer_draw_scaled_line_in_frame(ctx, frame, 26, 1, 23, 27);
  } else {
    substratum_renderer_draw_scaled_line_in_frame(ctx, frame, 4, 4, 1, 16);
    substratum_renderer_draw_scaled_line_in_frame(ctx, frame, 11, 4, 8, 16);
    substratum_renderer_draw_scaled_line_in_frame(ctx, frame, 18, 4, 15, 16);
    substratum_renderer_draw_scaled_line_in_frame(ctx, frame, 26, 4, 23, 16);
  }
}

static void draw_weather_rain_icon(GContext *ctx, const GRect *frame, bool heavy,
                                   const ClimatePalette *climate_palette) {
  if (!ctx || !frame || !climate_palette) {
    return;
  }
  draw_weather_rain_marks(
      ctx, frame,
      weather_color_for_kind(HELPER_IF_ELSE(heavy, WEATHER_ICON_HEAVY_RAIN, WEATHER_ICON_RAIN),
                             climate_palette),
      heavy);
}

static inline void
draw_snowflake_segment_fixed(GContext *ctx,
                             GPoint *center, // Pass the pre-calculated absolute screen center pixel
                             int32_t cos_a, int32_t sin_a, int x1, int y1, int x2, int y2) {

  // Calculate raw relative offsets
  int32_t r_x1 = ((int32_t)x1 * cos_a - (int32_t)y1 * sin_a);
  int32_t r_y1 = ((int32_t)x1 * sin_a + (int32_t)y1 * cos_a);
  int32_t r_x2 = ((int32_t)x2 * cos_a - (int32_t)y2 * sin_a);
  int32_t r_y2 = ((int32_t)x2 * sin_a + (int32_t)y2 * cos_a);

  // Apply absolute positioning and map directly to Pebble's hardware graphics engine
  graphics_draw_line(ctx,
                     GPoint(center->x + HELPER_SCALE_ROUND(r_x1, 1, TRIG_MAX_RATIO),
                            center->y + HELPER_SCALE_ROUND(r_y1, 1, TRIG_MAX_RATIO)),
                     GPoint(center->x + HELPER_SCALE_ROUND(r_x2, 1, TRIG_MAX_RATIO),
                            center->y + HELPER_SCALE_ROUND(r_y2, 1, TRIG_MAX_RATIO)));
}

static void draw_weather_snowflake(GContext *ctx, const GRect *frame, GColor color) {
  if (!ctx || !frame) {
    return;
  }

  graphics_context_set_stroke_color(ctx, color);
  graphics_context_set_fill_color(ctx, color);

  uint8_t stroke_width = 1;
  graphics_context_set_stroke_width(ctx, stroke_width);

  // Calculate the EXACT absolute center pixel on the display screen right now
  GPoint absolute_center =
      GPoint(frame->origin.x + (frame->size.w / 2), frame->origin.y + (frame->size.h / 2));

  // Scale the snowflake radius based on the actual pixel box available
  // If frame width is 28, max_radius will be 11 pixels out from center.
  const int max_radius = (HELPER_MIN(frame->size.w, frame->size.h) / 2) - 3;

  // Keep the features proportionally scaled to the radius size
  const int inner_v_pos = HELPER_ROUND_UP((max_radius * 4), 10); // 40% out
  const int outer_v_pos = HELPER_ROUND_UP((max_radius * 7), 10); // 70% out
  const int feather_w = HELPER_CLAMP_MIN(HELPER_ROUND_UP(max_radius, 4), 2);
  const int feather_h = HELPER_CLAMP_MIN(HELPER_ROUND_UP(max_radius, 5), 2);

  for (uint16_t angle_deg = 0; angle_deg < 360; angle_deg += 60) {
    int32_t angle = (angle_deg * TRIG_MAX_ANGLE) / 360;
    int32_t sin_a = sin_lookup(angle);
    int32_t cos_a = cos_lookup(angle);

    // 1. Main Spoke
    draw_snowflake_segment_fixed(ctx, &absolute_center, cos_a, sin_a, 0, 0, 0, -max_radius);

    // 2. Inner Chevron Branch
    draw_snowflake_segment_fixed(ctx, &absolute_center, cos_a, sin_a, 0, -inner_v_pos, -feather_w,
                                 -inner_v_pos - feather_h);
    draw_snowflake_segment_fixed(ctx, &absolute_center, cos_a, sin_a, 0, -inner_v_pos, feather_w,
                                 -inner_v_pos - feather_h);

    // 3. Outer Chevron Branch
    draw_snowflake_segment_fixed(ctx, &absolute_center, cos_a, sin_a, 0, -outer_v_pos, -feather_w,
                                 -outer_v_pos - feather_h);
    draw_snowflake_segment_fixed(ctx, &absolute_center, cos_a, sin_a, 0, -outer_v_pos, feather_w,
                                 -outer_v_pos - feather_h);

    // 4. Diamond Tip Accent
    draw_snowflake_segment_fixed(ctx, &absolute_center, cos_a, sin_a, 0, -max_radius, -1,
                                 -max_radius - 1);
    draw_snowflake_segment_fixed(ctx, &absolute_center, cos_a, sin_a, 0, -max_radius, 1,
                                 -max_radius - 1);
    draw_snowflake_segment_fixed(ctx, &absolute_center, cos_a, sin_a, -1, -max_radius - 1, 0,
                                 -max_radius - 2);
    draw_snowflake_segment_fixed(ctx, &absolute_center, cos_a, sin_a, 1, -max_radius - 1, 0,
                                 -max_radius - 2);
  }

  // Draw core central dot using native circle fill matching the absolute center
  graphics_fill_circle(ctx, absolute_center, stroke_width);
}

static void draw_weather_sleet_icon(GContext *ctx, const GRect *frame,
                                    const ClimatePalette *climate_palette, bool heavy) {
  if (!ctx || !frame || !climate_palette) {
    return;
  }
  GRect sub_frame = {0};

  // Draw the cloud first
  weather_subframe(frame, &sub_frame, 6, 0, 16, 14);
  draw_weather_cloud(ctx, &sub_frame, climate_palette, WEATHER_ICON_CLOUD);

  GColor color = weather_color_for_kind(
      HELPER_IF_ELSE(heavy, WEATHER_ICON_SLEET_HEAVY, WEATHER_ICON_SLEET_DRIZZLE), climate_palette);

  // Set the right sub-frame
  weather_subframe(frame, &sub_frame, 13, 13, 14, 14);
  if (heavy) {
    // Draw a snow-flake in this sub-frame
    graphics_context_set_stroke_color(ctx, color);
    draw_weather_snowflake(ctx, &sub_frame, color);
  } else {
    // Draw rain
    draw_weather_rain_icon(ctx, &sub_frame, false, climate_palette);
  }

  // Now that the heaviness is determined, draw the left sub-frame
  // And set the stroke color in case it was changed
  weather_subframe(frame, &sub_frame, 0, 13, 14, 14);
  graphics_context_set_stroke_color(ctx, color);
  draw_weather_snowflake(ctx, &sub_frame, color);
}

static void draw_weather_snow_showers_icon(GContext *ctx, const GRect *frame,
                                           const ClimatePalette *climate_palette) {
  if (!ctx || !frame || !climate_palette) {
    return;
  }
  GRect sub_frame = {0};

  weather_subframe(frame, &sub_frame, 0, 0, 18, 12);
  draw_weather_cloud(ctx, &sub_frame, climate_palette, WEATHER_ICON_CLOUD);

  GColor color = weather_color_for_kind(WEATHER_ICON_SNOW_SHOWERS, climate_palette);

  weather_subframe(frame, &sub_frame, 5, 9, 18, 18);
  draw_weather_snowflake(ctx, &sub_frame, color);
}

static void draw_weather_showers_icon(GContext *ctx, const GRect *frame, bool heavy,
                                      const ClimatePalette *climate_palette) {
  if (!ctx || !frame || !climate_palette) {
    return;
  }
  GRect sub_frame = {0};

  WeatherIconKind kind = HELPER_IF_ELSE(heavy, WEATHER_ICON_HEAVY_SHOWERS, WEATHER_ICON_SHOWERS);
  GColor color = weather_color_for_kind(kind, climate_palette);

  weather_subframe(frame, &sub_frame, 0, 0, 20, 15);
  draw_weather_cloud(ctx, &sub_frame, climate_palette, kind);

  weather_subframe(frame, &sub_frame, 2, 7, 16, 20);
  draw_weather_rain_marks(ctx, &sub_frame, color, heavy);
}

static void draw_weather_unavailable_icon(GContext *ctx, const GRect *frame,
                                          const ClimatePalette *climate_palette) {
  if (!ctx || !frame || !climate_palette) {
    return;
  }
  draw_weather_cloud(ctx, frame, climate_palette, WEATHER_ICON_UNKNOWN);
  substratum_renderer_draw_unavailable_slash(
    ctx,
    &frame->size,
    climate_palette->normal);
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
    return WEATHER_ICON_SLEET_DRIZZLE;
  }
  if (weather_condition <= 63) {
    return WEATHER_ICON_RAIN;
  }
  if (weather_condition <= 65) {
    return WEATHER_ICON_HEAVY_RAIN;
  }
  if (weather_condition <= 67) {
    return WEATHER_ICON_SLEET_HEAVY;
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

void draw_climate_icon(GContext *ctx, const GRect *frame, int16_t weather_condition, bool is_day,
                       const ClimatePalette *climate_palette) {
  if (!ctx || !frame || !climate_palette) {
    return;
  }

  WeatherIconKind icon_kind = get_weather_icon_kind(weather_condition);

#if ATAGLANCE_DEBUG
  // Draw a rectangle around the icon's bounds
  graphics_context_set_stroke_color(ctx, climate_palette->normal);
  graphics_draw_rect(ctx, *frame);
#endif

  switch (icon_kind) {
  case WEATHER_ICON_CLEAR:
    if (is_day) {
      draw_weather_sun(ctx, frame, climate_palette);
    } else {
      draw_weather_clear_icon(ctx, frame, climate_palette, is_day, true);
    }
    break;
  case WEATHER_ICON_SUNNY:
    draw_weather_sun(ctx, frame, climate_palette);
    break;
  case WEATHER_ICON_PARTLY_CLOUDY:
    draw_weather_partly_cloudy_icon(ctx, frame, climate_palette, is_day);
    break;
  case WEATHER_ICON_CLOUD:
    draw_weather_cloud(ctx, frame, climate_palette, icon_kind);
    break;
  case WEATHER_ICON_FOG:
    draw_weather_fog_icon(ctx, frame, climate_palette);
    break;
  case WEATHER_ICON_DRIZZLE:
    draw_weather_drizzle_icon(ctx, frame, climate_palette);
    break;
  case WEATHER_ICON_RAIN:
    draw_weather_rain_icon(ctx, frame, false, climate_palette);
    break;
  case WEATHER_ICON_HEAVY_RAIN:
    draw_weather_rain_icon(ctx, frame, true, climate_palette);
    break;
  case WEATHER_ICON_SLEET_DRIZZLE:
    draw_weather_sleet_icon(ctx, frame, climate_palette, false);
    break;
  case WEATHER_ICON_SLEET_HEAVY:
    draw_weather_sleet_icon(ctx, frame, climate_palette, true);
    break;
  case WEATHER_ICON_SNOW:
    draw_weather_snowflake(ctx, frame, weather_color_for_kind(WEATHER_ICON_SNOW, climate_palette));
    break;
  case WEATHER_ICON_SHOWERS:
    draw_weather_showers_icon(ctx, frame, false, climate_palette);
    break;
  case WEATHER_ICON_HEAVY_SHOWERS:
    draw_weather_showers_icon(ctx, frame, true, climate_palette);
    break;
  case WEATHER_ICON_SNOW_SHOWERS:
    draw_weather_snow_showers_icon(ctx, frame, climate_palette);
    break;
  case WEATHER_ICON_THUNDERSTORM:
    substratum_renderer_draw_filled_bolt_in_frame(
        ctx, frame, weather_color_for_kind(WEATHER_ICON_THUNDERSTORM, climate_palette));
    break;
  case WEATHER_ICON_UNKNOWN:
    draw_weather_unavailable_icon(ctx, frame, climate_palette);
    break;
  }
}
