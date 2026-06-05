#include "layout.h"
#include "helper.h"

static int16_t layout_scale_coord_y(
    int16_t design_y,
    int16_t face_height) {
  return helper_scale_round(design_y, face_height, DESIGN_FACE_HEIGHT);
}

static int16_t layout_scale_coord_x(
    int16_t design_x,
    int16_t face_width) {
  return helper_scale_round(design_x, face_width, DESIGN_FACE_WIDTH);
}

static int16_t layout_scale_height(
    int16_t design_height,
    int16_t face_height) {
  return helper_scale_round(
      design_height,
      face_height,
      DESIGN_FACE_HEIGHT);
}

static int16_t layout_scale_width(
    int16_t design_width,
    int16_t face_width) {
  return helper_scale_round(design_width, face_width, DESIGN_FACE_WIDTH);
}

int16_t layout_scale_icon_x(const GSize* bounds_size, int16_t coord) {
  if (!bounds_size) {
    return 0;
  }

  return helper_scale_round(coord, bounds_size->w, DESIGN_ICON_SIZE);
}

int16_t layout_scale_icon_y(const GSize* bounds_size, int16_t coord) {
  if (!bounds_size) {
    return 0;
  }

  return helper_scale_round(coord, bounds_size->h, DESIGN_ICON_SIZE);
}

int16_t layout_scale_icon_coord(
    const GSize* bounds_size,
    int16_t coord) {
  if (!bounds_size) {
    return 0;
  }

  return helper_scale_round(
      coord,
      helper_min(bounds_size->w, bounds_size->h),
      DESIGN_ICON_SIZE);
}

GPoint layout_scaled_icon_point(
    const GSize* bounds_size,
    int16_t x,
    int16_t y) {
  if (!bounds_size) {
    return GPoint(x, y);
  }

  return GPoint(
      layout_scale_icon_x(bounds_size, x),
      layout_scale_icon_y(bounds_size, y));
}

void layout_draw_scaled_icon_line(
    GContext* ctx,
    const GSize* bounds_size,
    int16_t x0,
    int16_t y0,
    int16_t x1,
    int16_t y1) {
  if (!ctx || !bounds_size) {
    return;
  }

  graphics_draw_line(
      ctx,
      layout_scaled_icon_point(bounds_size, x0, y0),
      layout_scaled_icon_point(bounds_size, x1, y1));
}

