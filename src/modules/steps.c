#include "steps.h"
#ifdef PBL_HEALTH

#include "helper.h"
#include "settings.h"
#include "substratum_renderer.h"

#define MAX_STR_LEN 12

enum {
  STEPS_INVALID = -1,
  STEPS_MIN = 1,
  STEPS_APPROACHING_GOAL_PERCENT = 70,
  STEPS_MAX = 100000,
};

// Steps should at least be STEPS_MIN and less than STEPS_MAX
#define STEPS_IN_RANGE(s) (HELPER_VALUE_IN_RANGE((s), STEPS_MIN, STEPS_MAX))

// This needs to be the main color of the steps icon
static GColor s_steps_icon_color;

// This needs to be any color other than the main color of the steps icon
static GColor s_steps_color;

static char s_steps_buffer[MAX_STR_LEN] = {0};
static GBitmap* s_steps_bitmap = NULL;
static TextLayer* s_steps_layer = NULL;
static Layer* s_steps_icon_layer = NULL;
static Layer* s_steps_progress_layer = NULL;
static int s_steps = STEPS_INVALID;
static bool s_steps_is_available = true;
static uint16_t s_steps_goal = STEPS_INVALID;
static uint16_t s_steps_threshold = STEPS_INVALID;

static bool s_oneshot_steps_is_set = false;
static int s_oneshot_steps = STEPS_INVALID;

typedef struct {
  GColor background;
  GColor normal;
  GColor outofrange;
  GColor approaching;
  GColor achieved;
} StepsPalette;

static StepsPalette s_steps_palette = {0};

static const StepsPalette c_dark_steps_palette = {
  .approaching = PBL_IF_COLOR_ELSE(GColorYellow, GColorWhite),
  .achieved = PBL_IF_COLOR_ELSE(GColorIslamicGreen, GColorWhite),
};

static const StepsPalette c_light_steps_palette = {
  .approaching = PBL_IF_COLOR_ELSE(GColorVividViolet, GColorBlack),
  .achieved = PBL_IF_COLOR_ELSE(GColorGreen, GColorBlack),
};

static void steps_update_palette(const ColorPalette* palette);
static GColor calculate_steps_color(int steps);
static void steps_icon_update_proc(Layer* layer, GContext* ctx);
static void steps_progress_update_proc(Layer* layer, GContext* ctx);
static void apply_steps_value(int steps, bool is_available);
static void update_steps();

static void steps_update_palette(
  const ColorPalette* palette) {
  const StepsPalette* template =
    palette->is_light_mode ? &c_light_steps_palette : &c_dark_steps_palette;

  if (MODULE_PALETTE_LOADED(s_steps_palette)) {
    // We know that light-mode and dark-mode have different backgrounds
    if (HELPER_COLOR_EQUAL(s_steps_palette.background, palette->background)) {
      // The palette doesn't need to be updated
      return;
    }
  }
  // Two possibilities:
  // 1. First time the palette is being initialized
  // 2. The palette has changed
  s_steps_palette = *template;
  s_steps_palette.background = palette->background;
  s_steps_palette.normal = palette->primary_text;
  s_steps_palette.outofrange = palette->outofrange_text;
}

static GColor calculate_steps_color(
  int steps) {
  if (!MODULE_PALETTE_LOADED(s_steps_palette)) {
    return WATCHFACE_UNINITIALIZED_TEXT_COLOR;
  }

  if (!(STEPS_IN_RANGE(steps))) {
    return s_steps_palette.outofrange;
  }

  if (steps >= s_steps_goal) {
    return s_steps_palette.achieved;
  }

  if (steps >= s_steps_threshold) {
    return s_steps_palette.approaching;
  }
  return s_steps_palette.normal;
}

static void steps_icon_update_proc(
  Layer* layer,
  GContext* ctx) {
  if (!layer || !ctx || !MODULE_PALETTE_LOADED(s_steps_palette)) {
    return;
  }

  GRect bounds = layer_get_bounds(layer);

  if (s_steps_bitmap) {
    if (!HELPER_COLOR_EQUAL(s_steps_icon_color, s_steps_color)) {
      if (helper_replace_color_in_bitmap(s_steps_bitmap, s_steps_icon_color, s_steps_color)) {
        s_steps_icon_color = s_steps_color;
      }
    }
  }

  graphics_context_set_compositing_mode(ctx, GCompOpSet);
  graphics_draw_bitmap_in_rect(ctx, s_steps_bitmap, bounds);

  if (!s_steps_is_available) {
    substratum_renderer_draw_unavailable_slash(ctx, &bounds.size, s_steps_palette.normal);
  }
}

