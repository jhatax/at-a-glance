#pragma once

#include <pebble.h>

#include "glyph_lab_components.h"

bool glyph_lab_glyphs_init(void);
void glyph_lab_glyphs_deinit(void);

void glyph_lab_select_palette(
    ColorPalette* palette,
    bool is_light_mode);

void glyph_lab_draw_battery_icon(
    GContext* ctx,
    const GRect* frame,
    const ColorPalette* palette,
    bool is_light_mode,
    int percent,
    bool is_charging);

void glyph_lab_draw_bolt_icon(
    GContext* ctx,
    const GRect* frame,
    const ColorPalette* palette,
    GColor color);

void glyph_lab_draw_charge_icon(
    GContext* ctx,
    const GRect* frame,
    const ColorPalette* palette,
    int variant);

void glyph_lab_draw_steps_icon(
    GContext* ctx,
    const GRect* frame,
    const ColorPalette* palette,
    bool is_available);

void glyph_lab_draw_steps_bitmap_icon(
    GContext* ctx,
    const GRect* frame,
    const ColorPalette* palette,
    int bitmap_kind);

void glyph_lab_draw_bpm_icon(
    GContext* ctx,
    const GRect* frame,
    const ColorPalette* palette,
    bool is_light_mode,
    int bpm,
    bool is_available);

void glyph_lab_draw_climate_icon(
    GContext* ctx,
    const GRect* frame,
    int16_t weather_condition,
    const ColorPalette* palette);
