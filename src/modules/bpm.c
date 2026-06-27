#ifdef PBL_HEALTH
#include "bpm.h"
#include "helper.h"
#include "substratum_renderer.h"

#define MAX_STR_LEN 12

#define BPM_INVALID -1
#define BPM_MIN 1
#define BPM_EXTREME 120
#define BPM_HIGH 100
#define IS_BPM_VALID(bpm) ((bpm) >= BPM_MIN)

typedef struct {
  GColor background;
  GColor normal;
  GColor warning;
  GColor critical;
  GColor unknown;
} BpmPalette;

static BpmPalette s_bpm_palette = {0};

static char s_bpm_buffer[MAX_STR_LEN] = {0};
static GBitmap *s_bpm_bitmap = NULL;
static Layer *s_bpm_icon_layer = NULL;
static TextLayer *s_bpm_layer = NULL;
static bool s_bpm_is_valid = false;

// This needs to be the main color of the BPM icon
static GColor s_bpm_icon_color = GColorBlack;
// This needs to be any color other than the main color of the bpm icon
static GColor s_bpm_color = WATCHFACE_UNINITIALIZED_TEXT_COLOR;

#if ATAGLANCE_DEBUG
static bool s_debug_bpm_is_set = false;
static int s_debug_bpm = BPM_INVALID;
#endif

static const BpmPalette c_dark_bpm_palette = {
    .warning = PBL_IF_COLOR_ELSE(GColorPastelYellow, GColorWhite),
    .critical = PBL_IF_COLOR_ELSE(GColorShockingPink, GColorWhite),
};

static const BpmPalette c_light_bpm_palette = {
    .warning = PBL_IF_COLOR_ELSE(GColorVividViolet, GColorBlack),
    .critical = PBL_IF_COLOR_ELSE(GColorRed, GColorBlack),
};

static void bpm_update_palette(const ColorPalette *palette);
static GColor calculate_bpm_color(int bpm);
static void bpm_icon_update_proc(Layer *layer, GContext *ctx);
static void update_bpm(void);

static void bpm_update_palette(const ColorPalette *palette) {
  const BpmPalette *template = palette->is_light_mode ? &c_light_bpm_palette : &c_dark_bpm_palette;

  if (MODULE_PALETTE_LOADED(s_bpm_palette)) {
    // We know that light-mode and dark-mode have different backgrounds
    if (HELPER_COLOR_EQUAL(s_bpm_palette.background, palette->background)) {
      // The palette doesn't need to be updated
      return;
    }
  }
  // Two possibilities:
  // 1. First time the palette is being initialized
  // 2. The palette has changed
  s_bpm_palette = *template;
  s_bpm_palette.background = palette->background;
  s_bpm_palette.normal = palette->primary_text;
  s_bpm_palette.unknown = palette->unavailable_text;
}

static GColor calculate_bpm_color(int bpm) {
  if (!MODULE_PALETTE_LOADED(s_bpm_palette)) {
    return WATCHFACE_UNINITIALIZED_TEXT_COLOR;
  }

  if (bpm < BPM_MIN) {
    return s_bpm_palette.unknown;
  }
  if (bpm >= BPM_EXTREME) {
    return s_bpm_palette.critical;
  }
  if (bpm >= BPM_HIGH) {
    return s_bpm_palette.warning;
  }
  return s_bpm_palette.normal;
}

static void bpm_icon_update_proc(Layer *layer, GContext *ctx) {
  if (!layer || !ctx || !MODULE_PALETTE_LOADED(s_bpm_palette)) {
    return;
  }

  GRect bounds = layer_get_bounds(layer);

  graphics_context_set_fill_color(ctx, s_bpm_palette.background);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  if (s_bpm_bitmap && !HELPER_COLOR_EQUAL(s_bpm_icon_color, s_bpm_color)) {
    if (helper_replace_color_in_bitmap(s_bpm_bitmap, s_bpm_icon_color, s_bpm_color)) {
      s_bpm_icon_color = s_bpm_color;
    }
  }

  graphics_context_set_compositing_mode(ctx, GCompOpSet);
  graphics_draw_bitmap_in_rect(ctx, s_bpm_bitmap, bounds);

  if (!s_bpm_is_valid) {
    substratum_renderer_draw_unavailable_slash(ctx, &bounds.size, s_bpm_palette.unknown);
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
      health_service_metric_accessible(HealthMetricHeartRateBPM, now, now);

  if (hr_mask & HealthServiceAccessibilityMaskAvailable) {
    bpm_to_render = (int)health_service_peek_current_value(HealthMetricHeartRateBPM);
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

bool bpm_module_create(Layer *root, const WatchfaceTextSubstratum *text,
                       const WatchfaceIconSubstratum *icon, GFont font) {
  if (!root || !text || !font) {
    return false;
  }

  s_bpm_layer = substratum_renderer_create_text_layer(root, text, font);

  if (!s_bpm_layer) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to create BPM text layer");
    return false;
  }

  s_bpm_is_valid = false;
  s_bpm_icon_color = GColorBlack;
#if ATAGLANCE_DEBUG
  s_debug_bpm_is_set = false;
  s_debug_bpm = BPM_INVALID;
#endif

  if (icon && icon->is_enabled) {
    uint32_t resource_id = 0;
#if defined(PBL_PLATFORM_EMERY) || defined(PBL_PLATFORM_GABBRO)
    resource_id = RESOURCE_ID_ECG_FULL;
#else
    resource_id = RESOURCE_ID_ECG_COMPACT;
#endif
    s_bpm_bitmap = gbitmap_create_with_resource(resource_id);

    if (s_bpm_bitmap) {
      s_bpm_icon_layer = substratum_renderer_create_icon_layer(root, icon, bpm_icon_update_proc);
      if (!s_bpm_icon_layer) {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Failed to create BPM icon layer");
        gbitmap_destroy(s_bpm_bitmap);
        s_bpm_bitmap = NULL;
      }
    } else {
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Failed to create BPM icon bitmap");
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
  if (s_bpm_bitmap) {
    gbitmap_destroy(s_bpm_bitmap);
    s_bpm_bitmap = NULL;
  }

  s_bpm_color = WATCHFACE_UNINITIALIZED_TEXT_COLOR;
  s_bpm_icon_color = GColorBlack;
  s_bpm_buffer[0] = '\0';
  s_bpm_is_valid = false;
  s_bpm_palette = (BpmPalette){0};
#if ATAGLANCE_DEBUG
  s_debug_bpm_is_set = false;
  s_debug_bpm = BPM_INVALID;
#endif
}

void bpm_module_refresh(const ColorPalette *palette) {
  if (!palette) {
    return;
  }

  bpm_update_palette(palette);
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
