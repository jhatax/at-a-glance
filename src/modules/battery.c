#include "battery.h"
#include "../c/ataglance.h"

static char s_battery_buffer[ATAGLANCE_MAX_STR_LEN];

static Layer* s_battery_icon_layer;
static TextLayer* s_battery_layer;
static BatteryChargeState s_battery_state;

static const VisualPalette* s_palette;

static inline TextLayer* create_battery_text_layer(
    Layer* parent,
    const GRect* frame,
    GFont font);
static inline GColor get_battery_color_from_state(void);
static void draw_battery_charging_bolt(
    GContext* ctx,
    const GSize* bounds_size,
    GColor color);
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
        display_legible_over_background(s_palette));
  }

  if (percent > 50) {
    return PBL_IF_COLOR_ELSE(
        GColorCobaltBlue,
        display_legible_over_background(s_palette));
  }
  if (percent > 20) {
    return PBL_IF_COLOR_ELSE(
        GColorRajah,
        display_legible_over_background(s_palette));
  }
  return PBL_IF_COLOR_ELSE(
      GColorRed,
      display_legible_over_background(s_palette));
}

static void draw_battery_charging_bolt(
    GContext* ctx,
    const GSize* bounds_size,
    GColor color) {
  graphics_context_set_stroke_color(ctx, s_palette->background);
  graphics_context_set_stroke_width(ctx, 1);
  layout_draw_scaled_icon_line(ctx, bounds_size, 17, 3, 10, 14);
  layout_draw_scaled_icon_line(ctx, bounds_size, 10, 14, 16, 14);
  layout_draw_scaled_icon_line(ctx, bounds_size, 16, 14, 11, 25);
  layout_draw_scaled_icon_line(ctx, bounds_size, 11, 25, 22, 11);
  layout_draw_scaled_icon_line(ctx, bounds_size, 22, 11, 16, 11);
  layout_draw_scaled_icon_line(ctx, bounds_size, 16, 11, 17, 3);

  graphics_context_set_stroke_color(ctx, color);
  graphics_context_set_stroke_width(ctx, 2);
  layout_draw_scaled_icon_line(ctx, bounds_size, 17, 3, 10, 14);
  layout_draw_scaled_icon_line(ctx, bounds_size, 10, 14, 16, 14);
  layout_draw_scaled_icon_line(ctx, bounds_size, 16, 14, 11, 25);
  layout_draw_scaled_icon_line(ctx, bounds_size, 11, 25, 22, 11);
  layout_draw_scaled_icon_line(ctx, bounds_size, 22, 11, 16, 11);
  layout_draw_scaled_icon_line(ctx, bounds_size, 16, 11, 17, 3);
}

static void battery_icon_update_proc(Layer* layer, GContext* ctx) {
  if (!layer || !ctx || !s_palette) {
    return;
  }

  GRect bounds = layer_get_bounds(layer);
  const int stroke_width = 2;
  int percent = s_battery_state.charge_percent;
  GColor draw_color = get_battery_color_from_state();
  int fill_w = (percent * 18) / 100;

  graphics_context_set_fill_color(ctx, s_palette->background);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  graphics_context_set_stroke_color(ctx, draw_color);
  graphics_context_set_fill_color(ctx, draw_color);
  graphics_context_set_stroke_width(ctx, stroke_width);

  graphics_draw_rect(
      ctx,
      GRect(layout_scale_icon_x(&bounds.size, 2),
            layout_scale_icon_y(&bounds.size, 8),
            layout_scale_icon_x(&bounds.size, 22),
            layout_scale_icon_y(&bounds.size, 13)));
  graphics_fill_rect(
      ctx,
      GRect(layout_scale_icon_x(&bounds.size, 24),
            layout_scale_icon_y(&bounds.size, 12),
            layout_scale_icon_x(&bounds.size, 2),
            layout_scale_icon_y(&bounds.size, 5)),
      0,
      GCornerNone);

  if (fill_w < 1) {
    fill_w = 1;
  }

  graphics_fill_rect(
      ctx,
      GRect(layout_scale_icon_x(&bounds.size, 5),
            layout_scale_icon_y(&bounds.size, 11),
            layout_scale_icon_x(&bounds.size, fill_w),
            layout_scale_icon_y(&bounds.size, 7)),
      0,
      GCornerNone);

  if (s_battery_state.is_charging) {
    draw_battery_charging_bolt(ctx, &bounds.size, draw_color);
  }
}

static void update_battery(void) {
  if (!s_battery_layer || !s_palette) {
    return;
  }

  snprintf(
      s_battery_buffer,
      ATAGLANCE_MAX_STR_LEN,
      "%d%%",
      s_battery_state.charge_percent);

  display_update_text_layer(
      s_battery_layer,
      s_battery_buffer,
      get_battery_color_from_state());

  if (s_battery_icon_layer) {
    layer_mark_dirty(s_battery_icon_layer);
  }
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

  s_battery_layer = create_battery_text_layer(
      root,
      &layout->battery_text_frame,
      value_font);

  if (s_battery_layer) {
    s_battery_icon_layer = layer_create(layout->battery_icon_frame);
    if (s_battery_icon_layer) {
      layer_set_update_proc(
          s_battery_icon_layer,
          battery_icon_update_proc);
      layer_add_child(root, s_battery_icon_layer);
    }
  }
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
