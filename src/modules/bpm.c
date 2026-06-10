#ifdef PBL_HEALTH
#include "bpm.h"
#include "../c/ataglance.h"

static char s_bpm_buffer[ATAGLANCE_MAX_STR_LEN];
static Layer* s_bpm_icon_layer;
static TextLayer* s_bpm_layer;
static int s_bpm;
static bool s_bpm_is_available;
static const WatchfaceSurface* s_surface;

static TextLayer* create_bpm_text_layer(
    Layer* parent,
    const WatchfaceTextSubstratum* text,
    GFont font);
static GColor calculate_bpm_color(int bpm);
static void draw_bpm_icon_with_color(
    GContext* ctx,
    GColor color,
    const GSize* bounds_size);
static void draw_data_gap_slash(
    GContext* ctx,
    const GSize* bounds_size);
static void bpm_draw_scaled_line(
    GContext* ctx,
    const GSize* bounds_size,
    int16_t x0,
    int16_t y0,
    int16_t x1,
    int16_t y1);
static void bpm_icon_update_proc(Layer* layer, GContext* ctx);
static void apply_bpm_value(int bpm, bool is_available);
static void update_bpm(void);

static TextLayer* create_bpm_text_layer(
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

static GColor calculate_bpm_color(int bpm) {
  if (!s_surface || !s_surface->style.palette) {
    return GColorWhite;
  }

  const ColorPalette* palette = s_surface->style.palette;
  if (bpm <= 0) {
    return palette->unavailable_text;
  }
  if (bpm > 120) {
    return PBL_IF_COLOR_ELSE(
        GColorRed,
        gcolor_legible_over(palette->background));
  }
  if (bpm >= 100) {
    return PBL_IF_COLOR_ELSE(
        GColorMagenta,
        gcolor_legible_over(palette->background));
  }
  return palette->primary_text;
}

static void draw_bpm_icon_with_color(
    GContext* ctx,
    GColor color,
    const GSize* bounds_size) {
  if (!ctx || !bounds_size) {
    return;
  }

  graphics_context_set_stroke_color(ctx, color);
  graphics_context_set_stroke_width(ctx, 2);
  bpm_draw_scaled_line(ctx, bounds_size, 3, 15, 8, 15);
  bpm_draw_scaled_line(ctx, bounds_size, 8, 15, 11, 8);
  bpm_draw_scaled_line(ctx, bounds_size, 11, 8, 15, 22);
  bpm_draw_scaled_line(ctx, bounds_size, 15, 22, 19, 12);
  bpm_draw_scaled_line(ctx, bounds_size, 19, 12, 24, 12);
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
  bpm_draw_scaled_line(ctx, bounds_size, 5, 3, 24, 26);
}

static void bpm_draw_scaled_line(
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

static void bpm_icon_update_proc(Layer* layer, GContext* ctx) {
  if (!layer || !ctx || !s_surface || !s_surface->style.palette) {
    return;
  }

  GRect bounds = layer_get_bounds(layer);

  graphics_context_set_fill_color(
      ctx,
      s_surface->style.palette->background);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  draw_bpm_icon_with_color(
      ctx,
      calculate_bpm_color(s_bpm),
      &bounds.size);

  if (!s_bpm_is_available) {
    draw_data_gap_slash(ctx, &bounds.size);
  }
}

static void apply_bpm_value(int bpm, bool is_available) {
  if (!s_bpm_layer || !s_surface || !s_surface->style.palette) {
    return;
  }

  GColor text_color = s_surface->style.palette->unavailable_text;
  s_bpm = bpm;
  s_bpm_is_available = is_available && bpm > 0;

  if (s_bpm_is_available) {
    snprintf(s_bpm_buffer, ATAGLANCE_MAX_STR_LEN, "%d", s_bpm);
    text_color = calculate_bpm_color(s_bpm);
  } else {
    snprintf(
        s_bpm_buffer,
        ATAGLANCE_MAX_STR_LEN,
        "%s",
        WATCHFACE_UNAVAILABLE_TEXT);
  }

  layout_update_text_layer(s_bpm_layer, s_bpm_buffer, text_color);

  if (s_bpm_icon_layer) {
    layer_mark_dirty(s_bpm_icon_layer);
  }
}

static void update_bpm(void) {
  time_t now = time(NULL);
  HealthServiceAccessibilityMask hr_mask =
      health_service_metric_accessible(
          HealthMetricHeartRateBPM,
          now,
          now);

  int bpm = 0;
  bool is_available = false;

  if (hr_mask & HealthServiceAccessibilityMaskAvailable) {
    bpm = (int) health_service_peek_current_value(
        HealthMetricHeartRateBPM);
    is_available = bpm > 0;
  }

  apply_bpm_value(bpm, is_available);
}

bool bpm_module_create(
    Layer* root,
    const WatchfaceSurface* surface) {
  if (!root || !surface || !surface->style.palette) {
    return false;
  }

  const WatchfaceTextSubstratum* text = &surface->bpm.text;
  const WatchfaceIconSubstratum* icon = &surface->bpm.icon;
  s_surface = surface;
  s_bpm = 0;
  s_bpm_is_available = false;

  s_bpm_layer = create_bpm_text_layer(
      root,
      text,
      surface->style.fonts[text->font_role]);

  if (!s_bpm_layer) {
    APP_LOG(APP_LOG_LEVEL_ERROR,
            "Failed to create BPM text layer");
    return false;
  }

  if (icon->is_enabled) {
    s_bpm_icon_layer = layer_create(icon->frame);
  }
  if (s_bpm_icon_layer) {
    layer_set_update_proc(s_bpm_icon_layer, bpm_icon_update_proc);
    layer_add_child(root, s_bpm_icon_layer);
  }

  return true;
}

void bpm_module_destroy(void) {
  if (s_bpm_icon_layer) {
    layer_destroy(s_bpm_icon_layer);
    s_bpm_icon_layer = NULL;
  }
  if (s_bpm_layer) {
    text_layer_destroy(s_bpm_layer);
    s_bpm_layer = NULL;
  }

  s_bpm_buffer[0] = '\0';
  s_surface = NULL;
}

void bpm_module_refresh(const WatchfaceSurface* surface) {
  if (surface) {
    s_surface = surface;
  }

  update_bpm();
}

void bpm_module_handle_event(HealthEventType event) {
  if (event == HealthEventSignificantUpdate ||
      event == HealthEventHeartRateUpdate) {
    update_bpm();
  }
}

#ifdef DEBUG_ATAGLANCE
void bpm_module_debug_set_value(int bpm) {
  apply_bpm_value(bpm, bpm > 0);
}
#endif

#else

#include "bpm.h"

bool bpm_module_create(
    Layer* root,
    const WatchfaceSurface* surface) {
  (void)root;
  (void)surface;
  return true;
}

void bpm_module_destroy(void) {
}

void bpm_module_refresh(const WatchfaceSurface* surface) {
  (void)surface;
}

#endif
