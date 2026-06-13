#pragma once

#include <pebble.h>

#include "watchface_components.h"

#define SUBSTRATUM_RENDERER_ICON_STROKE_WIDTH(frame_min) \
  (((frame_min) < 20) ? 1 : 2)

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

int16_t substratum_renderer_scale_icon_x(
    const GSize* size,
    int16_t coord);

int16_t substratum_renderer_scale_icon_y(
    const GSize* size,
    int16_t coord);

int16_t substratum_renderer_scale_icon_coord(
    const GSize* size,
    int16_t coord);

GPoint substratum_renderer_scale_icon_point(
    const GSize* size,
    int16_t x,
    int16_t y);

void substratum_renderer_draw_scaled_polygon_outline(
    GContext* ctx,
    const GSize* size,
    const GPoint* design_points,
    uint32_t point_count,
    GColor stroke_color,
    GColor halo_color,
    int16_t stroke_width,
    bool draw_halo);

void substratum_renderer_draw_scaled_line(
    GContext* ctx,
    const GSize* size,
    int16_t x0,
    int16_t y0,
    int16_t x1,
    int16_t y1);

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

void substratum_renderer_draw_unavailable_slash(
    GContext* ctx,
    const GSize* size,
    GColor color);
