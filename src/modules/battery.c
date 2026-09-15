#include "battery.h"

#include "helper_computations.h"
#include "layout_blueprints.h"
#include "substratum_renderer.h"

typedef struct {
  GColor background;
  GColor track;
  GColor normal;
  GColor medium;
  GColor critical;
  GColor pluggedin;
} BatteryPalette;

static BatteryPalette s_battery_palette = {0};

static GRect s_battery_track = {0};
static GRect s_battery_fill = {0};
static GRect s_battery_bolt = {0};
static Layer* s_battery_track_layer = NULL;
static Layer* s_battery_bolt_layer = NULL;

static BatteryChargeState s_battery_state = {0};
static bool s_cached_is_vertical = false;

static const BatteryPalette c_dark_battery_palette = {
    .track = GColorDarkGray,
    .medium = PBL_IF_COLOR_ELSE(GColorYellow, GColorWhite),
    .critical = PBL_IF_COLOR_ELSE(GColorRed, GColorWhite),
    .pluggedin = PBL_IF_COLOR_ELSE(GColorGreen, GColorWhite),
};

static const BatteryPalette c_light_battery_palette = {
    .track = GColorLightGray,
    .medium = PBL_IF_COLOR_ELSE(GColorVividViolet, GColorBlack),
    .critical = PBL_IF_COLOR_ELSE(GColorRed, GColorBlack),
    .pluggedin = PBL_IF_COLOR_ELSE(GColorIslamicGreen, GColorBlack),
};

static void battery_update_palette(const ColorPalette* palette);
static GColor calculate_battery_color(int16_t percent);

static void battery_update_palette(
    const ColorPalette* palette) {
  const BatteryPalette* template =
      palette->is_light_mode ? &c_light_battery_palette : &c_dark_battery_palette;

  if (MODULE_PALETTE_LOADED(s_battery_palette)) {
    // We know that light-mode and dark-mode have different backgrounds
    if (HELPER_COLOR_EQUAL(s_battery_palette.background, palette->background)) {
      // The palette doesn't need to be updated
      return;
    }
  }
  // Two possibilities:
  // 1. First time the palette is being initialized
  // 2. The palette has changed
  s_battery_palette = *template;
  s_battery_palette.background = palette->background;
  s_battery_palette.normal = palette->primary_text;
}

static GColor calculate_battery_color(
    int16_t percent) {
  if (!MODULE_PALETTE_LOADED(s_battery_palette)) {
    return WATCHFACE_UNINITIALIZED_TEXT_COLOR;
  }

  if (s_battery_state.is_plugged) {
    return s_battery_palette.pluggedin;
  }

  if (percent > 50) {
    return s_battery_palette.normal;
  }
  if (percent > 20) {
    return s_battery_palette.medium;
  }

  return s_battery_palette.critical;
}

static void battery_track_update_proc(
    Layer* layer,
    GContext* ctx);
static void battery_bolt_update_proc(
    Layer* layer,
    GContext* ctx);

#ifdef PBL_ROUND
static void update_round_vertical_track_fill(
    Layer* layer,
    GContext* ctx,
    const int16_t charge_percent) {
  (void)layer;
  const GColor fill_color = calculate_battery_color(charge_percent);

  // Let's calculate the top and bottom angle if this were to be a full battery
  const int16_t pie_slice = HELPER_SCALE_ROUND(BATTERY_BAR_SIZE_PERCENT, 180, 100);
  const uint16_t deg_offset = (180 - pie_slice) >> 1;
  const uint16_t bottom = 180 - deg_offset;
  uint16_t top = deg_offset;
  // Erase the battery background so that the latest percent is displayed
  graphics_context_set_fill_color(ctx, s_battery_palette.track);
  graphics_fill_radial(
      ctx,
      s_battery_track,
      GOvalScaleModeFitCircle,
      BATTERY_TRACK_HEIGHT,
      DEG_TO_TRIGANGLE(top),
      DEG_TO_TRIGANGLE(bottom));

  // Now fill the inside of the track up to the charge width.
  // Calculate the top based on the current charge_percent
  top += HELPER_SCALE_ROUND(pie_slice, (100 - charge_percent), 100);
  graphics_context_set_fill_color(ctx, fill_color);
  graphics_fill_radial(
      ctx,
      s_battery_fill,
      GOvalScaleModeFitCircle,
      BATTERY_FILL_HEIGHT,
      DEG_TO_TRIGANGLE(top),
      DEG_TO_TRIGANGLE(bottom));
}
#endif

