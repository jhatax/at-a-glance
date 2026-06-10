#pragma once

#include <pebble.h>

#include "watchface_components.h"

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
    const GSize* bounds_size,
    int16_t coord);

int16_t substratum_renderer_scale_icon_y(
    const GSize* bounds_size,
    int16_t coord);

int16_t substratum_renderer_scale_icon_coord(
    const GSize* bounds_size,
    int16_t coord);

GPoint substratum_renderer_scale_icon_point(
    const GSize* bounds_size,
    int16_t x,
    int16_t y);
