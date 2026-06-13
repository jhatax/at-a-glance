#include "glyph_lab_glyphs.h"

#include "glyph_lab_helper.h"
#include "glyph_lab_renderer.h"

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
  WEATHER_ICON_CENTER_X = DESIGN_ICON_WIDTH / 2,
  WEATHER_ICON_CENTER_Y = DESIGN_ICON_HEIGHT / 2,
  WEATHER_ICON_WIDTH = DESIGN_ICON_WIDTH,
  WEATHER_ICON_HEIGHT = DESIGN_ICON_HEIGHT,
} WeatherIconGeometry;

void glyph_lab_select_palette(
    ColorPalette* palette,
    bool is_light_mode) {
  if (!palette) {
    return;
  }

  if (is_light_mode) {
    *palette = (ColorPalette) {
      .background = GColorWhite,
      .background_layer_background = GColorWhite,
      .background_layer_rule =
          PBL_IF_COLOR_ELSE(GColorOxfordBlue, GColorBlack),
      .primary_text = PBL_IF_COLOR_ELSE(GColorCobaltBlue, GColorBlack),
      .unavailable_text = PBL_IF_COLOR_ELSE(GColorDarkGray, GColorBlack),
      .date = PBL_IF_COLOR_ELSE(GColorBlack, GColorBlack),
      .time = PBL_IF_COLOR_ELSE(GColorSunsetOrange, GColorBlack),
    };
    return;
  }

  *palette = (ColorPalette) {
    .background = GColorBlack,
    .background_layer_background = GColorBlack,
    .background_layer_rule = GColorWhite,
    .primary_text = PBL_IF_COLOR_ELSE(GColorCeleste, GColorWhite),
    .unavailable_text = PBL_IF_COLOR_ELSE(GColorLightGray, GColorWhite),
    .date = PBL_IF_COLOR_ELSE(GColorElectricBlue, GColorWhite),
    .time = PBL_IF_COLOR_ELSE(GColorSunsetOrange, GColorWhite),
  };
}

static void draw_scaled_line_in_frame(
    GContext* ctx,
    const GRect* frame,
    int16_t x0,
    int16_t y0,
    int16_t x1,
    int16_t y1) {
  graphics_draw_line(
      ctx,
      GPoint(frame->origin.x + glyph_lab_scale_icon_x(&frame->size, x0),
             frame->origin.y + glyph_lab_scale_icon_y(&frame->size, y0)),
      GPoint(frame->origin.x + glyph_lab_scale_icon_x(&frame->size, x1),
             frame->origin.y + glyph_lab_scale_icon_y(&frame->size, y1)));
}

static void draw_unavailable_slash(
    GContext* ctx,
    const GRect* frame,
    GColor color) {
  graphics_context_set_stroke_color(ctx, color);
  graphics_context_set_stroke_width(ctx, 3);
  draw_scaled_line_in_frame(ctx, frame, 5, 5, 24, 24);
}

static GRect battery_scaled_rect_from_corners(
    const GSize* size,
    int16_t x0,
    int16_t y0,
    int16_t x1,
    int16_t y1) {
  int16_t left = glyph_lab_scale_icon_x(size, x0);
  int16_t top = glyph_lab_scale_icon_y(size, y0);
  int16_t right = glyph_lab_scale_icon_x(size, x1);
  int16_t bottom = glyph_lab_scale_icon_y(size, y1);

  return GRect(left, top, right - left, bottom - top);
}

static GRect offset_rect(GRect rect, GPoint offset) {
  rect.origin.x += offset.x;
  rect.origin.y += offset.y;
  return rect;
}

static GColor battery_color(
    const ColorPalette* palette,
    bool is_light_mode,
    int percent,
    bool is_charging) {
  if (is_charging) {
    return PBL_IF_COLOR_ELSE(GColorIslamicGreen, palette->primary_text);
  }

  if (percent > 50) {
    return palette->primary_text;
  }
  if (percent > 20) {
    return PBL_IF_COLOR_ELSE(
        is_light_mode ? GColorWindsorTan : GColorRajah,
        palette->primary_text);
  }

  return PBL_IF_COLOR_ELSE(
      is_light_mode ? GColorBulgarianRose : GColorRed,
      palette->primary_text);
}

