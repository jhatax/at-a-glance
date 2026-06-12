#ifdef PBL_ROUND
#include "layout_architect.h"
#include "../c/ataglance.h"
#include "helper.h"

// The round-reference is for Chalk
// We shouldn't need to scale any of these down for Chalk
// We will have to scale them up for Gabbro, which the macro
// should take care of.
typedef enum {
  ROUND_HEADER_FOOTER = 27,
  ROUND_COLUMN_GAP = 1,
  ROUND_REFERENCE_DIA = 180,
  ROUND_TIME_WIDTH = 140,
  ROUND_DATE_WIDTH = 120,
  ROUND_REFERENCE_WIDTH = DESIGN_FACE_HEIGHT,
  ROUND_REFERENCE_HEIGHT = DESIGN_FACE_HEIGHT,
  // Design decision to scale everything to DESIGN_FACE_HEIGHT
} RoundDesignInputs;

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

#define DESIGN_SCALE_X(v) HELPER_SCALE_ROUND((v), PBL_DISPLAY_WIDTH, ROUND_REFERENCE_WIDTH)

#define DESIGN_SCALE_Y(v) HELPER_SCALE_ROUND((v), PBL_DISPLAY_HEIGHT, ROUND_REFERENCE_HEIGHT)

#define ROUND_SCALE_VALUE(v) HELPER_SCALE_ROUND((v), PBL_DISPLAY_HEIGHT, ROUND_REFERENCE_DIA)

#if (PBL_DISPLAY_WIDTH <= DESIGN_FACE_WIDTH && \
  PBL_DISPLAY_HEIGHT <= DESIGN_FACE_HEIGHT)
// Blueprint for compact devices
static const LayoutBlueprint c_round_blueprint = {
  .margin = DESIGN_MARGIN,
  .y_start = ROUND_HEADER_FOOTER,
  .y_end = PBL_DISPLAY_HEIGHT - ROUND_HEADER_FOOTER,
  .row_gap = DESIGN_SCALE_Y(DESIGN_ROW_GAP),
  .icon_w = DESIGN_SCALE_X(DESIGN_ICON_WIDTH),
  .icon_h = DESIGN_SCALE_Y(DESIGN_ICON_HEIGHT),
  .icon_text_gap = DESIGN_SCALE_X(DESIGN_ICON_TEXT_GAP),
  .date_text_height = DESIGN_SCALE_Y(DESIGN_DATE_TEXT_HEIGHT),
  .time_text_height = DESIGN_SCALE_Y(DESIGN_TIME_TEXT_HEIGHT),
  .data_text_width = DESIGN_SCALE_X(DESIGN_DATA_TEXT_WIDTH),
  .data_text_height = DESIGN_SCALE_Y(DESIGN_DATA_TEXT_HEIGHT),
  .icon_text_pair_height = DESIGN_SCALE_Y(HELPER_MAX(DESIGN_ICON_HEIGHT,DESIGN_DATA_TEXT_HEIGHT)),
};
#else
// Blueprint for larger devices
static const LayoutBlueprint c_round_blueprint = {
  .margin = DESIGN_MARGIN,
  .y_start = ROUND_HEADER_FOOTER, // Should be bigger than defined
  .y_end = PBL_DISPLAY_HEIGHT - ROUND_HEADER_FOOTER,
  .row_gap = DESIGN_ROW_GAP,
  .icon_w = DESIGN_ICON_WIDTH,
  .icon_h = DESIGN_ICON_HEIGHT,
  .icon_text_gap = DESIGN_ICON_TEXT_GAP,
  .date_text_height = DESIGN_DATE_TEXT_HEIGHT,
  .time_text_height = DESIGN_TIME_TEXT_HEIGHT,
  .data_text_width = DESIGN_DATA_TEXT_WIDTH,
  .data_text_height = DESIGN_DATA_TEXT_HEIGHT,
  .icon_text_pair_height = HELPER_MAX(DESIGN_ICON_HEIGHT,DESIGN_DATA_TEXT_HEIGHT),
};
#endif

