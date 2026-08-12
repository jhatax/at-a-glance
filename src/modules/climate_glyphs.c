#include "climate_glyphs.h"

#include "helper.h"
#include "substratum_renderer.h"
#include "watchface_debug.h"

/*
 * File invariants:
 *
 * - one public contract
 *   This file exposes only the climate glyph API declared in climate_glyphs.h.
 *
 * - private static helpers only
 *   Internal mapping, color selection, geometry, and drawing helpers stay
 *   file-local.
 *
 * - no layout ownership leaking in
 *   This file consumes caller-provided frames and palette data. It does not
 *   decide watchface layout, placement policy, or surface ownership.
 *
 * - no app/runtime policy leaking in
 *   This file maps already-resolved climate facts to glyph kinds and draws
 *   them. App lifecycle, transport, settings, and refresh policy belong
 *   elsewhere.
 *
 * - no duplicated primitive drawing logic
 *   Shared line, point, rect, path, and scaling primitives belong in
 *   substratum_renderer.c/.h, not in repeated local implementations here.
 *
 * - clear section ordering inside the file
 *   Keep the file ordered as:
 *   1. private enums/constants
 *   2. private geometry/color/mapping helpers
 *   3. private glyph drawing helpers
 *   4. public climate glyph contract
 */

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
  WEATHER_ICON_OUTOFRANGE,
} WeatherIconKind;

static GColor weather_color_for_kind(
    WeatherIconKind kind,
    const ClimatePalette* climate_palette) {
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
    case WEATHER_ICON_OUTOFRANGE:
      return climate_palette->outofrange;
  }

  return climate_palette->normal;
}

static void draw_weather_sun(
    GContext* ctx,
    const GRect* frame,
    const ClimatePalette* climate_palette) {
  if (!ctx || !frame || !climate_palette) {
    return;
  }
  const uint8_t sun_radius = 6;
  const uint8_t sun_dia = sun_radius << 1;
  const uint8_t stroke_w =
      HELPER_CLAMP_MIN((substratum_renderer_scale_icon_coord(&frame->size, 3)), 2);
  graphics_context_set_stroke_color(ctx, climate_palette->sun);
  graphics_context_set_stroke_width(ctx, stroke_w);

  substratum_renderer_draw_scaled_line_in_frame(
      ctx,
      frame,
      WATCHFACE_ICON_CENTER_X,
      WATCHFACE_ICON_CENTER_Y - sun_dia,
      WATCHFACE_ICON_CENTER_X,
      WATCHFACE_ICON_CENTER_Y + sun_dia);

  substratum_renderer_draw_scaled_line_in_frame(
      ctx,
      frame,
      WATCHFACE_ICON_CENTER_X - sun_dia,
      WATCHFACE_ICON_CENTER_Y,
      WATCHFACE_ICON_CENTER_X + sun_dia,
      WATCHFACE_ICON_CENTER_Y);

  substratum_renderer_draw_scaled_line_in_frame(
      ctx,
      frame,
      WATCHFACE_ICON_CENTER_X - 10,
      WATCHFACE_ICON_CENTER_Y - 10,
      WATCHFACE_ICON_CENTER_X + 10,
      WATCHFACE_ICON_CENTER_Y + 10);

  substratum_renderer_draw_scaled_line_in_frame(
      ctx,
      frame,
      WATCHFACE_ICON_CENTER_X - 10,
      WATCHFACE_ICON_CENTER_Y + 10,
      WATCHFACE_ICON_CENTER_X + 10,
      WATCHFACE_ICON_CENTER_Y - 10);

  graphics_context_set_fill_color(ctx, climate_palette->background);
  substratum_renderer_fill_scaled_circle_in_frame(
      ctx,
      frame,
      WATCHFACE_ICON_CENTER_X,
      WATCHFACE_ICON_CENTER_Y,
      sun_radius);

  substratum_renderer_draw_scaled_circle_in_frame(
      ctx,
      frame,
      WATCHFACE_ICON_CENTER_X,
      WATCHFACE_ICON_CENTER_Y,
      sun_radius);
}