static void draw_battery_charging_bolt(
    GContext* ctx,
    const GRect* frame,
    const ColorPalette* palette,
    GColor color) {
  graphics_context_set_stroke_color(ctx, palette->background);
  graphics_context_set_stroke_width(ctx, 1);
  draw_scaled_line_in_frame(ctx, frame, 17, 3, 10, 14);
  draw_scaled_line_in_frame(ctx, frame, 10, 14, 16, 14);
  draw_scaled_line_in_frame(ctx, frame, 16, 14, 11, 25);
  draw_scaled_line_in_frame(ctx, frame, 11, 25, 22, 11);
  draw_scaled_line_in_frame(ctx, frame, 22, 11, 16, 11);
  draw_scaled_line_in_frame(ctx, frame, 16, 11, 17, 3);

  graphics_context_set_stroke_color(ctx, color);
  graphics_context_set_stroke_width(ctx, 2);
  draw_scaled_line_in_frame(ctx, frame, 17, 3, 10, 14);
  draw_scaled_line_in_frame(ctx, frame, 10, 14, 16, 14);
  draw_scaled_line_in_frame(ctx, frame, 16, 14, 11, 25);
  draw_scaled_line_in_frame(ctx, frame, 11, 25, 22, 11);
  draw_scaled_line_in_frame(ctx, frame, 22, 11, 16, 11);
  draw_scaled_line_in_frame(ctx, frame, 16, 11, 17, 3);
}

void glyph_lab_draw_battery_icon(
    GContext* ctx,
    const GRect* frame,
    const ColorPalette* palette,
    bool is_light_mode,
    int percent,
    bool is_charging) {
  if (!ctx || !frame || !palette) {
    return;
  }

  const int16_t shell_left = 2;
  const int16_t shell_top = 4;
  const int16_t shell_right = 24;
  const int16_t shell_bottom = 19;
  const int16_t nub_left = 24;
  const int16_t nub_top = 8;
  const int16_t nub_right = 26;
  const int16_t nub_bottom = 15;
  const int16_t shell_stroke_width = 2;
  const int16_t inner_breathing_room = 1;
  const int16_t fill_inset = shell_stroke_width + inner_breathing_room;
  const int16_t fill_left = shell_left + fill_inset;
  const int16_t fill_top = shell_top + fill_inset;
  const int16_t fill_right = shell_right - fill_inset;
  const int16_t fill_bottom = shell_bottom - fill_inset;
  // battery_scaled_rect_from_corners() treats right/bottom as exclusive
  // edges, so the maximum unscaled fill span comes from the shell endpoints
  // minus the shell stroke and one extra pixel of inner breathing room on
  // each side.
  const int16_t fill_width_max = fill_right - fill_left;
  GColor draw_color = battery_color(
      palette,
      is_light_mode,
      percent,
      is_charging);
  int16_t fill_width = (percent * fill_width_max) / 100;
  GRect body = offset_rect(
      battery_scaled_rect_from_corners(
          &frame->size,
          shell_left,
          shell_top,
          shell_right,
          shell_bottom),
      frame->origin);
  GRect nub = offset_rect(
      battery_scaled_rect_from_corners(
          &frame->size,
          nub_left,
          nub_top,
          nub_right,
          nub_bottom),
      frame->origin);

  graphics_context_set_fill_color(ctx, palette->background);
  graphics_fill_rect(ctx, *frame, 0, GCornerNone);

  graphics_context_set_stroke_color(ctx, draw_color);
  graphics_context_set_fill_color(ctx, draw_color);
  graphics_context_set_stroke_width(ctx, shell_stroke_width);

  graphics_draw_rect(ctx, body);
  graphics_fill_rect(ctx, nub, 0, GCornerNone);

  if (fill_width < 1) {
    fill_width = 1;
  }

  graphics_fill_rect(
      ctx,
      offset_rect(
          battery_scaled_rect_from_corners(
              &frame->size,
              fill_left,
              fill_top,
              fill_left + fill_width,
              fill_bottom),
          frame->origin),
      0,
      GCornerNone);

  if (is_charging) {
    draw_battery_charging_bolt(ctx, frame, palette, draw_color);
  }
}

