#include "layout.h"
#include "helper.h"

void layout_calculate(
    int16_t face_width,
    int16_t face_height,
    WatchfaceLayout* layout) {
  if (!layout) {
    return;
  }

  #if defined(PBL_RECT)
  const int layout_spacing = PBL_DISPLAY_HEIGHT / 28;
  #else
  const int layout_spacing = 8;
  #endif
  const int content_x = 2;
  const int row_gap = layout_spacing;
  const int column_gap = layout_spacing;
  const int icon_text_gap = 2;
  const int content_width = face_width - (2 * content_x);
  const int rule_y = face_height / 2;
  const int rule_left = content_x;
  const int rule_right = face_width - content_x;

  const int date_height = 20;

  const int time_layer_height = 48;
  const int time_layer_top = rule_y - row_gap - time_layer_height;

  const int date_top = row_gap;
  const int metrics_row_top = rule_y + row_gap;
  const int bottom_text_height = 20;
  const int bottom_row_top =
      face_height - row_gap - bottom_text_height;
  const int icon_size = HELPER_ICON_GRID_SIZE;
  const int metrics_text_height = 20;
  const int metrics_left_text_width = 28;
  const int metrics_right_text_width = 40;
  const int bottom_row_left_text_width = 40;
  const int bottom_row_right_text_width = 34;
  const int metrics_icon_top =
      metrics_row_top + ((metrics_text_height - icon_size) / 2);
  const int bottom_icon_top =
      bottom_row_top + ((bottom_text_height - icon_size) / 2);

  const int left_icon_x = content_x;
  const int left_value_x = left_icon_x + icon_size + icon_text_gap;
  const int right_value_x =
      face_width - content_x - metrics_right_text_width;
  const int right_icon_x = right_value_x - icon_text_gap - icon_size;
  const int weather_icon_x = content_x;
  const int temperature_value_x =
      weather_icon_x + icon_size + icon_text_gap;
  const int bottom_row_right_value_x =
      face_width - content_x - bottom_row_right_text_width;
  const int bottom_row_right_icon_x =
      bottom_row_right_value_x - icon_text_gap - icon_size;

  layout->background_frame = GRect(0, 0, face_width, face_height);
  layout->rule_left = rule_left;
  layout->rule_y = rule_y;
  layout->rule_right = rule_right;
  layout->content_x = content_x;
  layout->row_gap = row_gap;
  layout->column_gap = column_gap;
  layout->content_width_date = content_width;
  layout->content_width_time = content_width;
  layout->content_width_health = content_width;
  layout->content_width_bottom = content_width;
  layout->date_frame = GRect(content_x,
                            date_top,
                            layout->content_width_date,
                            date_height);
  layout->time_frame = GRect(content_x,
                            time_layer_top,
                            layout->content_width_time,
                            time_layer_height);
  layout->bpm_icon_frame = GRect(left_icon_x,
                                metrics_icon_top,
                                icon_size,
                                icon_size);
  layout->bpm_text_frame = GRect(left_value_x,
                                metrics_row_top,
                                metrics_left_text_width,
                                metrics_text_height);
  layout->steps_icon_frame = GRect(right_icon_x,
                                  metrics_icon_top,
                                  icon_size,
                                  icon_size);
  layout->steps_text_frame = GRect(right_value_x,
                                  metrics_row_top,
                                  metrics_right_text_width,
                                  metrics_text_height);
  layout->weather_icon_frame = GRect(weather_icon_x,
                                     bottom_icon_top,
                                     icon_size,
                                     icon_size);
  layout->temp_text_frame = GRect(temperature_value_x,
                                 bottom_row_top,
                                 bottom_row_left_text_width,
                                 bottom_text_height);
  layout->battery_icon_frame = GRect(bottom_row_right_icon_x,
                                    bottom_icon_top,
                                    icon_size,
                                    icon_size);
  layout->battery_text_frame = GRect(bottom_row_right_value_x,
                                    bottom_row_top,
                                    bottom_row_right_text_width,
                                    bottom_text_height);
}
