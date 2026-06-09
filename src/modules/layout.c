#include "layout.h"
#include "helper.h"
#include "../c/ataglance.h"

// Local functions
static int16_t layout_scale_coord_y(
    int16_t design_y,
    int16_t face_height) {
  return helper_scale_round(
      design_y,
      face_height,
      ATAGLANCE_DESIGN_FACE_HEIGHT);
}

static int16_t layout_scale_coord_x(
    int16_t design_x,
    int16_t face_width) {
  return helper_scale_round(
      design_x,
      face_width,
      ATAGLANCE_DESIGN_FACE_WIDTH);
}

static int16_t layout_scale_height(
    int16_t design_widget_height,
    int16_t current_face_height) {
  return helper_scale_round(
      design_widget_height,
      current_face_height,
      ATAGLANCE_DESIGN_FACE_HEIGHT);
}

static int16_t layout_scale_width(
    int16_t design_widget_width,
    int16_t current_face_width) {
  return helper_scale_round(
      design_widget_width,
      current_face_width,
      ATAGLANCE_DESIGN_FACE_WIDTH);
}

static inline bool is_valid_design_x_coord(int16_t x, int16_t design_width) {
  return (x >=0 && x <=design_width);
}

static inline bool is_valid_design_y_coord(int16_t y, int16_t design_height) {
  return (y >=0 && y <=design_height);
}

// Module APIs
inline int16_t layout_scale_icon_x(
  const GSize* size,
  int16_t coord) {
  if (!size || !is_valid_design_x_coord(coord, ATAGLANCE_DESIGN_ICON_WIDTH)) {
    return 0;
  }

  return helper_scale_round(
      coord,
      size->w,
      ATAGLANCE_DESIGN_ICON_WIDTH);
}

inline int16_t layout_scale_icon_y(
  const GSize* size,
  int16_t coord) {
  if (!size || !is_valid_design_y_coord(coord, ATAGLANCE_DESIGN_ICON_HEIGHT)) {
    return 0;
  }

  return helper_scale_round(
      coord,
      size->h,
      ATAGLANCE_DESIGN_ICON_HEIGHT
  );
}

int16_t layout_scale_icon_coord(const GSize* size, int16_t coord) {
  if (!size) {
    return 0;
  }

  if (!(is_valid_design_x_coord(coord, ATAGLANCE_DESIGN_ICON_WIDTH)
    || is_valid_design_y_coord(coord, ATAGLANCE_DESIGN_ICON_HEIGHT))) {
    return 0;
  }

  int16_t chosen_dimension = helper_min(size->w, size->h);
  int16_t design_dimension = (chosen_dimension == size->w) ?
    ATAGLANCE_DESIGN_ICON_WIDTH : ATAGLANCE_DESIGN_ICON_HEIGHT;
  return helper_scale_round(coord, chosen_dimension, design_dimension);
}

inline GPoint layout_scaled_icon_point(const GSize* size, int16_t x, int16_t y) {
  // 1. Guard Clause: Invalid coordinates always fall back to (0,0)
 if (!(is_valid_design_x_coord(x, ATAGLANCE_DESIGN_ICON_WIDTH) &&
   is_valid_design_y_coord(y, ATAGLANCE_DESIGN_ICON_HEIGHT))) {
   return GPoint(0, 0);
 }

 // 2. Rule: If size is provided, calculate the scaled point
 if (size) {
   return GPoint(
       layout_scale_icon_x(size, x),
       layout_scale_icon_y(size, y)
   );
 }

 // 3. Rule: If size is NULL (and coordinates are valid), return unscaled point
 return GPoint(x, y);
}

