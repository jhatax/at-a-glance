#if defined(PBL_HEALTH)
#include "health.h"
#include "helper.h"
#include "../c/ataglance.h"

typedef enum {
  HEALTH_BUF_BPM = 0,
  HEALTH_BUF_STEPS,
  HEALTH_BUF_COUNT,
} HealthTextBufferId;

static char s_text_buffers[HEALTH_BUF_COUNT][ATAGLANCE_MAX_STR_LEN];

static Layer* s_bpm_icon_layer;
static TextLayer* s_bpm_layer;
static int s_bpm;
static bool s_bpm_is_available;

static Layer* s_steps_icon_layer;
static TextLayer* s_steps_layer;
static bool s_steps_is_available;

static const VisualPalette* s_palette;

static inline TextLayer* create_health_text_layer(
    Layer* parent,
    const GRect* frame,
    GFont font);
static char* get_text_buffer(HealthTextBufferId id);
static inline GColor calculate_bpm_color(int bpm);
static void draw_bpm_icon_with_color(
    GContext* ctx,
    GColor color,
    const GSize* bounds_size);
static void draw_data_gap_slash(
    GContext* ctx,
    const GSize* bounds_size);
static void health_draw_scaled_line(
    GContext* ctx,
    const GSize* bounds_size,
    int16_t x0,
    int16_t y0,
    int16_t x1,
    int16_t y1);
static void bpm_icon_update_proc(Layer* layer, GContext* ctx);
static void steps_icon_update_proc(Layer* layer, GContext* ctx);
static void update_bpm(void);
static void update_steps(void);

static inline TextLayer* create_health_text_layer(
    Layer* parent,
    const GRect* frame,
    GFont font) {
  if (!parent || !frame) {
    return NULL;
  }

  TextLayer* layer = text_layer_create(*frame);
  if (!layer) {
    return NULL;
  }

  text_layer_set_background_color(layer, GColorClear);
  text_layer_set_font(layer, font);
  text_layer_set_text_alignment(layer, GTextAlignmentLeft);
  layer_add_child(parent, text_layer_get_layer(layer));
  return layer;
}

static char* get_text_buffer(HealthTextBufferId id) {
  switch (id) {
    case HEALTH_BUF_BPM:
    case HEALTH_BUF_STEPS:
      return s_text_buffers[id];

    case HEALTH_BUF_COUNT: {
        for (size_t i = 0; i < HEALTH_BUF_COUNT; ++i) {
          s_text_buffers[i][0] = '\0';
        }
        return NULL;
    }

    default:
      return NULL;
  }
}

static inline GColor calculate_bpm_color(int bpm) {
  if (!s_palette) {
    return GColorWhite;
  }
  if (bpm <= 0) {
    return s_palette->unavailable_text;
  } else if (bpm > 120) {
    return PBL_IF_COLOR_ELSE(
        GColorRed,
        display_legible_over_background(s_palette));
  } else if (bpm >= 100) {
    return PBL_IF_COLOR_ELSE(
        GColorMagenta,
        display_legible_over_background(s_palette));
  } else {
    return PBL_IF_COLOR_ELSE(
        GColorJaegerGreen,
        display_legible_over_background(s_palette));
  }
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
  health_draw_scaled_line(ctx, bounds_size, 3, 15, 8, 15);
  health_draw_scaled_line(ctx, bounds_size, 8, 15, 11, 8);
  health_draw_scaled_line(ctx, bounds_size, 11, 8, 15, 22);
  health_draw_scaled_line(ctx, bounds_size, 15, 22, 19, 12);
  health_draw_scaled_line(ctx, bounds_size, 19, 12, 24, 12);
}

static void draw_data_gap_slash(
    GContext* ctx,
    const GSize* bounds_size) {
  graphics_context_set_stroke_width(ctx, 3);
  graphics_context_set_stroke_color(
      ctx,
      display_legible_over_background(s_palette));
  health_draw_scaled_line(ctx, bounds_size, 5, 3, 24, 26);
}

static void health_draw_scaled_line(
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
  if (!layer || !ctx || !s_palette) {
    return;
  }

  GRect bounds = layer_get_bounds(layer);

  graphics_context_set_fill_color(ctx, s_palette->background);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  draw_bpm_icon_with_color(
      ctx,
      calculate_bpm_color(s_bpm),
      &bounds.size);

  if (!s_bpm_is_available) {
    draw_data_gap_slash(ctx, &bounds.size);
  }
}

