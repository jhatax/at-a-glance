#ifdef PBL_HEALTH
#include "steps.h"

#include "helper.h"
#include "substratum_renderer.h"
#include "../c/ataglance.h"

#define STEPS_INVALID -1

static char s_steps_buffer[ATAGLANCE_MAX_STR_LEN] = {0};
static GBitmap* s_steps_bitmap = NULL;
static Layer* s_steps_icon_layer = NULL;
static TextLayer* s_steps_layer = NULL;
static bool s_steps_is_available = false;
static const WatchfaceSurface* s_surface = NULL;
static GColor s_steps_icon_color = {0};
#if DEBUG_ATAGLANCE
static bool s_debug_steps_is_set = false;
static int s_debug_steps = STEPS_INVALID;
#endif

static void steps_icon_update_proc(Layer* layer, GContext* ctx);
static void draw_steps_bitmap_in_frame(
    GContext* ctx,
    const GRect* frame);
static void apply_steps_value(int steps, bool is_available);
static void update_steps(void);

static void draw_steps_bitmap_in_frame(
    GContext* ctx,
    const GRect* frame) {
  if (!ctx || !frame || !s_steps_bitmap) {
    return;
  }

  GRect bitmap_bounds = gbitmap_get_bounds(s_steps_bitmap);
  GRect draw_frame;

  if (bitmap_bounds.size.w > 0 && bitmap_bounds.size.h > 0) {
    // Fit the portrait-oriented walking bitmap into the icon frame
    // while preserving aspect ratio.
    int16_t draw_w = frame->size.w;
    int16_t draw_h = (bitmap_bounds.size.h * draw_w) /
        bitmap_bounds.size.w;

    if (draw_h > frame->size.h) {
      draw_h = frame->size.h;
      draw_w = (bitmap_bounds.size.w * draw_h) /
          bitmap_bounds.size.h;
    }

    draw_frame = GRect(
        frame->origin.x + ((frame->size.w - draw_w) / 2),
        frame->origin.y + ((frame->size.h - draw_h) / 2),
        draw_w,
        draw_h);
  } else {
    draw_frame = *frame;
  }

  graphics_context_set_compositing_mode(ctx, GCompOpSet);
  graphics_draw_bitmap_in_rect(ctx, s_steps_bitmap, draw_frame);
}

static void steps_icon_update_proc(Layer* layer, GContext* ctx) {
  if (!layer || !ctx || !s_surface || !s_surface->style.palette) {
    return;
  }

  const ColorPalette* palette = s_surface->style.palette;
  GRect bounds = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, palette->background);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  GColor icon_color = palette->primary_text;

  if (s_steps_bitmap &&
      !helper_color_equal(s_steps_icon_color, icon_color)) {
    if (helper_replace_color_in_bitmap(
            s_steps_bitmap,
            s_steps_icon_color,
            icon_color)) {
      s_steps_icon_color = icon_color;
    }
  }

  draw_steps_bitmap_in_frame(ctx, &bounds);

  if (!s_steps_is_available) {
    substratum_renderer_draw_unavailable_slash(
        ctx,
        &bounds.size,
        palette->primary_text);
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

  substratum_renderer_update_text_layer(
      s_steps_layer,
      s_steps_buffer,
      text_color);

  if (s_steps_icon_layer) {
    layer_mark_dirty(s_steps_icon_layer);
  }
}

static void update_steps(void) {
#if DEBUG_ATAGLANCE
  if (s_debug_steps_is_set) {
    apply_steps_value(s_debug_steps, s_debug_steps >= 0);
    s_debug_steps_is_set = false;
    s_debug_steps = STEPS_INVALID;
    return;
  }
#endif

  HealthServiceAccessibilityMask steps_mask =
      health_service_metric_accessible(
          HealthMetricStepCount,
          time_start_of_today(),
          time(NULL));

  int steps = STEPS_INVALID;
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

  s_steps_layer = substratum_renderer_create_text_layer(
      root,
      text,
      surface->style.fonts[text->font_role]);

  if (!s_steps_layer) {
    APP_LOG(APP_LOG_LEVEL_ERROR,
            "Failed to create steps text layer");
    return false;
  }

  s_steps_is_available = false;
#if DEBUG_ATAGLANCE
  s_debug_steps_is_set = false;
  s_debug_steps = STEPS_INVALID;
#endif
  // The walking bitmap ships with black foreground pixels. Seed the
  // cached color to that source palette value so the first recolor
  // pass knows which color to replace.
  s_steps_icon_color = GColorBlack;

  s_surface = surface;
  if (icon->is_enabled) {
    uint32_t resource_id = 0;
  #if defined(PBL_PLATFORM_EMERY) || defined(PBL_PLATFORM_GABBRO)
    resource_id = RESOURCE_ID_WALK_FULL;
  #else
    resource_id = RESOURCE_ID_WALK_COMPACT;
  #endif
    s_steps_bitmap = gbitmap_create_with_resource(resource_id);

    if(s_steps_bitmap) {
      s_steps_icon_layer = substratum_renderer_create_icon_layer(root, icon, steps_icon_update_proc);
      if (!s_steps_icon_layer) {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Failed to create steps icon layer");
        gbitmap_destroy(s_steps_bitmap);
        s_steps_bitmap = NULL;
      }
    } else {
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Failed to create steps icon bitmap");
    }
  }
  return true;
}

void steps_module_destroy(void) {
  if (s_steps_layer) {
    text_layer_destroy(s_steps_layer);
    s_steps_layer = NULL;
  }

  if (s_steps_icon_layer) {
    layer_destroy(s_steps_icon_layer);
    s_steps_icon_layer = NULL;
  }
  if (s_steps_bitmap) {
    gbitmap_destroy(s_steps_bitmap);
    s_steps_bitmap = NULL;
  }

  s_steps_buffer[0] = '\0';
  s_steps_is_available = false;
  s_surface = NULL;
  s_steps_icon_color = GColorBlack;
  #if DEBUG_ATAGLANCE
  s_debug_steps_is_set = false;
  s_debug_steps = STEPS_INVALID;
  #endif
}

void steps_module_refresh(void) {
  update_steps();
}

#if DEBUG_ATAGLANCE
void steps_module_debug_set_steps(int steps) {
  s_debug_steps = steps;
  s_debug_steps_is_set = true;
}

void steps_module_debug_clear_steps(void) {
  s_debug_steps_is_set = false;
  s_debug_steps = STEPS_INVALID;
}
#endif

#endif