void layout_calculate(int16_t face_width, int16_t face_height, WatchfaceLayout* layout) {
  if (!layout) {
    return;
  }

  #ifdef PBL_RECT
  const int16_t x_content_start = layout_scale_coord_x(
      ATAGLANCE_DESIGN_CONTENT_MARGIN,
      face_width);
  const int16_t x_content_end = face_width - x_content_start;
  const int16_t y_content_start = layout_scale_coord_y(
      ATAGLANCE_DESIGN_CONTENT_MARGIN,
      face_height);
  const int16_t y_content_end = face_height - y_content_start;
  const int16_t content_width = face_width - (2 * x_content_start);
  const int16_t display_center = face_height / 2;
  const int16_t rule_left = x_content_start;
  const int16_t rule_right = x_content_end;
  #else
  // Keeping these defaults for now, might change for circular displays
  const int16_t x_content_start = ATAGLANCE_DESIGN_CONTENT_MARGIN;
  const int16_t y_content_start = ATAGLANCE_DESIGN_CONTENT_MARGIN;
  const int16_t y_content_end = face_height - y_content_start;
  const int16_t content_width = face_width - (2 * x_content_start);
  const int16_t display_center = face_height / 2;
  const int16_t rule_left = x_content_start;
  const int16_t rule_right = face_width - x_content_start;
  #endif

  // Scale all of these based on face_height
  // Row and column gaps
  const int16_t row_gap = layout_scale_height(ATAGLANCE_DESIGN_ROW_GAP, face_height);
  const int16_t column_gap = layout_scale_width(ATAGLANCE_DESIGN_COLUMN_GAP, face_width);

  // Icons
  const int16_t icon_h = layout_scale_height(ATAGLANCE_DESIGN_ICON_HEIGHT, face_height);
  const int16_t icon_w = layout_scale_width(ATAGLANCE_DESIGN_ICON_WIDTH, face_width);
  const int16_t icon_text_gap = layout_scale_width(ATAGLANCE_DESIGN_ICON_TEXT_GAP, face_width);

  // Row-heights & widths
  // Top-Row: Date
  const int16_t date_row_height = layout_scale_height(ATAGLANCE_DESIGN_DATE_ROW_HEIGHT, face_height);
  const int16_t data_layer_width = layout_scale_width(ATAGLANCE_DESIGN_DATA_FIELD_WIDTH, face_width);
  const int16_t metrics_left_text_width = data_layer_width;

  // Second-Row: Time
  const int16_t time_row_height = layout_scale_height(ATAGLANCE_DESIGN_TIME_ROW_HEIGHT, face_height);
  const int16_t metrics_right_text_width = data_layer_width;

  // Third-Row: Health Metrics
  const int16_t metrics_text_height = layout_scale_height(ATAGLANCE_DESIGN_HEALTH_ROW_HEIGHT, face_height);
  const int16_t metrics_row_height = helper_max(icon_h, metrics_text_height);
  const int16_t bottom_row_left_text_width = data_layer_width;

  // Fourth-Row: Temp & Battery
  const int16_t bottom_text_height = layout_scale_height(ATAGLANCE_DESIGN_BOTTOM_ROW_HEIGHT, face_height);
  const int16_t bottom_row_height = helper_max(icon_h, bottom_text_height);
  const int16_t bottom_row_right_text_width = data_layer_width;

  // Positions
  // Date Row
  const int16_t date_row_top = y_content_start;

  // Time Row
  const int16_t time_row_top = display_center - row_gap - time_row_height;

  // Metrics Row
  const int16_t metrics_row_top = display_center + row_gap;
  const int16_t metrics_icon_top = metrics_row_top + ((metrics_row_height - icon_h) / 2);
  const int16_t metrics_text_top = metrics_row_top + ((metrics_row_height - metrics_text_height) / 2);
  // Metrics: BPM
  const int16_t bpm_icon_x = x_content_start;
  const int16_t bpm_value_x = bpm_icon_x + icon_w + icon_text_gap;
  // Metrics: Steps
  const int16_t steps_value_x = x_content_end - metrics_right_text_width;
  const int16_t steps_icon_x = steps_value_x - icon_text_gap - icon_w;

  // Bottom Row
  const int16_t bottom_row_top = y_content_end - bottom_row_height;
  const int16_t bottom_icon_top = bottom_row_top + ((bottom_row_height - icon_h) / 2);
  const int16_t bottom_text_top = bottom_row_top + ((bottom_row_height - bottom_text_height) / 2);
  // Bottom: Weather + Temperature
  const int16_t weather_icon_x = x_content_start;
  const int16_t temperature_value_x = weather_icon_x + icon_w + icon_text_gap;
  // Bottom: Battery %
  const int16_t battery_value_x = x_content_end - bottom_row_right_text_width;
  const int16_t battery_icon_x = battery_value_x - icon_text_gap - icon_w;

  layout->face_width = face_width;
  layout->face_height = face_height;
  layout->background_frame = GRect(0, 0, face_width, face_height);
  layout->rule_left = rule_left;
  layout->rule_y = display_center;
  layout->rule_right = rule_right;
  layout->content_x = x_content_start;
  layout->row_gap = row_gap;
  layout->column_gap = column_gap;
  layout->content_width_date = content_width;
  layout->content_width_time = content_width;
  layout->content_width_health = content_width;
  layout->content_width_bottom = content_width;
  layout->date_frame = GRect(x_content_start,
                            date_row_top,
                            layout->content_width_date,
                            date_row_height);
  layout->time_frame = GRect(x_content_start,
                            time_row_top,
                            layout->content_width_time,
                            time_row_height);
  layout->bpm_icon_frame = GRect(bpm_icon_x,
                                metrics_icon_top,
                                icon_w,
                                icon_h);
  layout->bpm_text_frame = GRect(bpm_value_x,
                                metrics_text_top,
                                metrics_left_text_width,
                                metrics_text_height);
  layout->steps_icon_frame = GRect(steps_icon_x,
                                  metrics_icon_top,
                                  icon_w,
                                  icon_h);
  layout->steps_text_frame = GRect(steps_value_x,
                                  metrics_text_top,
                                  metrics_right_text_width,
                                  metrics_text_height);
  layout->weather_icon_frame = GRect(weather_icon_x,
                                     bottom_icon_top,
                                     icon_w,
                                     icon_h);
  layout->temp_text_frame = GRect(temperature_value_x,
                                 bottom_text_top,
                                 bottom_row_left_text_width,
                                 bottom_text_height);
  layout->battery_icon_frame = GRect(battery_icon_x,
                                    bottom_icon_top,
                                    icon_w,
                                    icon_h);
  layout->battery_text_frame = GRect(battery_value_x,
                                    bottom_text_top,
                                    bottom_row_right_text_width,
                                    bottom_text_height);
}
