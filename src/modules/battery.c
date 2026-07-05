#include "battery.h"
#include "helper.h"
#include "substratum_renderer.h"

typedef struct {
  GColor background;
  GColor normal;
  GColor medium;
  GColor low;
  GColor pluggedin;
} BatteryPalette;

static BatteryPalette s_battery_palette = {0};

static GRect s_battery_track = {0};
static GRect s_battery_fill = {0};
static GRect s_battery_bolt = {0};
static Layer *s_battery_track_layer = NULL;
static Layer *s_battery_bolt_layer = NULL;

static BatteryChargeState s_battery_state = {0};

static const BatteryPalette c_dark_battery_palette = {
    .medium = PBL_IF_COLOR_ELSE(GColorYellow, GColorWhite),
    .low = PBL_IF_COLOR_ELSE(GColorShockingPink, GColorWhite),
    .pluggedin = PBL_IF_COLOR_ELSE(GColorIslamicGreen, GColorWhite),
};

static const BatteryPalette c_light_battery_palette = {
    .medium = PBL_IF_COLOR_ELSE(GColorVividViolet, GColorBlack),
    .low = PBL_IF_COLOR_ELSE(GColorRed, GColorBlack),
    .pluggedin = PBL_IF_COLOR_ELSE(GColorGreen, GColorBlack),
};

static void battery_update_palette(const ColorPalette *palette);
static GColor calculate_battery_color(int16_t percent);

static void battery_update_palette(const ColorPalette *palette) {
  const BatteryPalette *template =
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

static GColor calculate_battery_color(int16_t percent) {
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

  return s_battery_palette.low;
}

static void battery_track_update_proc(Layer *layer, GContext *ctx);
static void battery_bolt_update_proc(Layer *layer, GContext *ctx);
static void update_battery_state(void);

static void battery_track_update_proc(Layer *layer, GContext *ctx) {
  if (!layer || !ctx || !MODULE_PALETTE_LOADED(s_battery_palette)) {
    return;
  }

  const GRect *track = &s_battery_track;
  const GRect *fill = &s_battery_fill;
  GRect bounds = layer_get_bounds(layer);
  int16_t charge_percent = s_battery_state.charge_percent;
  GColor fill_color = calculate_battery_color(charge_percent);

  // Draw this bounding rectangle in the background color to create richer contrast.
  graphics_context_set_fill_color(ctx, s_battery_palette.background);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  // Now draw the outline of the battery's track.
  graphics_context_set_stroke_color(ctx, fill_color);
  graphics_draw_rect(ctx, bounds);

  // Now fill the inside of the track up to the charge width.
  graphics_context_set_fill_color(ctx, fill_color);
  graphics_fill_rect(ctx,
                     GRect(fill->origin.x - track->origin.x, fill->origin.y - track->origin.y,
                           HELPER_CLAMP_MIN((charge_percent * fill->size.w) / 100, 1),
                           fill->size.h),
                     0, GCornerNone);
}

static void battery_bolt_update_proc(Layer *layer, GContext *ctx) {
  if (!layer || !ctx || !MODULE_PALETTE_LOADED(s_battery_palette)) {
    return;
  }
  GRect bounds = layer_get_bounds(layer);
  substratum_renderer_draw_filled_bolt_in_frame(
      ctx, &bounds, calculate_battery_color(s_battery_state.charge_percent));
}

static void update_battery_state(void) {
  if (!MODULE_PALETTE_LOADED(s_battery_palette) || !s_battery_track_layer) {
    return;
  }

  layer_mark_dirty(s_battery_track_layer);
  if (s_battery_bolt_layer) {
    layer_set_hidden(s_battery_bolt_layer, !(s_battery_state.is_plugged));
    layer_mark_dirty(s_battery_bolt_layer);
  }
}

bool battery_module_create(Layer *root, const WatchfaceBatteryStratum *battery) {
  if (!root || !battery) {
    return false;
  }

  s_battery_track = battery->track;
  s_battery_fill = battery->fill;
  s_battery_bolt = battery->bolt;

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
  }

  return true;
}

void battery_module_destroy(void) {
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
}

void battery_module_refresh(const ColorPalette *palette) {
  if (!palette) {
    return;
  }

  battery_update_palette(palette);
  s_battery_state = battery_state_service_peek();
  update_battery_state();
}
