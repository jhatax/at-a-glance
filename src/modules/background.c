#include "background.h"

static Layer* s_background_layer = NULL;
static const WatchfaceSurface* s_surface = NULL;

static void background_layer_update_proc(Layer* layer, GContext* ctx);

static void background_layer_update_proc(Layer* layer, GContext* ctx) {
  if (!layer || !ctx || !s_surface || !s_surface->style.palette) {
    return;
  }

  GRect bounds = layer_get_bounds(layer);
  graphics_context_set_fill_color(
      ctx,
      s_surface->style.palette->background_layer_background);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  if (!s_surface->background.line_enabled) {
    return;
  }

  graphics_context_set_stroke_width(ctx, s_surface->background.line_height);
  graphics_context_set_stroke_color(
      ctx,
      s_surface->style.palette->background_layer_line);
  graphics_draw_line(
      ctx,
      GPoint(s_surface->background.line_x, s_surface->background.line_y),
      GPoint(
          s_surface->background.line_x + s_surface->background.line_width,
          s_surface->background.line_y));
}

bool background_module_create(
    Layer* root,
    const WatchfaceSurface* surface) {
  if (!root || !surface || !surface->style.palette) {
    return false;
  }

  s_background_layer = layer_create(surface->background.frame);
  if (!s_background_layer) {
    APP_LOG(APP_LOG_LEVEL_ERROR,
            "Failed to create background layer");
    return false;
  }

  s_surface = surface;
  layer_set_update_proc(s_background_layer, background_layer_update_proc);
  layer_add_child(root, s_background_layer);
  return true;
}

void background_module_destroy(void) {
  if (s_background_layer) {
    layer_destroy(s_background_layer);
    s_background_layer = NULL;
  }

  s_surface = NULL;
}

void background_module_refresh(void) {
  if (!s_background_layer || !s_surface || !s_surface->style.palette) {
    return;
  }

  layer_mark_dirty(s_background_layer);
}
