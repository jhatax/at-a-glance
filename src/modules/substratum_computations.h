#pragma once

#include <pebble.h>

#include "helper.h"
#include "layout_blueprints.h"

#define SUBSTRATUM_VALID_DESIGN_X(x) HELPER_VALUE_IN_RANGE((x), 0, (WATCHFACE_ICON_GRID_WIDTH - 1))
#define SUBSTRATUM_VALID_DESIGN_Y(y) HELPER_VALUE_IN_RANGE((y), 0, (WATCHFACE_ICON_GRID_HEIGHT - 1))
#define SUBSTRATUM_RENDERER_ICON_STROKE_WIDTH(frame_min, stroke_w) \
  HELPER_CLAMP_MIN((HELPER_IF_ELSE(((frame_min) < 20), ((stroke_w) - 1), (stroke_w))), 1)

int16_t substratum_renderer_scale_icon_x(
    const GSize* size,
    int16_t coord);

int16_t substratum_renderer_scale_icon_x_in_frame(
    const GRect* frame,
    int16_t x);

int16_t substratum_renderer_scale_icon_y(
    const GSize* size,
    int16_t coord);

int16_t substratum_renderer_scale_icon_y_in_frame(
    const GRect* frame,
    int16_t y);

int16_t substratum_renderer_scale_icon_coord(
    const GSize* size,
    int16_t coord);

GPoint substratum_renderer_scale_icon_x_y_in_frame(
    const GRect* frame,
    int16_t x,
    int16_t y);

GPoint substratum_renderer_scale_icon_point(
    const GSize* size,
    int16_t x,
    int16_t y);

void substratum_renderer_scale_icon_point_in_frame(
    const GRect* frame,
    GPoint* input);

void substratum_renderer_create_subframe(
    const GRect* frame,
    GRect* out,
    int16_t x,
    int16_t y,
    int16_t w,
    int16_t h);
