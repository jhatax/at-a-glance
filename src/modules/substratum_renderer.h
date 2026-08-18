#pragma once

#include <pebble.h>

#include "watchface_components.h"

#define MODULE_PALETTE_LOADED(pal) (!(HELPER_COLOR_EQUAL(((pal).normal), ((pal).background))))

TextLayer* substratum_renderer_create_text_layer(
    Layer* parent,
    const WatchfaceTextSubstratum* text,
    GFont font);

Layer* substratum_renderer_create_icon_layer(
    Layer* parent,
    const WatchfaceIconSubstratum* icon,
    LayerUpdateProc update_proc);

void substratum_renderer_update_text_layer(
    TextLayer* layer,
    const char* text,
    GColor text_color);

GColor substratum_renderer_color_for_role(
    const ColorPalette* palette,
    WatchfaceColorRole role);

void substratum_renderer_draw_scaled_line_in_frame(
    GContext* ctx,
    const GRect* frame,
    int16_t x0,
    int16_t y0,
    int16_t x1,
    int16_t y1);

void substratum_renderer_fill_scaled_rect_from_corners_in_frame(
    GContext* ctx,
    const GRect* frame,
    int16_t x0,
    int16_t y0,
    int16_t x1,
    int16_t y1);

void substratum_renderer_fill_scaled_circle_in_frame(
    GContext* ctx,
    const GRect* frame,
    int16_t x,
    int16_t y,
    int16_t r);

void substratum_renderer_draw_scaled_circle_in_frame(
    GContext* ctx,
    const GRect* frame,
    int16_t x,
    int16_t y,
    int16_t r);

// Use background color to draw a solid line before drawing a dashed-line
void substratum_renderer_mark_info_outofrange(
    GContext* ctx,
    const GRect* frame,
    GColor color,
    GColor background);

void substratum_renderer_draw_filled_bolt_in_frame(
    GContext* ctx,
    const GRect* frame,
    GColor fill_color);