static void battery_track_update_proc(
    Layer* layer,
    GContext* ctx) {
  if (!layer || !ctx || !MODULE_PALETTE_LOADED(s_battery_palette)) {
    return;
  }

  int16_t y = 0;
  int16_t w = 0;
  int16_t h = 0;
  // Set y, w, h for each orientation => x is the same
  const int16_t charge_percent = s_battery_state.charge_percent;
  if (s_cached_is_vertical) {
#ifdef PBL_ROUND
    // If it is a round device that needs a vertical battery,
    // call the update function for round devices and return to caller
    update_round_vertical_track_fill(layer, ctx, charge_percent);
    return;
#else
    h = HELPER_CLAMP_MIN(HELPER_SCALE_ROUND(charge_percent, s_battery_fill.size.h, 100), 1);
    y = BATTERY_TRACK_FILL_OFFSET + (s_battery_fill.size.h - h);
    w = s_battery_fill.size.w;
#endif
  } else {
    // Handle horizontal battery updates for round & rect devices
    y = BATTERY_TRACK_FILL_OFFSET;
    w = HELPER_CLAMP_MIN(HELPER_SCALE_ROUND(charge_percent, s_battery_fill.size.w, 100), 1);
    h = s_battery_fill.size.h;
  }
  const GRect bounds = layer_get_bounds(layer);
  const GColor fill_color = calculate_battery_color(charge_percent);

  // Draw the battery track
  graphics_context_set_fill_color(ctx, s_battery_palette.track);
  graphics_fill_rect(ctx, bounds, 2, GCornerNone);

  graphics_context_set_fill_color(ctx, fill_color);
  graphics_fill_rect(ctx, GRect(BATTERY_TRACK_FILL_OFFSET, y, w, h), 2, GCornerNone);
}

static void battery_bolt_update_proc(
    Layer* layer,
    GContext* ctx) {
  if (!layer || !ctx || !MODULE_PALETTE_LOADED(s_battery_palette)) {
    return;
  }
  GRect bounds = layer_get_bounds(layer);
  substratum_renderer_draw_filled_bolt_in_frame(ctx, &bounds, s_battery_palette.pluggedin);
}

bool battery_module_create(
    Layer* root,
    const WatchfaceBatteryStratum* battery) {
  if (!root || !battery) {
    return false;
  }

  s_battery_track = battery->track;
  s_battery_fill = battery->fill;
  s_battery_bolt = battery->bolt;
  s_cached_is_vertical = battery->is_vertical;

  s_battery_track_layer = layer_create(s_battery_track);
  if (!s_battery_track_layer) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to create battery track layer");
    return false;
  }
  layer_set_update_proc(s_battery_track_layer, battery_track_update_proc);
  layer_add_child(root, s_battery_track_layer);

  s_battery_state = battery_state_service_peek();

  s_battery_bolt_layer = layer_create(s_battery_bolt);
  if (s_battery_bolt_layer) {
    layer_set_update_proc(s_battery_bolt_layer, battery_bolt_update_proc);
    layer_add_child(root, s_battery_bolt_layer);
  } else {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to create battery bolt layer");
    return false;
  }

  return true;
}

void battery_module_destroy() {
  if (s_battery_bolt_layer) {
    layer_destroy(s_battery_bolt_layer);
    s_battery_bolt_layer = NULL;
  }

  if (s_battery_track_layer) {
    layer_destroy(s_battery_track_layer);
    s_battery_track_layer = NULL;
  }

  s_battery_state = (BatteryChargeState){0};
  s_battery_palette = (BatteryPalette){0};
  s_battery_track = GRectZero;
  s_battery_fill = GRectZero;
  s_battery_bolt = GRectZero;
  s_cached_is_vertical = false;
}

void battery_module_refresh(
    const ColorPalette* palette,
    WatchfaceBatteryStratum* battery) {
  if (!palette) {
    return;
  }
  battery_update_palette(palette);
  // The orientation has changed; update our state variables and reset layer frames
  if (s_cached_is_vertical != battery->is_vertical) {
    s_cached_is_vertical = battery->is_vertical;
    s_battery_fill = battery->fill;
    s_battery_track = battery->track;
    layer_set_frame(s_battery_track_layer, battery->track);
    layer_set_frame(s_battery_bolt_layer, battery->bolt);
  }
  s_battery_state = battery_state_service_peek();
  if (s_battery_bolt_layer) {
    layer_set_hidden(s_battery_bolt_layer, !(s_battery_state.is_plugged));
  }
  layer_mark_dirty(s_battery_bolt_layer);
  layer_mark_dirty(s_battery_track_layer);
}
