#include "layout_rect.h"
#include "../c/ataglance.h"

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
  int16_t date_text_height;
  int16_t time_text_height;
  int16_t data_text_width;
  int16_t data_text_height;
  int16_t icon_text_pair_height;
} LayoutRectMetrics;

#define RECT_SCALE_X(value) \
  (((value) * PBL_DISPLAY_WIDTH + (ATAGLANCE_DESIGN_FACE_WIDTH / 2)) / \
      ATAGLANCE_DESIGN_FACE_WIDTH)

#define RECT_SCALE_Y(value) \
  (((value) * PBL_DISPLAY_HEIGHT + (ATAGLANCE_DESIGN_FACE_HEIGHT / 2)) / \
      ATAGLANCE_DESIGN_FACE_HEIGHT)

#define RECT_MAX(a, b) ((a) > (b) ? (a) : (b))

#if (PBL_DISPLAY_WIDTH >= ATAGLANCE_DESIGN_FACE_WIDTH && \
  PBL_DISPLAY_HEIGHT >= ATAGLANCE_DESIGN_FACE_HEIGHT)
// Designed blueprint
static const LayoutRectMetrics c_rect_blueprint = {
  .content_left = ATAGLANCE_DESIGN_CONTENT_MARGIN,
  .content_right =
      ATAGLANCE_DESIGN_FACE_WIDTH - ATAGLANCE_DESIGN_CONTENT_MARGIN,
  .content_top = ATAGLANCE_DESIGN_CONTENT_MARGIN,
  .content_bottom =
      ATAGLANCE_DESIGN_FACE_HEIGHT - ATAGLANCE_DESIGN_CONTENT_MARGIN,
  .content_width = ATAGLANCE_DESIGN_FACE_WIDTH -
      (2 * ATAGLANCE_DESIGN_CONTENT_MARGIN),
  .center_y = ATAGLANCE_DESIGN_FACE_HEIGHT / 2,
  .row_gap = ATAGLANCE_DESIGN_ROW_GAP,
  .icon_w = ATAGLANCE_DESIGN_ICON_WIDTH,
  .icon_h = ATAGLANCE_DESIGN_ICON_HEIGHT,
  .icon_text_gap = ATAGLANCE_DESIGN_ICON_TEXT_GAP,
  .date_text_height = ATAGLANCE_DESIGN_DATE_TEXT_HEIGHT,
  .time_text_height = ATAGLANCE_DESIGN_TIME_TEXT_HEIGHT,
  .data_text_width = ATAGLANCE_DESIGN_DATA_TEXT_WIDTH,
  .data_text_height = ATAGLANCE_DESIGN_DATA_TEXT_HEIGHT,
  .icon_text_pair_height = RECT_MAX(
      ATAGLANCE_DESIGN_ICON_HEIGHT,
      ATAGLANCE_DESIGN_DATA_TEXT_HEIGHT),
};
#else
// Compact blueprint
static const LayoutRectMetrics c_rect_blueprint = {
  .content_left = ATAGLANCE_DESIGN_CONTENT_MARGIN,
  .content_right = PBL_DISPLAY_WIDTH - ATAGLANCE_DESIGN_CONTENT_MARGIN,
  .content_top = ATAGLANCE_DESIGN_CONTENT_MARGIN,
  .content_bottom = PBL_DISPLAY_HEIGHT - ATAGLANCE_DESIGN_CONTENT_MARGIN,
  .content_width = PBL_DISPLAY_WIDTH -
      (2 * ATAGLANCE_DESIGN_CONTENT_MARGIN),
  .center_y = PBL_DISPLAY_HEIGHT / 2,
  .row_gap = RECT_SCALE_Y(ATAGLANCE_DESIGN_ROW_GAP),
  .icon_w = RECT_SCALE_X(ATAGLANCE_DESIGN_ICON_WIDTH),
  .icon_h = RECT_SCALE_Y(ATAGLANCE_DESIGN_ICON_HEIGHT),
  .icon_text_gap = ATAGLANCE_DESIGN_ICON_TEXT_GAP,
  .date_text_height =
      RECT_SCALE_Y(ATAGLANCE_DESIGN_DATE_TEXT_HEIGHT),
  .time_text_height =
      RECT_SCALE_Y(ATAGLANCE_DESIGN_TIME_TEXT_HEIGHT),
  .data_text_width =
      RECT_SCALE_X(ATAGLANCE_DESIGN_DATA_TEXT_WIDTH),
  .data_text_height =
      RECT_SCALE_Y(ATAGLANCE_DESIGN_DATA_TEXT_HEIGHT),
  .icon_text_pair_height = RECT_MAX(
      RECT_SCALE_Y(ATAGLANCE_DESIGN_ICON_HEIGHT),
      RECT_SCALE_Y(ATAGLANCE_DESIGN_DATA_TEXT_HEIGHT)),
};
#endif