static void steps_icon_update_proc(Layer* layer, GContext* ctx) {
  if (!layer || !ctx || !s_palette) {
    return;
  }

  GRect bounds = layer_get_bounds(layer);

  graphics_context_set_fill_color(ctx, s_palette->background);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  GColor steps_icon_color = s_steps_is_available ?
      s_palette->steps_icon : s_palette->unavailable_text;

  graphics_context_set_fill_color(ctx, steps_icon_color);

  graphics_fill_circle(
      ctx,
      layout_scaled_icon_point(&bounds.size, 14, 9),
      layout_scale_icon_coord(&bounds.size, 7));

  graphics_context_set_fill_color(ctx, s_palette->background);
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

static void update_bpm(void) {
  char* bpm_buf = get_text_buffer(HEALTH_BUF_BPM);

  if (!bpm_buf || !s_bpm_layer || !s_palette) {
    return;
  }

  time_t now = time(NULL);
  HealthServiceAccessibilityMask hr_mask =
      health_service_metric_accessible(
          HealthMetricHeartRateBPM,
          now,
          now);

  GColor text_color = s_palette->unavailable_text;
  s_bpm = 0;
  s_bpm_is_available = false;

  if (hr_mask & HealthServiceAccessibilityMaskAvailable) {
    s_bpm = (int) health_service_peek_current_value(
        HealthMetricHeartRateBPM);

    if (s_bpm > 0) {
      snprintf(bpm_buf, ATAGLANCE_MAX_STR_LEN, "%d", s_bpm);
      text_color = calculate_bpm_color(s_bpm);
      s_bpm_is_available = true;
    } else {
      snprintf(
          bpm_buf,
          ATAGLANCE_MAX_STR_LEN,
          "%s",
          DISPLAY_UNAVAILABLE_TEXT);
    }
  } else {
    snprintf(
        bpm_buf,
        ATAGLANCE_MAX_STR_LEN,
        "%s",
        DISPLAY_UNAVAILABLE_TEXT);
  }

  display_update_text_layer(
      s_bpm_layer,
      bpm_buf,
      text_color);

  if (s_bpm_icon_layer) {
    layer_mark_dirty(s_bpm_icon_layer);
  }
}

static void update_steps(void) {
  char* steps_buf = get_text_buffer(HEALTH_BUF_STEPS);
  if (!steps_buf || !s_steps_layer || !s_palette) {
    return;
  }

  GColor text_color = s_palette->unavailable_text;
  s_steps_is_available = false;

  HealthServiceAccessibilityMask steps_mask =
      health_service_metric_accessible(
          HealthMetricStepCount,
          time_start_of_today(),
          time(NULL));

  if (steps_mask & HealthServiceAccessibilityMaskAvailable) {
    HealthValue steps = health_service_sum_today(HealthMetricStepCount);
    if (steps >= 0) {
      snprintf(steps_buf, ATAGLANCE_MAX_STR_LEN, "%d", (int)steps);
      text_color = s_palette->primary_text;
      s_steps_is_available = true;
    } else {
      snprintf(
          steps_buf,
          ATAGLANCE_MAX_STR_LEN,
          "%s",
          DISPLAY_UNAVAILABLE_TEXT);
    }
  } else {
    snprintf(
        steps_buf,
        ATAGLANCE_MAX_STR_LEN,
        "%s",
        DISPLAY_UNAVAILABLE_TEXT);
  }

  display_update_text_layer(
      s_steps_layer,
      steps_buf,
      text_color);

  if (s_steps_icon_layer) {
    layer_mark_dirty(s_steps_icon_layer);
  }
}

bool health_module_create(
    Layer* root,
    const WatchfaceLayout* layout,
    GFont value_font,
    const VisualPalette* palette) {
  if (!root || !layout || !palette) {
    return false;
  }

  s_palette = palette;

  s_bpm = 0;
  s_bpm_is_available = false;

  s_bpm_layer = create_health_text_layer(
      root,
      &layout->bpm_text_frame,
      value_font);

  if (!s_bpm_layer) {
    APP_LOG(APP_LOG_LEVEL_ERROR,
            "Failed to create BPM text layer");
  }

  if (s_bpm_layer) {
    s_bpm_icon_layer = layer_create(layout->bpm_icon_frame);
    if (s_bpm_icon_layer) {
      layer_set_update_proc(s_bpm_icon_layer, bpm_icon_update_proc);
      layer_add_child(root, s_bpm_icon_layer);
    }
  }

  s_steps_is_available = false;

  s_steps_layer = create_health_text_layer(
      root,
      &layout->steps_text_frame,
      value_font);

  if (!s_steps_layer) {
    APP_LOG(APP_LOG_LEVEL_ERROR,
            "Failed to create steps text layer");
  }

  if (s_steps_layer) {
    s_steps_icon_layer = layer_create(layout->steps_icon_frame);
    if (s_steps_icon_layer) {
      layer_set_update_proc(s_steps_icon_layer, steps_icon_update_proc);
      layer_add_child(root, s_steps_icon_layer);
    }
  }

  return s_bpm_layer && s_steps_layer;
}

void health_module_destroy(void) {
  if (s_bpm_icon_layer) {
    layer_destroy(s_bpm_icon_layer);
    s_bpm_icon_layer = NULL;
  }
  if (s_bpm_layer) {
    text_layer_destroy(s_bpm_layer);
    s_bpm_layer = NULL;
  }
  if (s_steps_layer) {
    text_layer_destroy(s_steps_layer);
    s_steps_layer = NULL;
  }
  if (s_steps_icon_layer) {
    layer_destroy(s_steps_icon_layer);
    s_steps_icon_layer = NULL;
  }

  get_text_buffer(HEALTH_BUF_COUNT);
}

void health_module_refresh(const VisualPalette* palette) {
  if (palette) {
    s_palette = palette;
  }

  update_steps();
  update_bpm();
}

void health_module_handle_event(HealthEventType event) {
  bool significant_update = event == HealthEventSignificantUpdate;

  if (significant_update || event == HealthEventMovementUpdate) {
    update_steps();
  }
  if (significant_update || event == HealthEventHeartRateUpdate) {
    update_bpm();
  }
}

#else

#include <pebble.h>
#include "display.h"
#include "layout.h"

bool health_module_create(
    Layer* root,
    const WatchfaceLayout* layout,
    GFont value_font,
    const VisualPalette* palette) {
  (void)root;
  (void)layout;
  (void)value_font;
  (void)palette;
  return true;
}

void health_module_destroy(void) {
}

void health_module_refresh(const VisualPalette* palette) {
  (void)palette;
}

#endif