void glyph_lab_draw_steps_icon(
    GContext* ctx,
    const GRect* frame,
    const ColorPalette* palette,
    bool is_available) {
  if (!ctx || !frame || !palette) {
    return;
  }

  GColor steps_icon_color = is_available ?
      palette->primary_text : palette->unavailable_text;

  graphics_context_set_fill_color(ctx, palette->background);
  graphics_fill_rect(ctx, *frame, 0, GCornerNone);
  graphics_context_set_fill_color(ctx, steps_icon_color);

  graphics_fill_circle(
      ctx,
      GPoint(frame->origin.x + glyph_lab_scale_icon_x(&frame->size, 14),
             frame->origin.y + glyph_lab_scale_icon_y(&frame->size, 9)),
      glyph_lab_scale_icon_coord(&frame->size, 7));

  graphics_context_set_fill_color(ctx, palette->background);
  graphics_fill_rect(
      ctx,
      GRect(frame->origin.x + glyph_lab_scale_icon_x(&frame->size, 6),
            frame->origin.y + glyph_lab_scale_icon_y(&frame->size, 15),
            glyph_lab_scale_icon_x(&frame->size, 16),
            glyph_lab_scale_icon_y(&frame->size, 4)),
      0,
      GCornerNone);

  graphics_context_set_fill_color(ctx, steps_icon_color);
  graphics_fill_circle(
      ctx,
      GPoint(frame->origin.x + glyph_lab_scale_icon_x(&frame->size, 14),
             frame->origin.y + glyph_lab_scale_icon_y(&frame->size, 22)),
      glyph_lab_scale_icon_coord(&frame->size, 4));

  if (!is_available) {
    draw_unavailable_slash(ctx, frame, palette->primary_text);
  }
}

static GColor bpm_color(
    const ColorPalette* palette,
    bool is_light_mode,
    int bpm) {
  if (bpm <= 0) {
    return palette->unavailable_text;
  }
  if (bpm > 120) {
    return PBL_IF_COLOR_ELSE(
        is_light_mode ? GColorBulgarianRose : GColorOrange,
        palette->primary_text);
  }
  if (bpm >= 100) {
    return PBL_IF_COLOR_ELSE(
        is_light_mode ? GColorWindsorTan : GColorChromeYellow,
        palette->primary_text);
  }

  return palette->primary_text;
}

static void draw_bpm_icon_with_color(
    GContext* ctx,
    GColor color,
    const GRect* frame) {
  graphics_context_set_stroke_color(ctx, color);
  graphics_context_set_stroke_width(ctx, 2);
  draw_scaled_line_in_frame(ctx, frame, 3, 15, 8, 15);
  draw_scaled_line_in_frame(ctx, frame, 8, 15, 11, 8);
  draw_scaled_line_in_frame(ctx, frame, 11, 8, 15, 22);
  draw_scaled_line_in_frame(ctx, frame, 15, 22, 19, 12);
  draw_scaled_line_in_frame(ctx, frame, 19, 12, 24, 12);
}

void glyph_lab_draw_bpm_icon(
    GContext* ctx,
    const GRect* frame,
    const ColorPalette* palette,
    bool is_light_mode,
    int bpm,
    bool is_available) {
  if (!ctx || !frame || !palette) {
    return;
  }

  graphics_context_set_fill_color(ctx, palette->background);
  graphics_fill_rect(ctx, *frame, 0, GCornerNone);

  draw_bpm_icon_with_color(
      ctx,
      bpm_color(palette, is_light_mode, bpm),
      frame);

  if (!is_available) {
    draw_unavailable_slash(ctx, frame, palette->primary_text);
  }
}

static int16_t weather_scale_x(const GRect* frame, int16_t value) {
  return frame->origin.x + HELPER_SCALE_ROUND(
      value,
      frame->size.w,
      WEATHER_ICON_WIDTH);
}

static int16_t weather_scale_y(const GRect* frame, int16_t value) {
  return frame->origin.y + HELPER_SCALE_ROUND(
      value,
      frame->size.h,
      WEATHER_ICON_HEIGHT);
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
  out->size.w = HELPER_SCALE_ROUND(w, frame->size.w, WEATHER_ICON_WIDTH);
  out->size.h = HELPER_SCALE_ROUND(h, frame->size.h, WEATHER_ICON_HEIGHT);
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
            HELPER_SCALE_ROUND(w, frame->size.w, WEATHER_ICON_WIDTH),
            HELPER_SCALE_ROUND(h, frame->size.h, WEATHER_ICON_HEIGHT)),
      0,
      GCornerNone);
}

static void weather_fill_rect_from_corners(
    GContext* ctx,
    const GRect* frame,
    int16_t x0,
    int16_t y0,
    int16_t x1,
    int16_t y1) {
  graphics_fill_rect(
      ctx,
      GRect(weather_scale_x(frame, x0),
            weather_scale_y(frame, y0),
            weather_scale_x(frame, x1) - weather_scale_x(frame, x0),
            weather_scale_y(frame, y1) - weather_scale_y(frame, y0)),
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
      glyph_lab_scale_icon_coord(&frame->size, r));
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
      glyph_lab_scale_icon_coord(&frame->size, r));
}

