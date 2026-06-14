#include "battery.h"
#include "helper.h"
#include "substratum_renderer.h"
#include "../c/ataglance.h"

static char s_battery_buffer[ATAGLANCE_MAX_STR_LEN] = {0};

static Layer* s_battery_icon_layer = NULL;
static TextLayer* s_battery_layer = NULL;
static BatteryChargeState s_battery_state = {0};

static const WatchfaceSurface* s_surface = NULL;
static const ColorPalette* s_palette = NULL;

static GColor calculate_battery_color(void);
static GRect battery_scaled_rect_from_corners(
    const GSize* size,
    int16_t x0,
    int16_t y0,
    int16_t x1,
    int16_t y1);
static void draw_battery_charging_bolt(
    GContext* ctx,
    const GSize* bounds_size,
    GColor color);
static void battery_icon_update_proc(Layer* layer, GContext* ctx);
static void update_battery(void);

static GColor calculate_battery_color(void) {
  if (!s_surface || !s_palette) {
    return GColorWhite;
  }

  int percent = s_battery_state.charge_percent;

  if (s_battery_state.is_charging) {
    return PBL_IF_COLOR_ELSE(GColorIslamicGreen, s_palette->primary_text);
  }

  if (percent > 50) {
    return s_palette->primary_text;
  }
  if (percent > 20) {
    return PBL_IF_COLOR_ELSE(
        s_surface->style.is_light_mode ? GColorWindsorTan : GColorRajah,
        s_palette->primary_text);
  }

  return PBL_IF_COLOR_ELSE(
      s_surface->style.is_light_mode ? GColorBulgarianRose : GColorRed,
      s_palette->primary_text);
}

static GRect battery_scaled_rect_from_corners(
    const GSize* size,
    int16_t x0,
    int16_t y0,
    int16_t x1,
    int16_t y1) {
  if (!size) {
    return GRectZero;
  }

  int16_t left = substratum_renderer_scale_icon_x(size, x0);
  int16_t top = substratum_renderer_scale_icon_y(size, y0);
  int16_t right = substratum_renderer_scale_icon_x(size, x1);
  int16_t bottom = substratum_renderer_scale_icon_y(size, y1);

  return GRect(left, top, right - left, bottom - top);
}

static void draw_battery_charging_bolt(
    GContext* ctx,
    const GSize* bounds_size,
    GColor color) {
  int16_t frame_min = HELPER_MIN(bounds_size->w, bounds_size->h);
  int16_t bolt_stroke_width = SUBSTRATUM_RENDERER_ICON_STROKE_WIDTH(
      frame_min);
  // Charging bolt contract: stroke-only 7-point polygon in 28x28 design
  // space. The top point, left jog, bottom point, and right jog define the
  // silhouette; the final point returns inside the right edge so the closing
  // segment reads as a bolt instead of a filled wedge after scaling.
  static const GPoint bolt_points[] = {
    {14, 0},
    {5, 11},
    {10, 11},
    {5, 27},
    {17, 13},
    {23, 13},
    {14, 5},
  };

  substratum_renderer_draw_scaled_polygon_outline(
      ctx,
      bounds_size,
      bolt_points,
      ARRAY_LENGTH(bolt_points),
      s_palette->primary_text,
      s_palette->background,
      bolt_stroke_width,
      true);
}

static void battery_icon_update_proc(Layer* layer, GContext* ctx) {
  if (!layer || !ctx || !s_surface || !s_palette) {
    return;
  }

  GRect bounds = layer_get_bounds(layer);
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
  int percent = s_battery_state.charge_percent;
  GColor draw_color = calculate_battery_color();
  int16_t fill_width = (percent * fill_width_max) / 100;

  graphics_context_set_fill_color(ctx, s_palette->background);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  graphics_context_set_stroke_color(ctx, draw_color);
  graphics_context_set_fill_color(ctx, draw_color);
  graphics_context_set_stroke_width(ctx, shell_stroke_width);

  graphics_draw_rect(
      ctx,
      battery_scaled_rect_from_corners(
          &bounds.size,
          shell_left,
          shell_top,
          shell_right,
          shell_bottom));
  graphics_fill_rect(
      ctx,
      battery_scaled_rect_from_corners(
          &bounds.size,
          nub_left,
          nub_top,
          nub_right,
          nub_bottom),
      0,
      GCornerNone);

  if (fill_width < 1) {
    fill_width = 1;
  }

  graphics_fill_rect(
      ctx,
      battery_scaled_rect_from_corners(
          &bounds.size,
          fill_left,
          fill_top,
          fill_left + fill_width,
          fill_bottom),
      0,
      GCornerNone);

  if (s_battery_state.is_charging) {
    draw_battery_charging_bolt(ctx, &bounds.size, draw_color);
  }
}

static void update_battery(void) {
  if (!s_battery_layer || !s_surface || !s_palette) {
    return;
  }

  snprintf(
      s_battery_buffer,
      ATAGLANCE_MAX_STR_LEN,
      "%d",
      s_battery_state.charge_percent);

  substratum_renderer_update_text_layer(
      s_battery_layer,
      s_battery_buffer,
      calculate_battery_color());

  if (s_battery_icon_layer) {
    layer_mark_dirty(s_battery_icon_layer);
  }
}

bool battery_module_create(
    Layer* root,
    const WatchfaceSurface* surface) {
  if (!root || !surface || !surface->style.palette) {
    return false;
  }

  const WatchfaceTextSubstratum* text = &surface->battery.text;
  const WatchfaceIconSubstratum* icon = &surface->battery.icon;
  s_battery_state = battery_state_service_peek();

  s_battery_layer = substratum_renderer_create_text_layer(
      root,
      text,
      surface->style.fonts[text->font_role]);

  if (!s_battery_layer) {
    APP_LOG(APP_LOG_LEVEL_ERROR,
            "Failed to create battery text layer");
    return false;
  }

  s_surface = surface;
  s_palette = surface->style.palette;
  if (icon->is_enabled) {
    s_battery_icon_layer = substratum_renderer_create_icon_layer(
        root,
        icon,
        battery_icon_update_proc);
}
  return true;
}

void battery_module_destroy(void) {
  if (s_battery_icon_layer) {
    layer_destroy(s_battery_icon_layer);
    s_battery_icon_layer = NULL;
  }

  if (s_battery_layer) {
    text_layer_destroy(s_battery_layer);
    s_battery_layer = NULL;
  }

  s_battery_buffer[0] = '\0';
  s_palette = NULL;
  s_surface = NULL;
}

void battery_module_refresh(void) {
  if (!s_surface || !s_surface->style.palette) {
    return;
  }

  s_palette = s_surface->style.palette;
  s_battery_state = battery_state_service_peek();
  update_battery();
}