static void draw_weather_cloud(
    GContext* ctx,
    const GRect* frame,
    const ClimatePalette* climate_palette,
    WeatherIconKind icon_kind) {
  if (!ctx || !frame || !climate_palette) {
    return;
  }
  // Cloud contract: three lobes plus a shared body fill, tuned for compact
  // displays so the silhouette reads as one cloud instead of three circles
  // with a heavy lower-right bias after scaling.
  const int16_t left_cloud_center_x = 6;
  const int16_t left_cloud_center_y = 18;
  const int16_t left_cloud_radius = 5;
  const int16_t center_cloud_center_x = 15;
  const int16_t center_cloud_center_y = 15;
  const int16_t center_cloud_radius = 8;
  const int16_t right_cloud_center_x = 21;
  const int16_t right_cloud_center_y = left_cloud_center_y + 1;
  const int16_t right_cloud_radius = left_cloud_radius;
  const int16_t cloud_fill_left = 5;
  const int16_t cloud_fill_top = 23;
  const int16_t cloud_fill_right = 22;
  const int16_t cloud_fill_bottom = 26;
  GColor color = weather_color_for_kind(icon_kind, climate_palette);

  graphics_context_set_stroke_color(ctx, color);

  int stroke_width =
      SUBSTRATUM_RENDERER_ICON_STROKE_WIDTH(HELPER_MIN(frame->size.w, frame->size.h), 3);

  graphics_context_set_stroke_width(ctx, stroke_width);

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

  graphics_context_set_fill_color(ctx, climate_palette->background);
  substratum_renderer_fill_scaled_circle_in_frame(
      ctx,
      frame,
      left_cloud_center_x,
      left_cloud_center_y,
      left_cloud_radius - 1);

  substratum_renderer_fill_scaled_circle_in_frame(
      ctx,
      frame,
      center_cloud_center_x,
      center_cloud_center_y,
      center_cloud_radius - 1);

  substratum_renderer_fill_scaled_circle_in_frame(
      ctx,
      frame,
      right_cloud_center_x,
      right_cloud_center_y,
      right_cloud_radius - 1);

  substratum_renderer_fill_scaled_rect_from_corners_in_frame(
      ctx,
      frame,
      cloud_fill_left,
      cloud_fill_top,
      cloud_fill_right,
      cloud_fill_bottom);

  graphics_context_set_fill_color(ctx, color);
  graphics_context_set_stroke_width(ctx, 1);
  substratum_renderer_draw_scaled_line_in_frame(
      ctx,
      frame,
      center_cloud_center_x,
      left_cloud_center_y,
      right_cloud_center_x,
      left_cloud_center_y);
  substratum_renderer_fill_scaled_rect_from_corners_in_frame(
      ctx,
      frame,
      cloud_fill_left,
      (cloud_fill_top + 1),
      cloud_fill_right,
      cloud_fill_bottom);
}

static void draw_weather_clear_icon(
    GContext* ctx,
    const GRect* frame,
    const ClimatePalette* climate_palette,
    bool is_day) {
  if (!ctx || !frame || !climate_palette) {
    return;
  }
  int16_t inner_radius = 10;
  int16_t outer_radius = 13;

  GColor inner = HELPER_IF_ELSE(is_day, climate_palette->background, climate_palette->clear_fill);
  GColor outer = HELPER_IF_ELSE(is_day, climate_palette->sun, climate_palette->background);
  GColor line = HELPER_IF_ELSE(is_day, climate_palette->sun, climate_palette->clear_ring);

  graphics_context_set_fill_color(ctx, outer);
  substratum_renderer_fill_scaled_circle_in_frame(
      ctx,
      frame,
      WATCHFACE_ICON_CENTER_X,
      WATCHFACE_ICON_CENTER_Y,
      outer_radius);

  graphics_context_set_fill_color(ctx, inner);
  substratum_renderer_fill_scaled_circle_in_frame(
      ctx,
      frame,
      WATCHFACE_ICON_CENTER_X,
      WATCHFACE_ICON_CENTER_Y,
      inner_radius);

  graphics_context_set_stroke_color(ctx, line);
  graphics_context_set_stroke_width(ctx, 1);
  substratum_renderer_draw_scaled_circle_in_frame(
      ctx,
      frame,
      WATCHFACE_ICON_CENTER_X,
      WATCHFACE_ICON_CENTER_Y,
      outer_radius);

  substratum_renderer_draw_scaled_circle_in_frame(
      ctx,
      frame,
      WATCHFACE_ICON_CENTER_X,
      WATCHFACE_ICON_CENTER_Y,
      inner_radius);
}