static GColor weather_subtle_color(const ColorPalette* palette) {
  if (!palette || helper_color_equal(palette->background, GColorWhite)) {
    return GColorDarkGray;
  }

  return GColorLightGray;
}

static GColor weather_clear_ring_color(const ColorPalette* palette) {
  return weather_subtle_color(palette);
}

static GColor weather_clear_fill_color(const ColorPalette* palette) {
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
      return PBL_IF_COLOR_ELSE(GColorVividCerulean, fallback);
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
  GColor sun_fill = is_outline ? palette->background : color;

  graphics_context_set_fill_color(ctx, sun_fill);
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
  GColor cloud_fill = is_filled ? color : palette->background;
  GColor left_cloud_debug = PBL_IF_COLOR_ELSE(GColorRed, color);
  GColor center_cloud_debug = PBL_IF_COLOR_ELSE(GColorIslamicGreen, color);
  GColor right_cloud_debug = PBL_IF_COLOR_ELSE(GColorBlueMoon, color);
  GColor cloud_body_debug = PBL_IF_COLOR_ELSE(GColorChromeYellow, color);
  GColor cloud_base_debug = PBL_IF_COLOR_ELSE(GColorMagenta, color);

  graphics_context_set_stroke_color(ctx, color);
  graphics_context_set_fill_color(ctx, cloud_fill);
  graphics_context_set_stroke_width(ctx, 2);

  weather_draw_circle(
      ctx,
      frame,
      left_cloud_center_x,
      left_cloud_center_y,
      left_cloud_radius);
  weather_draw_circle(
      ctx,
      frame,
      center_cloud_center_x,
      center_cloud_center_y,
      center_cloud_radius);
  weather_draw_circle(
      ctx,
      frame,
      right_cloud_center_x,
      right_cloud_center_y,
      right_cloud_radius);

  graphics_context_set_fill_color(ctx, left_cloud_debug);
  weather_fill_circle(ctx, frame, left_cloud_center_x, left_cloud_center_y, 3);
  graphics_context_set_fill_color(ctx, center_cloud_debug);
  weather_fill_circle(
      ctx,
      frame,
      center_cloud_center_x,
      center_cloud_center_y,
      4);
  graphics_context_set_fill_color(ctx, right_cloud_debug);
  weather_fill_circle(
      ctx,
      frame,
      right_cloud_center_x,
      right_cloud_center_y,
      3);
  graphics_context_set_fill_color(ctx, cloud_body_debug);
  weather_fill_rect_from_corners(
      ctx,
      frame,
      cloud_fill_left,
      cloud_fill_top,
      cloud_fill_right,
      cloud_fill_bottom);
  graphics_context_set_stroke_color(ctx, cloud_base_debug);
  weather_line(
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
  draw_weather_filled_cloud(
      ctx,
      &cloud_frame,
      palette,
      WEATHER_ICON_PARTLY_CLOUDY,
      true);
}

static void draw_weather_fog_icon(
    GContext* ctx,
    const GRect* frame,
    const ColorPalette* palette) {
  graphics_context_set_stroke_color(
      ctx,
      weather_color_for_kind(WEATHER_ICON_FOG, palette));
  graphics_context_set_stroke_width(ctx, 2);
  weather_line(ctx, frame, 5, 10, 23, 10);
  weather_line(ctx, frame, 2, 15, 26, 15);
  weather_line(ctx, frame, 7, 20, 21, 20);
}

static void draw_weather_drizzle_icon(
    GContext* ctx,
    const GRect* frame,
    const ColorPalette* palette) {
  graphics_context_set_stroke_color(
      ctx,
      weather_color_for_kind(WEATHER_ICON_DRIZZLE, palette));
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
      weather_color_for_kind(
          heavy ? WEATHER_ICON_HEAVY_RAIN : WEATHER_ICON_RAIN,
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
  WeatherIconKind kind = heavy ?
      WEATHER_ICON_HEAVY_SHOWERS : WEATHER_ICON_SHOWERS;
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
      palette->primary_text);

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
  draw_unavailable_slash(ctx, frame, palette->primary_text);
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

void glyph_lab_draw_climate_icon(
    GContext* ctx,
    const GRect* frame,
    int16_t weather_condition,
    const ColorPalette* palette) {
  if (!ctx || !frame || !palette) {
    return;
  }

  switch (get_weather_icon_kind(weather_condition)) {
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
      draw_weather_cloud(ctx, frame, palette, WEATHER_ICON_CLOUD);
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
