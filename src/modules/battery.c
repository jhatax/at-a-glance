#include "battery.h"
#include "substratum_renderer.h"
#include "../c/ataglance.h"

static char s_battery_buffer[ATAGLANCE_MAX_STR_LEN];

static Layer* s_battery_icon_layer;
static TextLayer* s_battery_layer;
static BatteryChargeState s_battery_state;

static const WatchfaceSurface* s_surface;

static GColor calculate_battery_color(void);
static void draw_battery_charging_bolt(
    GContext* ctx,
    const GSize* bounds_size,
    GColor color);
static void battery_draw_scaled_line(
    GContext* ctx,
    const GSize* bounds_size,
    int16_t x0,
    int16_t y0,
    int16_t x1,
    int16_t y1);
static void battery_icon_update_proc(Layer* layer, GContext* ctx);
static void update_battery(void);

static GColor calculate_battery_color(void) {
  if (!s_surface || !s_surface->style.palette) {
    return GColorWhite;
  }

  const ColorPalette* palette = s_surface->style.palette;
  int percent = s_battery_state.charge_percent;

  if (s_battery_state.is_charging) {
    return PBL_IF_COLOR_ELSE(
        GColorJaegerGreen,
        gcolor_legible_over(palette->background));
  }

  if (percent > 50) {
    return palette->primary_text;
  }
  if (percent > 20) {
    return PBL_IF_COLOR_ELSE(
        GColorRajah,
        gcolor_legible_over(palette->background));
  }
  return PBL_IF_COLOR_ELSE(
      GColorRed,
      gcolor_legible_over(palette->background));
}

static void draw_battery_charging_bolt(
    GContext* ctx,
    const GSize* bounds_size,
    GColor color) {
  graphics_context_set_stroke_color(ctx, s_surface->style.palette->background);
  graphics_context_set_stroke_width(ctx, 1);
  battery_draw_scaled_line(ctx, bounds_size, 17, 3, 10, 14);
  battery_draw_scaled_line(ctx, bounds_size, 10, 14, 16, 14);
  battery_draw_scaled_line(ctx, bounds_size, 16, 14, 11, 25);
  battery_draw_scaled_line(ctx, bounds_size, 11, 25, 22, 11);
  battery_draw_scaled_line(ctx, bounds_size, 22, 11, 16, 11);
  battery_draw_scaled_line(ctx, bounds_size, 16, 11, 17, 3);

  graphics_context_set_stroke_color(ctx, color);
  graphics_context_set_stroke_width(ctx, 2);
  battery_draw_scaled_line(ctx, bounds_size, 17, 3, 10, 14);
  battery_draw_scaled_line(ctx, bounds_size, 10, 14, 16, 14);
  battery_draw_scaled_line(ctx, bounds_size, 16, 14, 11, 25);
  battery_draw_scaled_line(ctx, bounds_size, 11, 25, 22, 11);
  battery_draw_scaled_line(ctx, bounds_size, 22, 11, 16, 11);
  battery_draw_scaled_line(ctx, bounds_size, 16, 11, 17, 3);
}

static void battery_draw_scaled_line(
    GContext* ctx,
    const GSize* bounds_size,
    int16_t x0,
    int16_t y0,
    int16_t x1,
    int16_t y1) {
  if (!ctx || !bounds_size) {
    return;
  }

  graphics_draw_line(
      ctx,
      substratum_renderer_scale_icon_point(bounds_size, x0, y0),
      substratum_renderer_scale_icon_point(bounds_size, x1, y1));
}

static void battery_icon_update_proc(Layer* layer, GContext* ctx) {
  if (!layer || !ctx || !s_surface || !s_surface->style.palette) {
    return;
  }

  const ColorPalette* palette = s_surface->style.palette;
  GRect bounds = layer_get_bounds(layer);
  const int stroke_width = 2;
  int percent = s_battery_state.charge_percent;
  GColor draw_color = calculate_battery_color();
  int fill_w = (percent * 18) / 100;

  graphics_context_set_fill_color(ctx, palette->background);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  graphics_context_set_stroke_color(ctx, draw_color);
  graphics_context_set_fill_color(ctx, draw_color);
  graphics_context_set_stroke_width(ctx, stroke_width);

  graphics_draw_rect(
      ctx,
      GRect(substratum_renderer_scale_icon_x(&bounds.size, 2),
            substratum_renderer_scale_icon_y(&bounds.size, 8),
            substratum_renderer_scale_icon_x(&bounds.size, 22),
            substratum_renderer_scale_icon_y(&bounds.size, 13)));
  graphics_fill_rect(
      ctx,
      GRect(substratum_renderer_scale_icon_x(&bounds.size, 24),
            substratum_renderer_scale_icon_y(&bounds.size, 12),
            substratum_renderer_scale_icon_x(&bounds.size, 2),
            substratum_renderer_scale_icon_y(&bounds.size, 5)),
      0,
      GCornerNone);

  if (fill_w < 1) {
    fill_w = 1;
  }

  graphics_fill_rect(
      ctx,
      GRect(substratum_renderer_scale_icon_x(&bounds.size, 5),
            substratum_renderer_scale_icon_y(&bounds.size, 11),
            substratum_renderer_scale_icon_x(&bounds.size, fill_w),
            substratum_renderer_scale_icon_y(&bounds.size, 7)),
      0,
      GCornerNone);

  if (s_battery_state.is_charging) {
    draw_battery_charging_bolt(ctx, &bounds.size, draw_color);
  }
}

static void update_battery(void) {
  if (!s_battery_layer || !s_surface || !s_surface->style.palette) {
    return;
  }

  snprintf(
      s_battery_buffer,
      ATAGLANCE_MAX_STR_LEN,
      "%d%%",
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
  s_battery_icon_layer = substratum_renderer_create_icon_layer(
      root,
      icon,
      battery_icon_update_proc);

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
  s_surface = NULL;
}

void battery_module_refresh(const WatchfaceSurface* surface) {
  if (surface) {
    s_surface = surface;
  }

  s_battery_state = battery_state_service_peek();
  update_battery();
}