static void draw_weather_partly_cloudy_icon(
    GContext* ctx,
    const GRect* frame,
    const ClimatePalette* climate_palette,
    bool is_day) {
  if (!ctx || !frame || !climate_palette) {
    return;
  }
  GRect sub_frame = {0};

  substratum_renderer_create_subframe(frame, &sub_frame, 10, 0, 18, 18);
  draw_weather_clear_icon(ctx, &sub_frame, climate_palette, is_day);

  substratum_renderer_create_subframe(frame, &sub_frame, 0, 3, 24, 24);
  draw_weather_cloud(ctx, &sub_frame, climate_palette, WEATHER_ICON_PARTLY_CLOUDY);
}

static void draw_weather_fog_icon(
    GContext* ctx,
    const GRect* frame,
    const ClimatePalette* climate_palette) {
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

static void draw_weather_drizzle_icon(
    GContext* ctx,
    const GRect* frame,
    const ClimatePalette* climate_palette) {
  // Drizzle contract: three heavier, staggered marks so the glyph does not
  // collapse into dots on compact monochrome displays.
  if (!ctx || !frame || !climate_palette) {
    return;
  }
  graphics_context_set_stroke_color(
      ctx,
      weather_color_for_kind(WEATHER_ICON_DRIZZLE, climate_palette));

  // Post visual audit: Design decision to use the same stroke width on all canvas sizes
  graphics_context_set_stroke_width(ctx, 2);
  substratum_renderer_draw_scaled_line_in_frame(ctx, frame, 7, 5, 4, 13);
  substratum_renderer_draw_scaled_line_in_frame(ctx, frame, 14, 10, 11, 18);
  substratum_renderer_draw_scaled_line_in_frame(ctx, frame, 21, 15, 18, 23);
}

static void draw_weather_rain_marks(
    GContext* ctx,
    const GRect* frame,
    GColor color,
    bool heavy) {
  if (!ctx || !frame) {
    return;
  }

  graphics_context_set_stroke_color(ctx, color);
  graphics_context_set_stroke_width(
      ctx,
      SUBSTRATUM_RENDERER_ICON_STROKE_WIDTH(HELPER_MIN(frame->size.w, frame->size.h), 2));
  if (heavy) {
    substratum_renderer_draw_scaled_line_in_frame(ctx, frame, 4, 1, 1, 26);
    substratum_renderer_draw_scaled_line_in_frame(ctx, frame, 11, 1, 8, 26);
    substratum_renderer_draw_scaled_line_in_frame(ctx, frame, 18, 1, 15, 26);
    substratum_renderer_draw_scaled_line_in_frame(ctx, frame, 26, 1, 23, 26);
  } else {
    substratum_renderer_draw_scaled_line_in_frame(ctx, frame, 4, 1, 1, 16);
    substratum_renderer_draw_scaled_line_in_frame(ctx, frame, 11, 1, 8, 16);
    substratum_renderer_draw_scaled_line_in_frame(ctx, frame, 18, 1, 15, 16);
    substratum_renderer_draw_scaled_line_in_frame(ctx, frame, 26, 1, 23, 16);
  }
}

static void draw_weather_rain_icon(
    GContext* ctx,
    const GRect* frame,
    bool heavy,
    const ClimatePalette* climate_palette) {
  if (!ctx || !frame || !climate_palette) {
    return;
  }
  GColor color = weather_color_for_kind(
      HELPER_IF_ELSE(heavy, WEATHER_ICON_HEAVY_RAIN, WEATHER_ICON_RAIN),
      climate_palette);

  draw_weather_rain_marks(ctx, frame, color, heavy);
}

static GPoint snowflake_point(
    const GPoint* center,
    int32_t cos_a,
    int32_t sin_a,
    int x,
    int y) {
  int32_t r_x = ((int32_t)x * cos_a - (int32_t)y * sin_a);
  int32_t r_y = ((int32_t)x * sin_a + (int32_t)y * cos_a);

  return GPoint(
      center->x + HELPER_SCALE_ROUND(r_x, 1, TRIG_MAX_RATIO),
      center->y + HELPER_SCALE_ROUND(r_y, 1, TRIG_MAX_RATIO));
}

static void draw_snowflake_segment_fixed(
    GContext* ctx,
    GPoint* center,  // Pass the pre-calculated absolute screen center pixel
    int32_t cos_a,
    int32_t sin_a,
    int x1,
    int y1,
    int x2,
    int y2) {
  GPoint scaled_1 = snowflake_point(center, cos_a, sin_a, x1, y1);
  GPoint scaled_2 = snowflake_point(center, cos_a, sin_a, x2, y2);

  graphics_draw_line(ctx, scaled_1, scaled_2);
}

static void draw_weather_snowflake(
    GContext* ctx,
    const GRect* frame,
    GColor color) {
  if (!ctx || !frame) {
    return;
  }

  bool is_mini = ((HELPER_MIN(frame->size.w, frame->size.h)) < 10);
  graphics_context_set_stroke_color(ctx, color);
  graphics_context_set_fill_color(ctx, color);

  uint8_t stroke_width = 1;
  graphics_context_set_stroke_width(ctx, stroke_width);

  // Calculate the EXACT absolute center pixel on the display screen right now
  GPoint absolute_center =
      GPoint(frame->origin.x + (frame->size.w / 2), frame->origin.y + (frame->size.h / 2));

  // Scale the snowflake radius based on the actual pixel box available
  // If frame width is 28, max_radius will be 13 pixels out from center.
  const int max_radius = HELPER_ROUND_UP((HELPER_MIN(frame->size.w, frame->size.h)), 2) - 1;

  // Keep the features proportionally scaled to the radius size
  const int inner_v_pos = HELPER_ROUND_UP((max_radius * 35), 100);  // 35% out
  const int outer_v_pos = HELPER_ROUND_UP((max_radius * 75), 100);  // 75% out
  const int feather_w = HELPER_CLAMP_MIN(HELPER_ROUND_UP(max_radius, 4), 1);
  const int feather_h = feather_w;

  for (uint16_t angle_deg = 0; angle_deg < 360; angle_deg += 60) {
    int32_t angle = (angle_deg * TRIG_MAX_ANGLE) / 360;
    int32_t sin_a = sin_lookup(angle);
    int32_t cos_a = cos_lookup(angle);

    draw_snowflake_segment_fixed(ctx, &absolute_center, cos_a, sin_a, 0, 0, 0, -max_radius);

    draw_snowflake_segment_fixed(
        ctx,
        &absolute_center,
        cos_a,
        sin_a,
        0,
        -outer_v_pos,
        feather_w,
        -outer_v_pos - feather_h);

    draw_snowflake_segment_fixed(
        ctx,
        &absolute_center,
        cos_a,
        sin_a,
        0,
        -outer_v_pos,
        -feather_w,
        -outer_v_pos - feather_h);

    draw_snowflake_segment_fixed(
        ctx,
        &absolute_center,
        cos_a,
        sin_a,
        0,
        -max_radius,
        -1,
        -max_radius - 1);
    draw_snowflake_segment_fixed(
        ctx,
        &absolute_center,
        cos_a,
        sin_a,
        0,
        -max_radius,
        1,
        -max_radius - 1);

    if (!is_mini) {
      draw_snowflake_segment_fixed(
          ctx,
          &absolute_center,
          cos_a,
          sin_a,
          -1,
          -max_radius - 1,
          0,
          -max_radius);
      draw_snowflake_segment_fixed(
          ctx,
          &absolute_center,
          cos_a,
          sin_a,
          1,
          -max_radius - 1,
          0,
          -max_radius);

      draw_snowflake_segment_fixed(
          ctx,
          &absolute_center,
          cos_a,
          sin_a,
          0,
          -inner_v_pos,
          feather_w,
          -inner_v_pos - feather_h);
      draw_snowflake_segment_fixed(
          ctx,
          &absolute_center,
          cos_a,
          sin_a,
          0,
          -inner_v_pos,
          -feather_w,
          -inner_v_pos - feather_h);
    }
  }

  // Draw core central dot using native circle fill matching the absolute center
  graphics_fill_circle(ctx, absolute_center, stroke_width);
}

static void draw_weather_sleet_icon(
    GContext* ctx,
    const GRect* frame,
    const ClimatePalette* climate_palette,
    bool heavy) {
  if (!ctx || !frame || !climate_palette) {
    return;
  }
  GRect sub_frame = {0};

  // Draw the cloud first
  substratum_renderer_create_subframe(frame, &sub_frame, 3, 0, 20, 12);
  draw_weather_cloud(ctx, &sub_frame, climate_palette, WEATHER_ICON_CLOUD);

  GColor color = weather_color_for_kind(
      HELPER_IF_ELSE(heavy, WEATHER_ICON_SLEET_HEAVY, WEATHER_ICON_SLEET_DRIZZLE),
      climate_palette);

  // Set the right sub-frame
  substratum_renderer_create_subframe(frame, &sub_frame, 14, 12, 13, 13);
  if (heavy) {
    // Draw a snow-flake in this sub-frame
    graphics_context_set_stroke_color(ctx, color);
    draw_weather_snowflake(ctx, &sub_frame, color);
  } else {
    // Draw rain
    draw_weather_rain_icon(ctx, &sub_frame, true, climate_palette);
  }

  // Now that the heaviness is determined, draw the left sub-frame
  // And set the stroke color in case it was changed
  substratum_renderer_create_subframe(frame, &sub_frame, 0, 14, 12, 12);
  graphics_context_set_stroke_color(ctx, color);
  draw_weather_snowflake(ctx, &sub_frame, color);
}

static void draw_weather_snow_showers_icon(
    GContext* ctx,
    const GRect* frame,
    const ClimatePalette* climate_palette) {
  if (!ctx || !frame || !climate_palette) {
    return;
  }
  GRect sub_frame = {0};

  // Drawing multiple snowflakes
  GColor color = weather_color_for_kind(WEATHER_ICON_SNOW_SHOWERS, climate_palette);

  substratum_renderer_create_subframe(frame, &sub_frame, 0, 0, 12, 12);
  draw_weather_snowflake(ctx, &sub_frame, color);

  substratum_renderer_create_subframe(frame, &sub_frame, 15, 2, 12, 12);
  draw_weather_snowflake(ctx, &sub_frame, color);

  substratum_renderer_create_subframe(frame, &sub_frame, 4, 15, 12, 12);
  draw_weather_snowflake(ctx, &sub_frame, color);
}

static void draw_weather_showers_icon(
    GContext* ctx,
    const GRect* frame,
    bool heavy,
    const ClimatePalette* climate_palette) {
  if (!ctx || !frame || !climate_palette) {
    return;
  }
  GRect sub_frame = {0};

  WeatherIconKind kind = HELPER_IF_ELSE(heavy, WEATHER_ICON_HEAVY_SHOWERS, WEATHER_ICON_SHOWERS);
  GColor color = weather_color_for_kind(kind, climate_palette);

  substratum_renderer_create_subframe(frame, &sub_frame, 0, 0, 24, 12);
  draw_weather_cloud(ctx, &sub_frame, climate_palette, kind);

  substratum_renderer_create_subframe(frame, &sub_frame, 2, 8, 24, 18);
  draw_weather_rain_marks(ctx, &sub_frame, color, heavy);
}

static void draw_weather_condition_outofrange_icon(
    GContext* ctx,
    const GRect* frame,
    const ClimatePalette* climate_palette) {
  if (!ctx || !frame || !climate_palette) {
    return;
  }
  draw_weather_cloud(ctx, frame, climate_palette, WEATHER_ICON_OUTOFRANGE);
  substratum_renderer_mark_info_outofrange(
      ctx,
      frame,
      climate_palette->outofrange,
      climate_palette->background);
}

static WeatherIconKind get_weather_icon_kind(
    int16_t weather_condition) {
  if (weather_condition < 0) {
    return WEATHER_ICON_OUTOFRANGE;
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

  return WEATHER_ICON_OUTOFRANGE;
}

void draw_climate_icon(
    GContext* ctx,
    const GRect* frame,
    int16_t weather_condition,
    bool is_day,
    const ClimatePalette* climate_palette) {
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
        draw_weather_clear_icon(ctx, frame, climate_palette, false);
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
      draw_weather_snowflake(
          ctx,
          frame,
          weather_color_for_kind(WEATHER_ICON_SNOW, climate_palette));
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
          ctx,
          frame,
          weather_color_for_kind(WEATHER_ICON_THUNDERSTORM, climate_palette));
      break;
    case WEATHER_ICON_OUTOFRANGE:
      draw_weather_condition_outofrange_icon(ctx, frame, climate_palette);
      break;
  }
}