void layout_calculate(
    int16_t face_width,
    int16_t face_height,
    WatchfaceLayout* layout) {
  if (!layout) {
    return;
  }

  #if defined(PBL_RECT)
  const int16_t x_content_start = layout_scale_coord_x(
      DESIGN_CONTENT_MARGIN,
      face_width);
  const int16_t x_content_end = face_width - x_content_start;
  const int16_t y_content_start = layout_scale_coord_y(
      DESIGN_CONTENT_MARGIN,
      face_height);
  const int16_t y_content_end = face_height - y_content_start;
  const int16_t content_width = face_width - (2 * x_content_start);
  const int16_t display_center = face_height / 2;
  const int16_t rule_left = x_content_start;
  const int16_t rule_right = x_content_end;
  #else
  // Keeping these defaults for now, might change for circular displays
  const int16_t x_content_start = DESIGN_CONTENT_MARGIN;
  const int16_t y_content_start = DESIGN_CONTENT_MARGIN;
  const int16_t y_content_end = face_height - y_content_start;
  const int16_t content_width = face_width - (2 * x_content_start);
  const int16_t display_center = face_height / 2;
  const int16_t rule_left = x_content_start;
  const int16_t rule_right = face_width - x_content_start;
  #endif

  // Scale all of these based on face_height
  // Start with icons
  int16_t icon_size = layout_scale_height(DESIGN_ICON_SIZE, face_height);
  const int16_t icon_text_gap = layout_scale_width(
      DESIGN_ICON_TEXT_GAP,
      face_width);

  // Row and column gaps
  const int16_t row_gap = layout_scale_height(
      DESIGN_ROW_GAP,
      face_height);
  const int16_t column_gap = layout_scale_width(
      DESIGN_COLUMN_GAP,
      face_width);

  // Top-Row: Date
  const int16_t date_row_height = layout_scale_height(20, face_height);

  // Second-Row: Time
  const int16_t time_row_height = layout_scale_height(48, face_height);

  // Third-Row: Health Metrics
  const int16_t metrics_text_height = layout_scale_height(20, face_height);
  const int16_t metrics_row_height = helper_max(
      icon_size,
      metrics_text_height);

  // Fourth-Row: Temp & Battery
  const int16_t bottom_text_height = layout_scale_height(20, face_height);
  const int16_t bottom_row_height = helper_max(
      icon_size,
      bottom_text_height);

  const int16_t time_layer_top = display_center - row_gap -
      time_row_height;

  const int16_t date_top = y_content_start;
  const int16_t metrics_row_top = display_center + row_gap;
  const int16_t bottom_row_top = y_content_end - bottom_row_height;
  const int16_t metrics_left_text_width = layout_scale_width(28, face_width);
  const int16_t metrics_right_text_width = layout_scale_width(40, face_width);
  const int16_t bottom_row_left_text_width = layout_scale_width(40, face_width);
  const int16_t bottom_row_right_text_width = layout_scale_width(34, face_width);
  const int16_t metrics_icon_top =
      metrics_row_top + ((metrics_row_height - icon_size) / 2);

  const int16_t metric_text_top =
      metrics_row_top + ((metrics_row_height - metrics_text_height) / 2);
  const int16_t bottom_icon_top =
      bottom_row_top + ((bottom_row_height - icon_size) / 2);
  const int16_t bottom_text_top =
      bottom_row_top + ((bottom_row_height - bottom_text_height) / 2);

  const int16_t bpm_icon_x = x_content_start;
  const int16_t bpm_value_x = bpm_icon_x + icon_size + icon_text_gap;
  const int16_t right_value_x =
      face_width - x_content_start - metrics_right_text_width;
  const int16_t right_icon_x = right_value_x - icon_text_gap - icon_size;
  const int16_t weather_icon_x = x_content_start;
  const int16_t temperature_value_x =
      weather_icon_x + icon_size + icon_text_gap;
  const int16_t bottom_row_right_value_x =
      face_width - x_content_start - bottom_row_right_text_width;
  const int16_t bottom_row_right_icon_x =
      bottom_row_right_value_x - icon_text_gap - icon_size;

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
                            date_top,
                            layout->content_width_date,
                            date_row_height);
  layout->time_frame = GRect(x_content_start,
                            time_layer_top,
                            layout->content_width_time,
                            time_row_height);
  layout->bpm_icon_frame = GRect(bpm_icon_x,
                                metrics_icon_top,
                                icon_size,
                                icon_size);
  layout->bpm_text_frame = GRect(bpm_value_x,
                                metric_text_top,
                                metrics_left_text_width,
                                metrics_text_height);
  layout->steps_icon_frame = GRect(right_icon_x,
                                  metrics_icon_top,
                                  icon_size,
                                  icon_size);
  layout->steps_text_frame = GRect(right_value_x,
                                  metric_text_top,
                                  metrics_right_text_width,
                                  metrics_text_height);
  layout->weather_icon_frame = GRect(weather_icon_x,
                                     bottom_icon_top,
                                     icon_size,
                                     icon_size);
  layout->temp_text_frame = GRect(temperature_value_x,
                                 bottom_text_top,
                                 bottom_row_left_text_width,
                                 bottom_text_height);
  layout->battery_icon_frame = GRect(bottom_row_right_icon_x,
                                    bottom_icon_top,
                                    icon_size,
                                    icon_size);
  layout->battery_text_frame = GRect(bottom_row_right_value_x,
                                    bottom_text_top,
                                    bottom_row_right_text_width,
                                    bottom_text_height);
}