static void steps_progress_update_proc(
  Layer* layer,
  GContext* ctx) {
  if (!layer || !ctx || !MODULE_PALETTE_LOADED(s_steps_palette)) {
    return;
  }

  GRect bounds = layer_get_bounds(layer);
  // Testing the situation with the stroke width
  // Is the x,y the bottom of the stroke, center?
  // It's definitely not the top!
  GPoint pt1 = bounds.origin;

  if (s_steps_is_available) {
    // Set the fill color to fill the track with the background
    // Set the stroke color to be the steps color to outline the track
    // This is particularly important for monochrome displays
    graphics_context_set_stroke_color(ctx, s_steps_color);
    graphics_context_set_fill_color(ctx, s_steps_palette.background);
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);
    graphics_draw_rect(ctx, bounds);
    // The text and progress bar need to be the same color. There might not be a steps icon.
    graphics_context_set_fill_color(ctx, s_steps_color);
    // We need to calculate the % that should be filled, i.e. x of the second point
    int completed = HELPER_CLAMP_TO_RANGE((HELPER_ROUND_UP((s_steps * 100), s_steps_goal)), 0, 100);
    // We have completed percent, so let's get the width of the line next
    // If goal achieved, pt2 is already correct.
    int width = 0;
    if (completed > 0) {
      // Change the width to be drawn. While this should never be greater than the width,
      // it doesn't hurt to be defensive+
      width =
        HELPER_CLAMP_MAX(HELPER_ROUND_UP((completed * bounds.size.w), 100), (bounds.size.w - 1));
    }
    graphics_fill_rect(ctx, GRect(pt1.x, pt1.y, width, bounds.size.h), 0, GCornerNone);
  } else {
    // Steps are not available. Draw over the rect in the background color
    graphics_context_set_fill_color(ctx, s_steps_palette.background);
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  }
}

static void apply_steps_value(
  int steps,
  bool is_available) {
  if (!s_steps_layer || !MODULE_PALETTE_LOADED(s_steps_palette)) {
    return;
  }

  s_steps = steps;
  s_steps_is_available = is_available;

  // You have to save the steps color even though we show text in only normal / unknown
  s_steps_color = calculate_steps_color(steps);

  // Do not render the text in the same color as the icon
  // except when the text is available vs. unavailable
  GColor text_color = s_steps_palette.normal;
  if (is_available) {
    snprintf(s_steps_buffer, MAX_STR_LEN, "%d", steps);
  } else {
    snprintf(s_steps_buffer, MAX_STR_LEN, "%s", WATCHFACE_OUTOFRANGE_TEXT);
    text_color = s_steps_palette.outofrange;
  }

  substratum_renderer_update_text_layer(s_steps_layer, s_steps_buffer, text_color);

  // Update visual elements (icon & progress)
  if (s_steps_icon_layer) {
    layer_mark_dirty(s_steps_icon_layer);
  }

  if (s_steps_progress_layer) {
    layer_mark_dirty(s_steps_progress_layer);
  }
}

static void update_steps() {
  int steps = STEPS_INVALID;
  bool is_available = false;

  // Store the current total sum first
  HealthServiceAccessibilityMask steps_mask =
    health_service_metric_accessible(HealthMetricStepCount, time_start_of_today(), time(NULL));

  if (steps_mask & HealthServiceAccessibilityMaskAvailable) {
    HealthValue health_steps = health_service_sum_today(HealthMetricStepCount);
    is_available = STEPS_IN_RANGE((int)health_steps);
    if (is_available) {
      steps = (int)health_steps;
    }
  }

  // If there is a one-shot steps value, overwrite the retrieved sum of steps
  if (s_oneshot_steps_is_set) {
    steps = s_oneshot_steps;
    is_available = STEPS_IN_RANGE(s_oneshot_steps);
    s_oneshot_steps_is_set = false;
    s_oneshot_steps = STEPS_INVALID;
  }

  apply_steps_value(steps, is_available);
}