void layout_rect_calculate_surface(
    int16_t face_width,
    int16_t face_height,
    WatchfaceSurface* surface) {
  if (!surface) {
    return;
  }

  bool is_compact = face_width < ATAGLANCE_DESIGN_FACE_WIDTH ||
      face_height < ATAGLANCE_DESIGN_FACE_HEIGHT;
  surface->style.is_compact = is_compact;
  surface->face_width = face_width;
  surface->face_height = face_height;

  const LayoutRectMetrics* metrics = &c_rect_blueprint;

  // These values are computed more than twice, so caching them
  const int16_t x_start = metrics->content_left;
  const int16_t x_end = metrics->content_right;
  const int16_t center_y = metrics->center_y;
  const int16_t content_width = metrics->content_width;
  const int16_t y_start = metrics->content_top;
  const int16_t y_end = metrics->content_bottom;
  const int16_t data_text_h = metrics->data_text_height;
  const int16_t data_text_w = metrics->data_text_width;
  const int16_t icon_h = metrics->icon_h;
  const int16_t icon_w = metrics->icon_w;
  const int16_t icon_text_gap = metrics->icon_text_gap;

  // Calculating the surface, one stratum at a time
  // Background
  int16_t str_top = center_y;
  int16_t str_left = x_start;
  surface->background = (WatchfaceBackgroundStratum) {
    .frame = GRect(0, 0, face_width, face_height),
    .line_enabled = true,
    .line_x = str_left,
    .line_y = str_top,
    .line_width = content_width,
  };

  // Time
  str_top = center_y - metrics->time_text_height - metrics->row_gap;
  str_left = x_start;
  surface->time.text = (WatchfaceTextSubstratum) {
    .frame = GRect(str_left, str_top, content_width, metrics->time_text_height),
    .alignment = GTextAlignmentCenter,
    .font_role = WATCHFACE_FONT_ROLE_TIME,
    .color_role = WATCHFACE_COLOR_ROLE_TIME,
  };

  // Date
  str_top = center_y + metrics->row_gap;
  str_left = x_start;
  surface->date.text = (WatchfaceTextSubstratum) {
    .frame = GRect(str_left, str_top, content_width, metrics->date_text_height),
    .alignment = GTextAlignmentCenter,
    .font_role = WATCHFACE_FONT_ROLE_DATE,
    .color_role = WATCHFACE_COLOR_ROLE_DATE,
  };

  // Calculate offsets for icons & text from the pair's starting y-coord
  // Both offsets start out as 0 in the world in which they have the same ht.
  int16_t text_offset_y = 0;
  int16_t icon_offset_y = 0;
  int16_t height_diff = (icon_h - data_text_h) / 2;
  if (height_diff > 0) {
    // Icon is taller, push text down by the difference
    text_offset_y = height_diff;
  } else if (height_diff < 0) {
    // Text is taller, push icon down by the difference
    icon_offset_y = -height_diff;
  }

  // Battery & Climate are in the same row, so anchor the row to str_top
  str_top = y_start;
  str_left = x_start;
  surface->battery.icon = (WatchfaceIconSubstratum) {
    .frame = GRect(str_left, str_top + icon_offset_y, icon_w, icon_h),
    .is_enabled = true,
  };
  // Only the x-coord will change for the row
  str_left += icon_w + icon_text_gap;
  surface->battery.text = (WatchfaceTextSubstratum) {
    .frame = GRect(str_left,
                   str_top + text_offset_y,
                   data_text_w,
                   data_text_h),
    .alignment = GTextAlignmentLeft,
    .font_role = WATCHFACE_FONT_ROLE_BATTERY,
  };

  // Climate, same y-coord as Battery
  str_left = x_end - data_text_w - icon_text_gap - icon_w;
  surface->climate.icon = (WatchfaceIconSubstratum) {
    .frame = GRect(str_left, str_top + icon_offset_y, icon_w, icon_h),
    .is_enabled = true,
  };

  // Only the x-coord will change for the row
  str_left += icon_w + icon_text_gap;
  surface->climate.text = (WatchfaceTextSubstratum) {
    .frame = GRect(str_left,
                   str_top + text_offset_y,
                   data_text_w,
                   data_text_h),
    .alignment = GTextAlignmentLeft,
    .font_role = WATCHFACE_FONT_ROLE_CLIMATE,
  };

  // Steps & BPM are in the same row, so anchor the row to str_top
  str_top = y_end - metrics->icon_text_pair_height;
  str_left = x_start;
  surface->steps.icon = (WatchfaceIconSubstratum) {
    .frame = GRect(str_left, str_top + icon_offset_y, icon_w, icon_h),
    .is_enabled = true,
  };

  // Only the x-coord will change for the row
  str_left += icon_w + icon_text_gap;
  surface->steps.text = (WatchfaceTextSubstratum) {
    .frame = GRect(str_left,
                   str_top + text_offset_y,
                   data_text_w,
                   data_text_h),
    .alignment = GTextAlignmentLeft,
    .font_role = WATCHFACE_FONT_ROLE_STEPS,
  };

  // BPM
  // Only the x-coord will change for the row
  str_left = x_end - data_text_w - icon_text_gap - icon_w;
  surface->bpm.icon = (WatchfaceIconSubstratum) {
    .frame = GRect(str_left, str_top + icon_offset_y, icon_w, icon_h),
    .is_enabled = true,
  };
  // Only the x-coord will change for the row
  str_left += icon_w + icon_text_gap;
  surface->bpm.text = (WatchfaceTextSubstratum) {
    .frame = GRect(str_left,
                   str_top + text_offset_y,
                   data_text_w,
                   data_text_h),
    .alignment = GTextAlignmentLeft,
    .font_role = WATCHFACE_FONT_ROLE_BPM,
  };
}
