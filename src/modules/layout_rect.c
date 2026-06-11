#include "layout_rect.h"
#include "helper.h"
#include "../c/ataglance.h"

static int16_t layout_rect_scale_coord_y(
    int16_t design_y,
    int16_t face_height) {
  return helper_scale_round(
      design_y,
      face_height,
      ATAGLANCE_DESIGN_FACE_HEIGHT);
}

static int16_t layout_rect_scale_coord_x(
    int16_t design_x,
    int16_t face_width) {
  return helper_scale_round(
      design_x,
      face_width,
      ATAGLANCE_DESIGN_FACE_WIDTH);
}

static int16_t layout_rect_scale_height(
    int16_t design_widget_height,
    int16_t current_face_height) {
  return helper_scale_round(
      design_widget_height,
      current_face_height,
      ATAGLANCE_DESIGN_FACE_HEIGHT);
}

static int16_t layout_rect_scale_width(
    int16_t design_widget_width,
    int16_t current_face_width) {
  return helper_scale_round(
      design_widget_width,
      current_face_width,
      ATAGLANCE_DESIGN_FACE_WIDTH);
}

typedef struct {
  int16_t content_left;
  int16_t content_right;
  int16_t content_top;
  int16_t content_bottom;
  int16_t content_width;
  int16_t center_y;
  int16_t row_gap;
  int16_t icon_w;
  int16_t icon_h;
  int16_t icon_text_gap;
  int16_t data_width;
  int16_t date_text_height;
  int16_t time_text_height;
  int16_t top_text_height;
  int16_t bottom_text_height;
  int16_t top_row_height;
  int16_t bottom_row_height;
} LayoutRectMetrics;

static LayoutRectMetrics layout_rect_metrics(
    int16_t face_width,
    int16_t face_height) {
  const int16_t content_left = layout_rect_scale_coord_x(
      ATAGLANCE_DESIGN_CONTENT_MARGIN,
      face_width);
  const int16_t content_top = layout_rect_scale_coord_y(
      ATAGLANCE_DESIGN_CONTENT_MARGIN,
      face_height);
  const int16_t icon_w = layout_rect_scale_width(
      ATAGLANCE_DESIGN_ICON_WIDTH,
      face_width);
  const int16_t icon_h = layout_rect_scale_height(
      ATAGLANCE_DESIGN_ICON_HEIGHT,
      face_height);
  const int16_t top_text_height = layout_rect_scale_height(
      ATAGLANCE_DESIGN_BOTTOM_TEXT_HEIGHT,
      face_height);
  const int16_t bottom_text_height = layout_rect_scale_height(
      ATAGLANCE_DESIGN_HEALTH_TEXT_HEIGHT,
      face_height);
  const int16_t date_text_height = layout_rect_scale_height(
      ATAGLANCE_DESIGN_DATE_TEXT_HEIGHT,
      face_height);

  return (LayoutRectMetrics) {
    .content_left = content_left,
    .content_right = face_width - content_left,
    .content_top = content_top,
    .content_bottom = face_height - content_top,
    .content_width = face_width - (2 * content_left),
    .center_y = face_height / 2,
    .row_gap = layout_rect_scale_height(
        ATAGLANCE_DESIGN_ROW_GAP,
        face_height),
    .icon_w = icon_w,
    .icon_h = icon_h,
    .icon_text_gap = layout_rect_scale_width(
        ATAGLANCE_DESIGN_ICON_TEXT_GAP,
        face_width),
    .data_width = layout_rect_scale_width(
        ATAGLANCE_DESIGN_DATA_FIELD_WIDTH,
        face_width),
    .date_text_height = date_text_height,
    .time_text_height = layout_rect_scale_height(
        ATAGLANCE_DESIGN_TIME_TEXT_HEIGHT,
        face_height),
    .top_text_height = top_text_height,
    .bottom_text_height = bottom_text_height,
    .top_row_height = helper_max(icon_h, top_text_height),
    .bottom_row_height = helper_max(icon_h, bottom_text_height),
  };
}

