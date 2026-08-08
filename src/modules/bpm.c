#include "bpm.h"
#ifdef PBL_HEALTH
#include "helper.h"
#include "substratum_renderer.h"

#define MAX_STR_LEN 12

#define BPM_INVALID -1
#define BPM_MIN 1
#define BPM_EXTREME 120
#define BPM_HIGH 100
#define BPM_MAX 220
#define BPM_IN_RANGE(bpm) HELPER_VALUE_IN_RANGE((bpm), BPM_MIN, BPM_MAX)

#define BPM_ICON_INITIAL_COLOR GColorBlack

typedef struct {
  GColor background;
  GColor normal;
  GColor elevated;
  GColor critical;
  GColor outofrange;
} BpmPalette;

static BpmPalette s_bpm_palette = {0};

static char s_bpm_buffer[MAX_STR_LEN] = {0};
static GBitmap* s_bpm_bitmap = NULL;
static Layer* s_bpm_icon_layer = NULL;
static TextLayer* s_bpm_layer = NULL;
static bool s_bpm_is_valid = false;

// This needs to be the main color of the BPM icon
static GColor s_bpm_icon_color;
// This needs to be any color other than the main color of the bpm icon
static GColor s_bpm_color;

static bool s_oneshot_bpm_is_set = false;
static int s_oneshot_bpm = BPM_INVALID;

static const BpmPalette c_dark_bpm_palette = {
    .elevated = PBL_IF_COLOR_ELSE(GColorYellow, GColorWhite),
    .critical = PBL_IF_COLOR_ELSE(GColorRed, GColorWhite),
};

static const BpmPalette c_light_bpm_palette = {
    .elevated = PBL_IF_COLOR_ELSE(GColorVividViolet, GColorBlack),
    .critical = PBL_IF_COLOR_ELSE(GColorRed, GColorBlack),
};

static void bpm_update_palette(const ColorPalette* palette);
static GColor calculate_bpm_color(int bpm);
static void bpm_icon_update_proc(Layer* layer,
    GContext* ctx);
static void update_bpm();
static void bpm_module_oneshot_clear_bpm();

static void bpm_update_palette(
    const ColorPalette* palette) {
  const BpmPalette* template = palette->is_light_mode ? &c_light_bpm_palette : &c_dark_bpm_palette;

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
  s_bpm_palette.outofrange = palette->outofrange_text;
}

static GColor calculate_bpm_color(
    int bpm) {
  if (!MODULE_PALETTE_LOADED(s_bpm_palette)) {
    return WATCHFACE_UNINITIALIZED_TEXT_COLOR;
  }

  if (!(BPM_IN_RANGE(bpm))) {
    return s_bpm_palette.outofrange;
  }
  if (bpm >= BPM_EXTREME) {
    return s_bpm_palette.critical;
  }
  if (bpm >= BPM_HIGH) {
    return s_bpm_palette.elevated;
  }
  return s_bpm_palette.normal;
}

static void bpm_icon_update_proc(
    Layer* layer,
    GContext* ctx) {
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
    substratum_renderer_mark_info_outofrange(ctx,
        &bounds,
        s_bpm_palette.outofrange,
        s_bpm_palette.background);
  }
}

// Flow:
// 1. Peek the current HealthService BPM value as the baseline.
// 2. In debug builds, apply a queued debug BPM as a one-shot override.
// 3. Determine validity and color.
// 4. Render text.
// 5. Mark icon to be repainted.
//
// One-shot values intentionally overwrite the peeked value only for the current
// refresh. This keeps one-shot messages as a transport/render test hook without
// needing special-casing.
static void update_bpm() {
  int bpm_to_render = BPM_INVALID;

  time_t now = time(NULL);
  HealthServiceAccessibilityMask hr_mask =
      health_service_metric_accessible(HealthMetricHeartRateBPM, now, now);

  if (hr_mask & HealthServiceAccessibilityMaskAvailable) {
    bpm_to_render = (int)health_service_peek_current_value(HealthMetricHeartRateBPM);
  }

  if (s_oneshot_bpm_is_set) {
    bpm_to_render = s_oneshot_bpm;
    s_oneshot_bpm_is_set = false;
    s_oneshot_bpm = BPM_INVALID;
  }

  // Save this state unconditionally
  s_bpm_is_valid = BPM_IN_RANGE(bpm_to_render);
  s_bpm_color = calculate_bpm_color(bpm_to_render);

  // Color the text as either normal or unknown
  GColor text_color = s_bpm_palette.normal;
  if (s_bpm_is_valid) {
    snprintf(s_bpm_buffer, MAX_STR_LEN, "%d", bpm_to_render);
  } else {
    snprintf(s_bpm_buffer, MAX_STR_LEN, "%s", WATCHFACE_OUTOFRANGE_TEXT);
    text_color = s_bpm_palette.outofrange;
  }

  substratum_renderer_update_text_layer(s_bpm_layer, s_bpm_buffer, text_color);

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

  // Symmetry with destroy
  s_bpm_is_valid = false;
  bpm_module_oneshot_clear_bpm();

  s_bpm_icon_color = BPM_ICON_INITIAL_COLOR;
  // By making these different from the get-go, we ensure that the
  // color is replaced the first time the icon is rendered in the right color
  s_bpm_color = gcolor_legible_over(s_bpm_icon_color);

  if (icon) {
    uint32_t resource_id = 0;
    resource_id = RESOURCE_ID_ECG;
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

void bpm_module_destroy() {
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

  // Symmetry with create
  s_bpm_is_valid = false;
  bpm_module_oneshot_clear_bpm();
  s_bpm_color = WATCHFACE_UNINITIALIZED_TEXT_COLOR;
  s_bpm_icon_color = BPM_ICON_INITIAL_COLOR;
  s_bpm_buffer[0] = '\0';
  memset(&s_bpm_buffer, 0, sizeof(s_bpm_buffer));
}

void bpm_module_refresh(
    const ColorPalette* palette) {
  if (!palette) {
    return;
  }

  bpm_update_palette(palette);
  update_bpm();
}

void bpm_module_oneshot_set_bpm(
    int bpm) {
  if (BPM_IN_RANGE(bpm)) {
    s_oneshot_bpm = bpm;
    s_oneshot_bpm_is_set = true;
  } else {
    s_oneshot_bpm = BPM_INVALID;
    s_oneshot_bpm_is_set = false;
  }
}

static void bpm_module_oneshot_clear_bpm() {
  s_oneshot_bpm_is_set = false;
  s_oneshot_bpm = BPM_INVALID;
}
#endif