void architect_get_layout_from_blueprint(
  const LayoutBlueprint* blueprint,
  CalculatedLayout* computed,
  int16_t face_width,
  int16_t face_height) {

  if (!blueprint || !computed) {
    return;
  }
    // Start from a clean slate
  memset(computed, 0, sizeof(*computed));

  const int16_t row_gap = blueprint->row_gap;
  const int16_t center_x = face_width / 2;

  computed->top_row_y = blueprint->y_start;

  int tmp_a = 0;
  int tmp_b = 0;

  // Time
  tmp_a = blueprint->time_text_height;
  tmp_b = ROUND_SCALE_VALUE(ROUND_TIME_WIDTH);
  computed->time_x = center_x - (tmp_b >> 1); // x-start = x-center - field-width/2
  computed->time_y = computed->top_row_y + blueprint->icon_text_pair_height + row_gap;
  computed->time_w = tmp_b;
  computed->time_h = tmp_a;

  tmp_b = face_width >> 2;
  computed->rule_x = center_x - tmp_b;
  computed->rule_y = computed->time_y + computed->time_h + row_gap;
  computed->rule_w = face_width >> 1;
  computed->rule_h = DESIGN_HORIZON_H;

  // Date
  tmp_b = ROUND_SCALE_VALUE(ROUND_DATE_WIDTH);
  computed->date_y = computed->rule_y + row_gap;
  computed->date_x = center_x - (tmp_b >> 1); // x-start = x-center - field-width/2
  computed->date_w = tmp_b;
  computed->date_h = blueprint->date_text_height;

  computed->bottom_row_y = computed->date_y + computed->date_h + row_gap;

  // For icon and text layers
  const int16_t col_gap = ROUND_SCALE_VALUE(ROUND_COLUMN_GAP);
  const int16_t data_text_h = blueprint->data_text_height;
  const int16_t data_text_w = blueprint->data_text_width;
  const int16_t icon_text_gap = blueprint->icon_text_gap;
  const int16_t icon_w = blueprint->icon_w;
  const int16_t icon_h = blueprint->icon_h;

  tmp_b = center_x - col_gap - data_text_w;
  computed->left_text_x = tmp_b;
  computed->left_icon_x = tmp_b - icon_text_gap - icon_w;

  tmp_b = center_x + col_gap;
  computed->right_icon_x = tmp_b;
  computed->right_text_x = tmp_b + icon_text_gap + icon_w;

  computed->icon_w = icon_w;
  computed->icon_h = icon_h;

  computed->text_w = data_text_w;
  computed->text_h = data_text_h;
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
  computed->icon_offset_y = icon_offset_y;
  computed->text_offset_y = text_offset_y;
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

  const LayoutBlueprint* blueprint = &c_round_blueprint;
  CalculatedLayout computed = {0};
  architect_get_layout_from_blueprint(blueprint, &computed, face_width, face_height);

  // Calculating the surface, one stratum at a time
  // Background
  surface->background = (WatchfaceBackgroundStratum) {
    .frame = GRect(0, 0, face_width, face_height),
    .line_enabled = true,
    .line_x = computed.rule_x,
    .line_y = computed.rule_y,
    .line_width = computed.rule_w,
    .line_height = computed.rule_h,
  };

  // Time
  surface->time.text = (WatchfaceTextSubstratum) {
    .frame = GRect(computed.time_x, computed.time_y, computed.time_w, computed.time_h),
    .alignment = GTextAlignmentCenter,
    .font_role = WATCHFACE_FONT_ROLE_TIME,
    .color_role = WATCHFACE_COLOR_ROLE_TIME,
  };

  // Date
  surface->date.text = (WatchfaceTextSubstratum) {
    .frame = GRect(computed.date_x, computed.date_y, computed.date_w, computed.date_h),
    .alignment = GTextAlignmentCenter,
    .font_role = WATCHFACE_FONT_ROLE_DATE,
    .color_role = WATCHFACE_COLOR_ROLE_DATE,
  };

  // Battery: top-left
  surface->battery.icon = (WatchfaceIconSubstratum) {
    .frame = GRect(
      computed.left_icon_x,
      computed.top_row_y + computed.icon_offset_y,
      computed.icon_w,
      computed.icon_h),
    .is_enabled = true,
  };
  surface->battery.text = (WatchfaceTextSubstratum) {
    .frame = GRect(
      computed.left_text_x,
      computed.top_row_y + computed.text_offset_y,
      computed.text_w,
      computed.text_h),
    .alignment = GTextAlignmentLeft,
    .font_role = WATCHFACE_FONT_ROLE_BATTERY,
  };

  // Climate: top-right
  surface->climate.icon = (WatchfaceIconSubstratum) {
    .frame = GRect(
      computed.right_icon_x,
      computed.top_row_y + computed.icon_offset_y,
      computed.icon_w,
      computed.icon_h),
    .is_enabled = true,
  };

  surface->climate.text = (WatchfaceTextSubstratum) {
    .frame = GRect(
      computed.right_text_x,
      computed.top_row_y + computed.text_offset_y,
      computed.text_w,
      computed.text_h),
    .alignment = GTextAlignmentLeft,
    .font_role = WATCHFACE_FONT_ROLE_CLIMATE,
  };

  // BPM: bottom-left
  surface->bpm.icon = (WatchfaceIconSubstratum) {
    .frame = GRect(
      computed.left_icon_x,
      computed.bottom_row_y + computed.icon_offset_y,
      computed.icon_w,
      computed.icon_h),
    .is_enabled = true,
  };

  surface->bpm.text = (WatchfaceTextSubstratum) {
    .frame = GRect(
      computed.left_text_x,
      computed.bottom_row_y + computed.text_offset_y,
      computed.text_w,
      computed.text_h),
    .alignment = GTextAlignmentLeft,
    .font_role = WATCHFACE_FONT_ROLE_BPM,
  };

  // Steps: bottom-right
  surface->steps.icon = (WatchfaceIconSubstratum) {
    .frame = GRect(
      computed.right_icon_x,
      computed.bottom_row_y + computed.icon_offset_y,
      computed.icon_w,
      computed.icon_h),
    .is_enabled = true,
  };

  surface->steps.text = (WatchfaceTextSubstratum) {
    .frame = GRect(
      computed.right_text_x,
      computed.bottom_row_y + computed.text_offset_y,
      computed.text_w,
      computed.text_h),
    .alignment = GTextAlignmentLeft,
    .font_role = WATCHFACE_FONT_ROLE_STEPS,
  };
}
#endif
