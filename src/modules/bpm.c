#ifdef PBL_HEALTH
#include "bpm.h"
#include "helper.h"
#include "substratum_renderer.h"
#include "../c/ataglance.h"

#define BPM_INVALID -1

static char s_bpm_buffer[ATAGLANCE_MAX_STR_LEN] = {0};
static Layer* s_bpm_icon_layer = NULL;
static TextLayer* s_bpm_layer = NULL;
static int s_bpm = BPM_INVALID;
static bool s_bpm_is_available = false;
static const WatchfaceSurface* s_surface = NULL;
static const ColorPalette* s_palette = NULL;
#if DEBUG_ATAGLANCE
static bool s_debug_bpm_is_set = false;
static int s_debug_bpm = BPM_INVALID;
#endif

static GColor calculate_bpm_color(int bpm);
static void draw_bpm_icon_with_color(
    GContext* ctx,
    GColor color,
    const GSize* bounds_size);
static void bpm_icon_update_proc(Layer* layer, GContext* ctx);
static void apply_bpm_value(int bpm, bool is_available);
static void update_bpm(void);

static GColor calculate_bpm_color(int bpm) {
  if (!s_surface || !s_palette) {
    return GColorWhite;
  }

  const ColorPalette* palette = s_palette;
  if (bpm <= 0) {
    return palette->unavailable_text;
  }
  if (bpm > 120) {
    return PBL_IF_COLOR_ELSE(
        s_surface->style.is_light_mode ? GColorBulgarianRose : GColorOrange,
        palette->primary_text);
  }
  if (bpm >= 100) {
    return PBL_IF_COLOR_ELSE(
        s_surface->style.is_light_mode ?
            GColorWindsorTan : GColorChromeYellow,
        palette->primary_text);
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

  int16_t frame_min = HELPER_MIN(bounds_size->w, bounds_size->h);
  int16_t waveform_stroke_width = SUBSTRATUM_RENDERER_ICON_STROKE_WIDTH(
      frame_min);

  graphics_context_set_stroke_color(ctx, color);
  // BPM icon contract: a 1px reference box anchors the waveform visually on
  // compact displays so the pulse does not collapse into a floating blob.
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_rect(
      ctx,
      GRect(substratum_renderer_scale_icon_x(bounds_size, 3),
            substratum_renderer_scale_icon_y(bounds_size, 4),
            substratum_renderer_scale_icon_x(bounds_size, 25) -
                substratum_renderer_scale_icon_x(bounds_size, 3),
            substratum_renderer_scale_icon_y(bounds_size, 23) -
                substratum_renderer_scale_icon_y(bounds_size, 4)));

  graphics_context_set_stroke_width(ctx, waveform_stroke_width);
  substratum_renderer_draw_scaled_line(ctx, bounds_size, 2, 16, 7, 16);
  substratum_renderer_draw_scaled_line(ctx, bounds_size, 7, 16, 10, 9);
  substratum_renderer_draw_scaled_line(ctx, bounds_size, 10, 9, 14, 23);
  substratum_renderer_draw_scaled_line(ctx, bounds_size, 14, 23, 18, 10);
  substratum_renderer_draw_scaled_line(ctx, bounds_size, 18, 10, 21, 16);
  substratum_renderer_draw_scaled_line(ctx, bounds_size, 21, 16, 26, 16);
}

static void bpm_icon_update_proc(Layer* layer, GContext* ctx) {
  if (!layer || !ctx || !s_surface || !s_palette) {
    return;
  }

  GRect bounds = layer_get_bounds(layer);

  graphics_context_set_fill_color(
      ctx,
      s_palette->background);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  draw_bpm_icon_with_color(
      ctx,
      calculate_bpm_color(s_bpm),
      &bounds.size);

  if (!s_bpm_is_available) {
    substratum_renderer_draw_unavailable_slash(
        ctx,
        &bounds.size,
        s_palette->primary_text);
  }
}

static void apply_bpm_value(int bpm, bool is_available) {
  if (!s_bpm_layer || !s_surface || !s_palette) {
    return;
  }

  GColor text_color = s_palette->unavailable_text;
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

  substratum_renderer_update_text_layer(s_bpm_layer, s_bpm_buffer, text_color);

  if (s_bpm_icon_layer) {
    layer_mark_dirty(s_bpm_icon_layer);
  }
}

static void update_bpm(void) {
#if DEBUG_ATAGLANCE
  if (s_debug_bpm_is_set) {
    apply_bpm_value(s_debug_bpm, s_debug_bpm > 0);
    s_debug_bpm_is_set = false;
    s_debug_bpm = BPM_INVALID;
    return;
  }
#endif

  time_t now = time(NULL);
  HealthServiceAccessibilityMask hr_mask =
      health_service_metric_accessible(
          HealthMetricHeartRateBPM,
          now,
          now);

  int bpm = BPM_INVALID;
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

  s_bpm_layer = substratum_renderer_create_text_layer(
      root,
      text,
      surface->style.fonts[text->font_role]);

  if (!s_bpm_layer) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to create BPM text layer");
    return false;
  }

  s_bpm = BPM_INVALID;
  s_bpm_is_available = false;
#if DEBUG_ATAGLANCE
  s_debug_bpm_is_set = false;
  s_debug_bpm = BPM_INVALID;
#endif

  s_surface = surface;
  s_palette = surface->style.palette;
  if (icon->is_enabled) {
    s_bpm_icon_layer = substratum_renderer_create_icon_layer(root, icon, bpm_icon_update_proc);
    if (!s_bpm_icon_layer) {
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Failed to create BPM icon");
    }
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
  s_bpm = BPM_INVALID;
  s_bpm_is_available = false;
  s_palette = NULL;
  s_surface = NULL;
#if DEBUG_ATAGLANCE
  s_debug_bpm_is_set = false;
  s_debug_bpm = BPM_INVALID;
  #endif
}

void bpm_module_refresh(void) {
  if (!s_surface || !s_surface->style.palette) {
    return;
  }

  s_palette = s_surface->style.palette;
  update_bpm();
}

#if DEBUG_ATAGLANCE
void bpm_module_debug_set_bpm(int bpm) {
  s_debug_bpm = bpm;
  s_debug_bpm_is_set = true;
}

void bpm_module_debug_clear_bpm(void) {
  s_debug_bpm_is_set = false;
  s_debug_bpm = BPM_INVALID;
}
#endif

#endif
