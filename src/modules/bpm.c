#ifdef PBL_HEALTH
#include "bpm.h"
#include "helper.h"
#include "substratum_renderer.h"

#define MAX_STR_LEN 12

#define BPM_INVALID -1
#define BPM_MIN 1
#define BPM_EXTREME 120
#define BPM_HIGH 100
#define BPM_UNINITIALIZED_COLOR GColorLightGray

#define IS_BPM_VALID(bpm) ((bpm) >= BPM_MIN)

// Decision to set the waveform_stroke_width to 2px
#define BPM_STROKE_WIDTH 2

static char s_bpm_buffer[MAX_STR_LEN] = {0};
static Layer* s_bpm_icon_layer = NULL;
static TextLayer* s_bpm_layer = NULL;
static bool s_bpm_is_valid = false;
static GColor s_bpm_color = BPM_UNINITIALIZED_COLOR;
static const ColorPalette* s_palette = NULL;

#if ATAGLANCE_DEBUG
static bool s_debug_bpm_is_set = false;
static int s_debug_bpm = BPM_INVALID;
#endif

static GColor calculate_bpm_color(int bpm);
static void draw_bpm_icon_with_color(
    GContext* ctx,
    GColor color,
    const GSize* bounds_size);
static void bpm_icon_update_proc(Layer* layer, GContext* ctx);
static void update_bpm(void);

static GColor calculate_bpm_color(int bpm) {
  if (!s_palette) {
    return GColorWhite;
  }

  if (bpm < BPM_MIN) {
    return s_palette->unavailable_text;
  }
  if (bpm >= BPM_EXTREME) {
    return PBL_IF_COLOR_ELSE(
        s_palette->is_light_mode ?
            GColorBulgarianRose : GColorOrange,
        s_palette->primary_text);
  }
  if (bpm >= BPM_HIGH) {
    return PBL_IF_COLOR_ELSE(
        s_palette->is_light_mode ?
            GColorWindsorTan : GColorChromeYellow,
        s_palette->primary_text);
  }
  return s_palette->primary_text;
}

static void draw_bpm_icon_with_color(
    GContext* ctx,
    GColor color,
    const GSize* bounds_size) {
  if (!ctx || !bounds_size) {
    return;
  }

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

  graphics_context_set_stroke_width(ctx, BPM_STROKE_WIDTH);
  substratum_renderer_draw_scaled_line(ctx, bounds_size, 2, 16, 7, 16);
  substratum_renderer_draw_scaled_line(ctx, bounds_size, 7, 16, 10, 9);
  substratum_renderer_draw_scaled_line(ctx, bounds_size, 10, 9, 14, 23);
  substratum_renderer_draw_scaled_line(ctx, bounds_size, 14, 23, 18, 10);
  substratum_renderer_draw_scaled_line(ctx, bounds_size, 18, 10, 21, 16);
  substratum_renderer_draw_scaled_line(ctx, bounds_size, 21, 16, 26, 16);
}

static void bpm_icon_update_proc(Layer* layer, GContext* ctx) {
  if (!layer || !ctx || !s_palette) {
    return;
  }

  GRect bounds = layer_get_bounds(layer);

  graphics_context_set_fill_color(ctx, s_palette->background);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  draw_bpm_icon_with_color(ctx, s_bpm_color, &bounds.size);

  if (!s_bpm_is_valid) {
    substratum_renderer_draw_unavailable_slash(
      ctx,
      &bounds.size,
      s_palette->unavailable_text);
  }
}

// Flow:
// 1. Peek the current HealthService BPM value as the baseline.
// 2. In debug builds, apply a queued debug BPM as a one-shot override.
// 3. Determine validity and color.
// 4. Render text.
// 5. Mark icon to be repainted.
//
// Debug values intentionally overwrite the peeked value only for the current
// refresh. This keeps ATAGLANCE_DEBUG as a transport/render test hook without
// making debug builds synthetic-only when no debug BPM packet is pending.
static void update_bpm(void) {
  int bpm_to_render = BPM_INVALID;

  time_t now = time(NULL);
  HealthServiceAccessibilityMask hr_mask =
      health_service_metric_accessible(
          HealthMetricHeartRateBPM,
          now,
          now);

  if (hr_mask & HealthServiceAccessibilityMaskAvailable) {
    bpm_to_render = (int) health_service_peek_current_value(
        HealthMetricHeartRateBPM);
  }

#if ATAGLANCE_DEBUG
  if (s_debug_bpm_is_set) {
    bpm_to_render = s_debug_bpm;
    s_debug_bpm_is_set = false;
    s_debug_bpm = BPM_INVALID;
  }
#endif

  // Save this state unconditionally
  s_bpm_is_valid = IS_BPM_VALID(bpm_to_render);
  s_bpm_color = calculate_bpm_color(bpm_to_render);
  if (s_bpm_is_valid) {
    snprintf(s_bpm_buffer, MAX_STR_LEN, "%d", bpm_to_render);
  } else {
    snprintf(s_bpm_buffer, MAX_STR_LEN, "%s", WATCHFACE_UNAVAILABLE_TEXT);
  }

  substratum_renderer_update_text_layer(s_bpm_layer, s_bpm_buffer, s_bpm_color);

  if (s_bpm_icon_layer) {
    layer_mark_dirty(s_bpm_icon_layer);
  }
}

bool bpm_module_create(
    Layer* root,
    const WatchfaceTextSubstratum* text,
    const WatchfaceIconSubstratum* icon,
    GFont font) {
  if (!root || !text || !font) {
    return false;
  }

  s_bpm_layer = substratum_renderer_create_text_layer(root, text, font);

  if (!s_bpm_layer) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to create BPM text layer");
    return false;
  }

  s_bpm_is_valid = false;
#if ATAGLANCE_DEBUG
  s_debug_bpm_is_set = false;
  s_debug_bpm = BPM_INVALID;
#endif

  if (icon && icon->is_enabled) {
    s_bpm_icon_layer = substratum_renderer_create_icon_layer(
        root,
        icon,
        bpm_icon_update_proc);
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

  s_bpm_color = BPM_UNINITIALIZED_COLOR;
  s_bpm_buffer[0] = '\0';
  s_bpm_is_valid = false;
  s_palette = NULL;
#if ATAGLANCE_DEBUG
  s_debug_bpm_is_set = false;
  s_debug_bpm = BPM_INVALID;
#endif
}

void bpm_module_refresh(const ColorPalette* palette) {
  if (!palette) {
    return;
  }

  s_palette = palette;
  update_bpm();
}

#if ATAGLANCE_DEBUG
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
