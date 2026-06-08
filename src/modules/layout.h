#pragma once

#include <pebble.h>

typedef struct {
  GRect background_frame;
  GRect date_frame;
  GRect time_frame;
  GRect bpm_icon_frame;
  GRect bpm_text_frame;
  GRect steps_icon_frame;
  GRect steps_text_frame;
  GRect weather_icon_frame;
  GRect temp_text_frame;
  GRect battery_icon_frame;
  GRect battery_text_frame;
  int16_t rule_left;
  int16_t rule_y;
  int16_t rule_right;
  int16_t content_x;
  int16_t row_gap;
  int16_t column_gap;
  int16_t content_width_date;
  int16_t content_width_time;
  int16_t content_width_health;
  int16_t content_width_bottom;
} WatchfaceLayout;

void layout_calculate(
    int16_t face_width,
    int16_t face_height,
    WatchfaceLayout* layout);

int16_t layout_scale_icon_x(const GSize* bounds_size, int16_t coord);

int16_t layout_scale_icon_y(const GSize* bounds_size, int16_t coord);

int16_t layout_scale_icon_coord(
    const GSize* bounds_size,
    int16_t coord);

GPoint layout_scaled_icon_point(
    const GSize* bounds_size,
    int16_t x,
    int16_t y);