bool steps_module_create(
  Layer* root,
  const WatchfaceTextSubstratum* text,
  const WatchfaceIconSubstratum* icon,
  const GRect* progress,
  GFont font) {
  // The icon is not mandatory
  if (!root || !text || !progress || !font) {
    return false;
  }

  s_steps_layer = substratum_renderer_create_text_layer(root, text, font);

  if (!s_steps_layer) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to create steps text layer");
    return false;
  }

  s_steps_is_available = false;
  s_steps = STEPS_INVALID;
  s_oneshot_steps_is_set = false;
  s_oneshot_steps = STEPS_INVALID;
  // The walking bitmap ships with black foreground pixels. Seed the
  // cached color to that source palette value so the first recolor
  // pass knows which color to replace.
  s_steps_icon_color = GColorBlack;
  // By making these different from the get-go, we ensure that the icon is replaced
  // the first time
  s_steps_color = gcolor_legible_over(s_steps_icon_color);
  s_steps_goal = STEPS_GOAL_DEFAULT;
  s_steps_threshold = HELPER_ROUND_UP((s_steps_goal * STEPS_APPROACHING_GOAL_PERCENT), 100);

  if (icon) {
    uint32_t resource_id = 0;
    resource_id = RESOURCE_ID_WALK;
    s_steps_bitmap = gbitmap_create_with_resource(resource_id);

    if (s_steps_bitmap) {
      s_steps_icon_layer =
        substratum_renderer_create_icon_layer(root, icon, steps_icon_update_proc);
      if (!s_steps_icon_layer) {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Failed to create steps icon layer");
        gbitmap_destroy(s_steps_bitmap);
        s_steps_bitmap = NULL;
      }
    } else {
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Failed to create steps icon bitmap");
    }
  }

  // Handle the progress bar
  GRect frame = *progress;

  // The width of the progress bar is either the width of icon plus text
  // or just the text. Change the width only if the icon is not visible
  if (!(icon)) {
    // The progress bar's width needs to be text_width, so shift the origin of the bar
    int text_w = text->frame.size.w;
    frame.origin.x += (progress->size.w - text_w);
    frame.size.w = text_w;
  }

  // Our frame has the right coordinates now
  s_steps_progress_layer = layer_create(frame);
  if (!s_steps_progress_layer) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Failed to create steps progress layer");
  } else {
    layer_set_update_proc(s_steps_progress_layer, steps_progress_update_proc);
    layer_add_child(root, s_steps_progress_layer);
  }

  return true;
}

void steps_module_destroy() {
  if (s_steps_layer) {
    text_layer_destroy(s_steps_layer);
    s_steps_layer = NULL;
  }

  if (s_steps_bitmap) {
    gbitmap_destroy(s_steps_bitmap);
    s_steps_bitmap = NULL;
  }

  if (s_steps_icon_layer) {
    layer_destroy(s_steps_icon_layer);
    s_steps_icon_layer = NULL;
  }

  if (s_steps_progress_layer) {
    layer_destroy(s_steps_progress_layer);
    s_steps_progress_layer = NULL;
  }

  s_steps_buffer[0] = '\0';
  s_steps = STEPS_INVALID;
  s_steps_is_available = false;
  s_steps_palette = (StepsPalette){0};
  s_steps_icon_color = GColorBlack;
  s_steps_color = WATCHFACE_UNINITIALIZED_TEXT_COLOR;
  s_steps_goal = STEPS_GOAL_DEFAULT;
  s_oneshot_steps_is_set = false;
  s_oneshot_steps = STEPS_INVALID;
}

void steps_module_refresh(
  const ColorPalette* palette,
  uint16_t steps_goal) {
  if (!palette) {
    return;
  }

  s_steps_goal = steps_goal;
  s_steps_threshold = HELPER_ROUND_UP((steps_goal * STEPS_APPROACHING_GOAL_PERCENT), 100);
  steps_update_palette(palette);
  update_steps();
}

void steps_module_oneshot_set_steps(
  int steps) {
  s_oneshot_steps = steps;
  s_oneshot_steps_is_set = true;
}

void steps_module_oneshot_clear_steps() {
  s_oneshot_steps_is_set = false;
  s_oneshot_steps = STEPS_INVALID;
}
#endif
// Health capabilities
