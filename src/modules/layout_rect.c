#ifdef PBL_RECT
#include "layout_architect.h"
#include "../c/ataglance.h"

typedef struct {
  int16_t margin;
  int16_t y_start;
  int16_t y_end;
  int16_t row_gap;
  int16_t icon_w;
  int16_t icon_h;
  int16_t icon_text_gap;
  int16_t date_text_height;
  int16_t time_text_height;
  int16_t data_text_width;
  int16_t data_text_height;
  int16_t icon_text_pair_height;
} LayoutBlueprint;

typedef struct {
  int16_t top_row_y;
  int16_t bottom_row_y;

  int16_t time_x;
  int16_t time_y;
  int16_t time_w;
  int16_t time_h;

  int16_t rule_x;
  int16_t rule_y;
  int16_t rule_w;
  int16_t rule_h;

  int16_t date_x;
  int16_t date_y;
  int16_t date_w;
  int16_t date_h;

  int16_t left_icon_x;
  int16_t left_text_x;

  int16_t right_icon_x;
  int16_t right_text_x;

  int16_t icon_w;
  int16_t icon_h;

  int16_t text_w;
  int16_t text_h;

  int16_t icon_offset_y;
  int16_t text_offset_y;
  } CalculatedLayout;

#define RECT_SCALE_X(value) \
  (((value) * PBL_DISPLAY_WIDTH + (DESIGN_FACE_WIDTH / 2)) / \
      DESIGN_FACE_WIDTH)

#define RECT_SCALE_Y(value) \
  (((value) * PBL_DISPLAY_HEIGHT + (DESIGN_FACE_HEIGHT / 2)) / \
      DESIGN_FACE_HEIGHT)

#define RECT_MAX(a, b) ((a) > (b) ? (a) : (b))

#if (PBL_DISPLAY_WIDTH >= DESIGN_FACE_WIDTH && \
  PBL_DISPLAY_HEIGHT >= DESIGN_FACE_HEIGHT)
// Designed blueprint
static const LayoutBlueprint c_rect_blueprint = {
  .margin = DESIGN_MARGIN,
  .y_start = DESIGN_MARGIN,
  .y_end = DESIGN_FACE_HEIGHT - DESIGN_MARGIN,
  .row_gap = DESIGN_ROW_GAP,
  .icon_w = DESIGN_ICON_WIDTH,
  .icon_h = DESIGN_ICON_HEIGHT,
  .icon_text_gap = DESIGN_ICON_TEXT_GAP,
  .date_text_height = DESIGN_DATE_TEXT_HEIGHT,
  .time_text_height = DESIGN_TIME_TEXT_HEIGHT,
  .data_text_width = DESIGN_DATA_TEXT_WIDTH,
  .data_text_height = DESIGN_DATA_TEXT_HEIGHT,
  .icon_text_pair_height = RECT_MAX(
      DESIGN_ICON_HEIGHT,
      DESIGN_DATA_TEXT_HEIGHT),
};
#else
// Compact blueprint
static const LayoutBlueprint c_rect_blueprint = {
  .margin = DESIGN_MARGIN,
  .y_start = DESIGN_MARGIN,
  .y_end = PBL_DISPLAY_HEIGHT - DESIGN_MARGIN,
  .row_gap = RECT_SCALE_Y(DESIGN_ROW_GAP),
  .icon_w = RECT_SCALE_X(DESIGN_ICON_WIDTH),
  .icon_h = RECT_SCALE_Y(DESIGN_ICON_HEIGHT),
  .icon_text_gap = DESIGN_ICON_TEXT_GAP,
  .date_text_height = RECT_SCALE_Y(DESIGN_DATE_TEXT_HEIGHT),
  .time_text_height = RECT_SCALE_Y(DESIGN_TIME_TEXT_HEIGHT),
  .data_text_width = RECT_SCALE_X(DESIGN_DATA_TEXT_WIDTH),
  .data_text_height = RECT_SCALE_Y(DESIGN_DATA_TEXT_HEIGHT),
  .icon_text_pair_height = RECT_SCALE_Y(RECT_MAX(
    DESIGN_ICON_HEIGHT,
    DESIGN_DATA_TEXT_HEIGHT)),
};
#endif

void architect_get_layout_from_blueprint(const LayoutBlueprint* blueprint, CalculatedLayout* layout) {
  (void)blueprint;
  (void)layout;
}

void architect_apply_blueprint(
    int16_t face_width,
    int16_t face_height,
    WatchfaceSurface* surface) {
  if (!surface) {
    return;
  }

  bool is_compact = face_width < DESIGN_FACE_WIDTH &&
      face_height < DESIGN_FACE_HEIGHT;
  surface->style.is_compact = is_compact;
  surface->face_width = face_width;
  surface->face_height = face_height;

  const LayoutBlueprint* blueprint = &c_rect_blueprint;

  // These values are computed more than twice, so caching them
  const int16_t x_start = blueprint->margin;
  const int16_t x_end = face_width - blueprint->margin;
  const int16_t rule_y = face_height>>1;
  const int16_t content_width = x_end - x_start;
  const int16_t y_start = blueprint->y_start;
  const int16_t y_end = blueprint->y_end;
  const int16_t data_text_h = blueprint->data_text_height;
  const int16_t data_text_w = blueprint->data_text_width;
  const int16_t icon_h = blueprint->icon_h;
  const int16_t icon_w = blueprint->icon_w;
  const int16_t icon_text_gap = blueprint->icon_text_gap;

  // Calculating the surface, one stratum at a time
  // Background
  int16_t str_top = rule_y;
  int16_t str_left = x_start;
  surface->background = (WatchfaceBackgroundStratum) {
    .frame = GRect(0, 0, face_width, face_height),
    .line_enabled = true,
    .line_x = str_left,
    .line_y = str_top,
    .line_width = content_width,
    .line_height = DESIGN_HORIZON_H,
  };

  // Time
  str_top = rule_y - blueprint->time_text_height - blueprint->row_gap;
  str_left = x_start;
  surface->time.text = (WatchfaceTextSubstratum) {
    .frame = GRect(str_left, str_top, content_width, blueprint->time_text_height),
    .alignment = GTextAlignmentCenter,
    .font_role = WATCHFACE_FONT_ROLE_TIME,
    .color_role = WATCHFACE_COLOR_ROLE_TIME,
  };

  // Date
  str_top = rule_y + blueprint->row_gap;
  str_left = x_start;
  surface->date.text = (WatchfaceTextSubstratum) {
    .frame = GRect(str_left, str_top, content_width, blueprint->date_text_height),
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
  str_top = y_end - blueprint->icon_text_pair_height;
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
#endif
