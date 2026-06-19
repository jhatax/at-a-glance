#include "battery.h"

#include "helper.h"
#include "substratum_renderer.h"

static BatteryChargeState s_battery_state = {0};
static const WatchfaceSurface* s_surface = NULL;
static const ColorPalette* s_palette = NULL;

static GColor calculate_battery_color(void);

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

static Layer* s_battery_track_layer = NULL;
static Layer* s_battery_bolt_layer = NULL;

static void draw_battery_track(
    GContext* ctx,
    const GRect* bounds,
    const GRect* track,
    GColor color);
static void draw_battery_fill(
    GContext* ctx,
    const GRect* track,
    const GRect* fill,
    int16_t fill_width,
    GColor color);
static void battery_track_update_proc(Layer* layer, GContext* ctx);
static void battery_bolt_update_proc(Layer* layer, GContext* ctx);
static void update_battery_track(void);

static void draw_battery_track(
    GContext* ctx,
    const GRect* bounds,
    const GRect* track,
    GColor color) {
  if (!ctx || !bounds || !track) {
    return;
  }

  graphics_context_set_stroke_color(ctx, color);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_rect(
      ctx,
      GRect(bounds->origin.x, bounds->origin.y, track->size.w, track->size.h));
}

static void draw_battery_fill(
    GContext* ctx,
    const GRect* track,
    const GRect* fill,
    int16_t fill_width,
    GColor color) {
  if (!ctx || !track || !fill || fill_width < 1) {
    return;
  }

  graphics_context_set_fill_color(ctx, color);
  graphics_fill_rect(
      ctx,
      GRect(fill->origin.x - track->origin.x,
            fill->origin.y - track->origin.y,
            fill_width,
            fill->size.h),
      0,
      GCornerNone);
}

static void battery_track_update_proc(Layer* layer, GContext* ctx) {
  if (!layer || !ctx || !s_surface || !s_palette) {
    return;
  }

  const GRect* track = &s_surface->battery.track;
  const GRect* fill = &s_surface->battery.fill;
  GRect bounds = layer_get_bounds(layer);
  int16_t fill_width = (s_battery_state.charge_percent * fill->size.w) / 100;
  GColor fill_color = calculate_battery_color();

  if (fill_width < 1) {
    fill_width = 1;
  }

  graphics_context_set_fill_color(ctx, s_palette->background);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  draw_battery_track(ctx, &bounds, track, s_palette->primary_text);
  draw_battery_fill(ctx, track, fill, fill_width, fill_color);
}

static void battery_bolt_update_proc(Layer* layer, GContext* ctx) {
  if (!layer || !ctx || !s_surface || !s_palette) {
    return;
  }

  GRect bounds = layer_get_bounds(layer);
  substratum_renderer_draw_filled_bolt_in_frame(
      ctx,
      &bounds,
      calculate_battery_color());
}

static void update_battery_track(void) {
  if (!s_surface || !s_palette || !s_battery_track_layer) {
    return;
  }

  layer_mark_dirty(s_battery_track_layer);
  if (s_battery_bolt_layer) {
    layer_set_hidden(
        s_battery_bolt_layer,
        !s_battery_state.is_charging);
    layer_mark_dirty(s_battery_bolt_layer);
  }
}

bool battery_module_create(
    Layer* root,
    const WatchfaceSurface* surface) {
  if (!root || !surface || !surface->style.palette) {
    return false;
  }

  s_battery_track_layer = layer_create(surface->battery.track);
  if (!s_battery_track_layer) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to create battery track layer");
    return false;
  }

  s_battery_state = battery_state_service_peek();
  s_surface = surface;
  s_palette = surface->style.palette;
  layer_set_update_proc(s_battery_track_layer, battery_track_update_proc);
  layer_add_child(root, s_battery_track_layer);

  s_battery_bolt_layer = layer_create(surface->battery.bolt);
  if (s_battery_bolt_layer) {
    layer_set_update_proc(s_battery_bolt_layer, battery_bolt_update_proc);
    layer_add_child(root, s_battery_bolt_layer);
    layer_set_hidden(s_battery_bolt_layer, !s_battery_state.is_charging);
  }

  return true;
}

void battery_module_destroy(void) {
  if (s_battery_bolt_layer) {
    layer_destroy(s_battery_bolt_layer);
    s_battery_bolt_layer = NULL;
  }

  if (s_battery_track_layer) {
    layer_destroy(s_battery_track_layer);
    s_battery_track_layer = NULL;
  }

  s_palette = NULL;
  s_surface = NULL;
}

void battery_module_refresh(void) {
  if (!s_surface || !s_surface->style.palette) {
    return;
  }

  s_palette = s_surface->style.palette;
  s_battery_state = battery_state_service_peek();
  update_battery_track();
}
