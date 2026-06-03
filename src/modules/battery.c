#include "battery.h"
#include "helper.h"
#include "../c/ataglance.h"

static char s_battery_buffer[MAX_STR_LEN];

static Layer* s_battery_icon_layer;
static TextLayer* s_battery_layer;
static BatteryChargeState s_battery_state;

static const VisualPalette* s_palette;

static inline TextLayer* create_battery_text_layer(
    Layer* parent,
    const GRect* frame,
    GFont font);
static inline GColor get_battery_color_from_state(void);
static void battery_icon_update_proc(Layer* layer, GContext* ctx);
static void update_battery(void);

static inline TextLayer* create_battery_text_layer(
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

static inline GColor get_battery_color_from_state(void) {
  int percent = s_battery_state.charge_percent;

  if (s_battery_state.is_charging) {
    return PBL_IF_COLOR_ELSE(
        GColorJaegerGreen,
        s_palette->primary_text);
  }

  if (percent > 50) {
    return PBL_IF_COLOR_ELSE(GColorCobaltBlue, s_palette->primary_text);
  }
  if (percent > 20) {
    return PBL_IF_COLOR_ELSE(GColorYellow, s_palette->primary_text);
  }
  return PBL_IF_COLOR_ELSE(GColorRed, s_palette->primary_text);
}

static void battery_icon_update_proc(Layer* layer, GContext* ctx) {
  if (!layer || !ctx || !s_palette) {
    return;
  }

  GRect bounds = layer_get_bounds(layer);
  const int stroke_width = 2;
  int body_w = helper_scale_icon_x(&bounds.size, 28);
  int body_h = helper_scale_icon_y(&bounds.size, 20);
  int body_x = (bounds.size.w - body_w) / 2;
  int body_y = bounds.size.h - body_h - stroke_width;
  int fill_inset_x = helper_scale_icon_x(&bounds.size, 2);
  int fill_inset_y = helper_scale_icon_y(&bounds.size, 2);
  int fill_x = body_x + fill_inset_x;
  int fill_y = body_y + fill_inset_y;
  int max_fill_w = body_w - (2 * fill_inset_x);
  int fill_h = body_h - (2 * fill_inset_y);
  int percent = s_battery_state.charge_percent;
  GColor draw_color = get_battery_color_from_state();

  graphics_context_set_fill_color(ctx, s_palette->background);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  graphics_context_set_stroke_color(ctx, draw_color);
  graphics_context_set_stroke_width(ctx, stroke_width);

  graphics_draw_rect(ctx, GRect(body_x, body_y, body_w, body_h));

  int fill_w = ((percent * max_fill_w) / 100) + 1;
  if (fill_w > max_fill_w) {
    fill_w = max_fill_w;
  }

  graphics_context_set_fill_color(ctx, draw_color);
  graphics_fill_rect(ctx, GRect(fill_x, fill_y, fill_w, fill_h),
                     0, GCornerNone);
}

static void update_battery(void) {
  if (!s_battery_layer || !s_battery_icon_layer || !s_palette) {
    return;
  }

  snprintf(
      s_battery_buffer,
      MAX_STR_LEN,
      "%d%%",
      s_battery_state.charge_percent);

  display_update_text_layer(
      s_battery_layer,
      s_battery_buffer,
      get_battery_color_from_state());
  layer_mark_dirty(s_battery_icon_layer);
}

void battery_module_create(
    Layer* root,
    const WatchfaceLayout* layout,
    GFont value_font,
    const VisualPalette* palette) {
  if (!root || !layout || !palette) {
    return;
  }

  s_palette = palette;
  s_battery_state = battery_state_service_peek();

  s_battery_icon_layer = layer_create(layout->battery_icon_frame);
  if (s_battery_icon_layer) {
    layer_set_update_proc(
        s_battery_icon_layer,
        battery_icon_update_proc);
    layer_add_child(root, s_battery_icon_layer);
  }

  s_battery_layer = create_battery_text_layer(
      root,
      &layout->battery_text_frame,
      value_font);
}

void battery_module_destroy(void) {
  if (s_battery_icon_layer) {
    layer_destroy(s_battery_icon_layer);
    s_battery_icon_layer = NULL;
  }

  if (s_battery_layer) {
    text_layer_destroy(s_battery_layer);
    s_battery_layer = NULL;
  }

  s_battery_buffer[0] = '\0';
}

void battery_module_refresh(const VisualPalette* palette) {
  if (palette) {
    s_palette = palette;
  }

  s_battery_state = battery_state_service_peek();
  update_battery();
}

void battery_module_set_state(const BatteryChargeState* state) {
  if (!state) {
    return;
  }

  s_battery_state = *state;
  update_battery();
}
