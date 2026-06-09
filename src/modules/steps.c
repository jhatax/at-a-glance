#if defined(PBL_HEALTH)
#include "steps.h"
#include "../c/ataglance.h"

static char s_steps_buffer[ATAGLANCE_MAX_STR_LEN];
static Layer* s_steps_icon_layer;
static TextLayer* s_steps_layer;
static bool s_steps_is_available;
static const WatchfaceSurface* s_surface;

static TextLayer* create_steps_text_layer(
    Layer* parent,
    const WatchfaceTextSubstratum* text,
    GFont font);
static void draw_data_gap_slash(
    GContext* ctx,
    const GSize* bounds_size);
static void steps_draw_scaled_line(
    GContext* ctx,
    const GSize* bounds_size,
    int16_t x0,
    int16_t y0,
    int16_t x1,
    int16_t y1);
static void steps_icon_update_proc(Layer* layer, GContext* ctx);
static void apply_steps_value(int steps, bool is_available);
static void update_steps(void);

static TextLayer* create_steps_text_layer(
    Layer* parent,
    const WatchfaceTextSubstratum* text,
    GFont font) {
  if (!parent || !text) {
    return NULL;
  }

  TextLayer* layer = text_layer_create(text->frame);
  if (!layer) {
    return NULL;
  }

  text_layer_set_background_color(layer, GColorClear);
  text_layer_set_font(layer, font);
  text_layer_set_text_alignment(layer, text->alignment);
  layer_add_child(parent, text_layer_get_layer(layer));
  return layer;
}

static void draw_data_gap_slash(
    GContext* ctx,
    const GSize* bounds_size) {
  if (!s_surface || !s_surface->style.palette) {
    return;
  }

  graphics_context_set_stroke_width(ctx, 3);
  graphics_context_set_stroke_color(
      ctx,
      gcolor_legible_over(s_surface->style.palette->background));
  steps_draw_scaled_line(ctx, bounds_size, 5, 3, 24, 26);
}

static void steps_draw_scaled_line(
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
      layout_scaled_icon_point(bounds_size, x0, y0),
      layout_scaled_icon_point(bounds_size, x1, y1));
}

static void steps_icon_update_proc(Layer* layer, GContext* ctx) {
  if (!layer || !ctx || !s_surface || !s_surface->style.palette) {
    return;
  }

  const ColorPalette* palette = s_surface->style.palette;
  GRect bounds = layer_get_bounds(layer);

  graphics_context_set_fill_color(ctx, palette->background);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  GColor steps_icon_color = s_steps_is_available ?
      palette->steps_icon : palette->unavailable_text;

  graphics_context_set_fill_color(ctx, steps_icon_color);

  graphics_fill_circle(
      ctx,
      layout_scaled_icon_point(&bounds.size, 14, 9),
      layout_scale_icon_coord(&bounds.size, 7));

  graphics_context_set_fill_color(ctx, palette->background);
  graphics_fill_rect(
      ctx,
      GRect(layout_scale_icon_x(&bounds.size, 6),
            layout_scale_icon_y(&bounds.size, 15),
            layout_scale_icon_x(&bounds.size, 16),
            layout_scale_icon_y(&bounds.size, 4)),
      0,
      GCornerNone);

  graphics_context_set_fill_color(ctx, steps_icon_color);
  graphics_fill_circle(
      ctx,
      layout_scaled_icon_point(&bounds.size, 14, 22),
      layout_scale_icon_coord(&bounds.size, 4));

  if (!s_steps_is_available) {
    draw_data_gap_slash(ctx, &bounds.size);
  }
}

static void apply_steps_value(int steps, bool is_available) {
  if (!s_steps_layer || !s_surface || !s_surface->style.palette) {
    return;
  }

  const ColorPalette* palette = s_surface->style.palette;
  s_steps_is_available = is_available && steps >= 0;
  GColor text_color = s_steps_is_available ?
      palette->primary_text : palette->unavailable_text;

  if (s_steps_is_available) {
    snprintf(s_steps_buffer, ATAGLANCE_MAX_STR_LEN, "%d", steps);
  } else {
    snprintf(
        s_steps_buffer,
        ATAGLANCE_MAX_STR_LEN,
        "%s",
        WATCHFACE_UNAVAILABLE_TEXT);
  }

  layout_update_text_layer(s_steps_layer, s_steps_buffer, text_color);

  if (s_steps_icon_layer) {
    layer_mark_dirty(s_steps_icon_layer);
  }
}

static void update_steps(void) {
  HealthServiceAccessibilityMask steps_mask =
      health_service_metric_accessible(
          HealthMetricStepCount,
          time_start_of_today(),
          time(NULL));

  int steps = 0;
  bool is_available = false;

  if (steps_mask & HealthServiceAccessibilityMaskAvailable) {
    HealthValue health_steps = health_service_sum_today(
        HealthMetricStepCount);
    if (health_steps >= 0) {
      steps = (int)health_steps;
      is_available = true;
    }
  }

  apply_steps_value(steps, is_available);
}

bool steps_module_create(
    Layer* root,
    const WatchfaceSurface* surface) {
  if (!root || !surface || !surface->style.palette) {
    return false;
  }

  const WatchfaceTextSubstratum* text = &surface->steps.text;
  const WatchfaceIconSubstratum* icon = &surface->steps.icon;
  s_surface = surface;
  s_steps_is_available = false;

  s_steps_layer = create_steps_text_layer(
      root,
      text,
      surface->style.fonts[text->font_role]);

  if (!s_steps_layer) {
    APP_LOG(APP_LOG_LEVEL_ERROR,
            "Failed to create steps text layer");
    return false;
  }

  if (icon->is_enabled) {
    s_steps_icon_layer = layer_create(icon->frame);
  }
  if (s_steps_icon_layer) {
    layer_set_update_proc(s_steps_icon_layer, steps_icon_update_proc);
    layer_add_child(root, s_steps_icon_layer);
  }

  return true;
}

void steps_module_destroy(void) {
  if (s_steps_icon_layer) {
    layer_destroy(s_steps_icon_layer);
    s_steps_icon_layer = NULL;
  }
  if (s_steps_layer) {
    text_layer_destroy(s_steps_layer);
    s_steps_layer = NULL;
  }

  s_steps_buffer[0] = '\0';
  s_surface = NULL;
}

void steps_module_refresh(const WatchfaceSurface* surface) {
  if (surface) {
    s_surface = surface;
  }

  update_steps();
}

void steps_module_handle_event(HealthEventType event) {
  if (event == HealthEventSignificantUpdate ||
      event == HealthEventMovementUpdate) {
    update_steps();
  }
}

#ifdef DEBUG_ATAGLANCE
void steps_module_debug_set_value(int steps) {
  apply_steps_value(steps, steps >= 0);
}
#endif

#else

#include "steps.h"

bool steps_module_create(
    Layer* root,
    const WatchfaceSurface* surface) {
  (void)root;
  (void)surface;
  return true;
}

void steps_module_destroy(void) {
}

void steps_module_refresh(const WatchfaceSurface* surface) {
  (void)surface;
}

#endif