void layout_rect_calculate_surface(
    int16_t face_width,
    int16_t face_height,
    WatchfaceSurface* surface) {
  if (!surface) {
    return;
  }

  const LayoutRectMetrics metrics =
      layout_rect_metrics(face_width, face_height);
  const int16_t pair_width =
      metrics.icon_w + metrics.icon_text_gap + metrics.data_width;
  const int16_t right_pair_x = metrics.content_right - pair_width;
  const int16_t rule_y = metrics.center_y;
  const int16_t top_row_top = metrics.content_top;
  const int16_t top_icon_top = top_row_top +
      ((metrics.top_row_height - metrics.icon_h) / 2);
  const int16_t top_text_top = top_row_top +
      ((metrics.top_row_height - metrics.top_text_height) / 2);
  const int16_t time_text_top =
      rule_y - metrics.row_gap - metrics.time_text_height;
  const int16_t date_text_top = rule_y + metrics.row_gap;
  const int16_t bottom_row_top =
      metrics.content_bottom - metrics.bottom_row_height;
  const int16_t bottom_icon_top = bottom_row_top +
      ((metrics.bottom_row_height - metrics.icon_h) / 2);
  const int16_t bottom_text_top = bottom_row_top +
      ((metrics.bottom_row_height - metrics.bottom_text_height) / 2);

  surface->face_width = face_width;
  surface->face_height = face_height;
  surface->background = (WatchfaceBackgroundSubstratum) {
    .frame = GRect(0, 0, face_width, face_height),
    .line_enabled = true,
    .line_x = metrics.content_left,
    .line_y = rule_y,
    .line_width = metrics.content_width,
  };

  surface->date.text = (WatchfaceTextSubstratum) {
    .frame = GRect(metrics.content_left,
                   date_text_top,
                   metrics.content_width,
                   metrics.date_text_height),
    .alignment = GTextAlignmentCenter,
    .font_role = WATCHFACE_FONT_ROLE_DATE,
    .color_role = WATCHFACE_COLOR_ROLE_DATE,
  };
  surface->time.text = (WatchfaceTextSubstratum) {
    .frame = GRect(metrics.content_left,
                   time_text_top,
                   metrics.content_width,
                   metrics.time_text_height),
    .alignment = GTextAlignmentCenter,
    .font_role = WATCHFACE_FONT_ROLE_TIME,
    .color_role = WATCHFACE_COLOR_ROLE_TIME,
  };
  surface->battery.icon = (WatchfaceIconSubstratum) {
    .frame = GRect(metrics.content_left,
                   top_icon_top,
                   metrics.icon_w,
                   metrics.icon_h),
    .is_enabled = true,
  };
  surface->battery.text = (WatchfaceTextSubstratum) {
    .frame = GRect(metrics.content_left +
                       metrics.icon_w +
                       metrics.icon_text_gap,
                   top_text_top,
                   metrics.data_width,
                   metrics.top_text_height),
    .alignment = GTextAlignmentLeft,
    .font_role = WATCHFACE_FONT_ROLE_BATTERY,
  };
  surface->climate.icon = (WatchfaceIconSubstratum) {
    .frame = GRect(right_pair_x,
                   top_icon_top,
                   metrics.icon_w,
                   metrics.icon_h),
    .is_enabled = true,
  };
  surface->climate.text = (WatchfaceTextSubstratum) {
    .frame = GRect(right_pair_x +
                       metrics.icon_w +
                       metrics.icon_text_gap,
                   top_text_top,
                   metrics.data_width,
                   metrics.top_text_height),
    .alignment = GTextAlignmentLeft,
    .font_role = WATCHFACE_FONT_ROLE_CLIMATE,
  };
  surface->steps.icon = (WatchfaceIconSubstratum) {
    .frame = GRect(metrics.content_left,
                   bottom_icon_top,
                   metrics.icon_w,
                   metrics.icon_h),
    .is_enabled = true,
  };
  surface->steps.text = (WatchfaceTextSubstratum) {
    .frame = GRect(metrics.content_left +
                       metrics.icon_w +
                       metrics.icon_text_gap,
                   bottom_text_top,
                   metrics.data_width,
                   metrics.bottom_text_height),
    .alignment = GTextAlignmentLeft,
    .font_role = WATCHFACE_FONT_ROLE_STEPS,
  };
  surface->bpm.icon = (WatchfaceIconSubstratum) {
    .frame = GRect(right_pair_x,
                   bottom_icon_top,
                   metrics.icon_w,
                   metrics.icon_h),
    .is_enabled = true,
  };
  surface->bpm.text = (WatchfaceTextSubstratum) {
    .frame = GRect(right_pair_x +
                       metrics.icon_w +
                       metrics.icon_text_gap,
                   bottom_text_top,
                   metrics.data_width,
                   metrics.bottom_text_height),
    .alignment = GTextAlignmentLeft,
    .font_role = WATCHFACE_FONT_ROLE_BPM,
  };
}
