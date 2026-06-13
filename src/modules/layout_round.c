#ifdef PBL_ROUND
#include "layout.h"
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
  GRect icon;
  GRect text;
} CalculatedMetricPair;

typedef struct {
  GRect rule;
  GRect time;
  GRect date;

  CalculatedMetricPair battery;
  CalculatedMetricPair climate;
#ifdef PBL_HEALTH
  CalculatedMetricPair steps;
  CalculatedMetricPair bpm;
#endif
} CalculatedLayout;

#define DESIGN_SCALE_X(v) \
  HELPER_SCALE_ROUND((v), PBL_DISPLAY_WIDTH, ROUND_REFERENCE_WIDTH)

#define DESIGN_SCALE_Y(v) \
  HELPER_SCALE_ROUND((v), PBL_DISPLAY_HEIGHT, ROUND_REFERENCE_HEIGHT)

#define ROUND_SCALE_VALUE(v) \
  HELPER_SCALE_ROUND((v), PBL_DISPLAY_HEIGHT, ROUND_REFERENCE_DIA)

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
  .icon_text_pair_height = DESIGN_SCALE_Y(HELPER_MAX(
      DESIGN_ICON_HEIGHT,
      DESIGN_DATA_TEXT_HEIGHT)),
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
  .icon_text_pair_height = HELPER_MAX(
      DESIGN_ICON_HEIGHT,
      DESIGN_DATA_TEXT_HEIGHT),
};
#endif

static void architect_get_layout_from_blueprint(
    const LayoutBlueprint* blueprint,
    CalculatedLayout* computed,
    int16_t face_width,
    int16_t face_height) {
  if (!blueprint || !computed) {
    return;
  }

  memset(computed, 0, sizeof(*computed));

  const int16_t row_gap = blueprint->row_gap;
  const int16_t center_x = face_width / 2;
  const int16_t top_row_y = blueprint->y_start;
  const int16_t time_w = ROUND_SCALE_VALUE(ROUND_TIME_WIDTH);
  const int16_t time_y = top_row_y + blueprint->icon_text_pair_height +
      row_gap;

  computed->time = GRect(
      center_x - (time_w >> 1),
      time_y,
      time_w,
      blueprint->time_text_height);

  computed->rule = GRect(
      center_x - (face_width >> 2),
      time_y + blueprint->time_text_height + row_gap,
      face_width >> 1,
      DESIGN_HORIZON_H);

  const int16_t date_w = ROUND_SCALE_VALUE(ROUND_DATE_WIDTH);
  const int16_t date_y = computed->rule.origin.y + row_gap;

  computed->date = GRect(
      center_x - (date_w >> 1),
      date_y,
      date_w,
      blueprint->date_text_height);

  const int16_t bottom_row_y = date_y + blueprint->date_text_height +
      row_gap;

  const int16_t col_gap = ROUND_SCALE_VALUE(ROUND_COLUMN_GAP);
  const int16_t data_text_h = blueprint->data_text_height;
  const int16_t data_text_w = blueprint->data_text_width;
  const int16_t icon_text_gap = blueprint->icon_text_gap;
  const int16_t icon_w = blueprint->icon_w;
  const int16_t icon_h = blueprint->icon_h;

  const int16_t left_text_x = center_x - col_gap - data_text_w;
  const int16_t left_icon_x = left_text_x - icon_text_gap - icon_w;
  const int16_t right_icon_x = center_x + col_gap;
  const int16_t right_text_x = right_icon_x + icon_text_gap + icon_w;

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

  computed->battery.icon = GRect(
      right_icon_x,
      top_row_y + icon_offset_y,
      icon_w,
      icon_h);
  computed->battery.text = GRect(
      right_text_x,
      top_row_y + text_offset_y,
      data_text_w,
      data_text_h);

  computed->climate.icon = GRect(
      left_icon_x,
      bottom_row_y + icon_offset_y,
      icon_w,
      icon_h);
  computed->climate.text = GRect(
      left_text_x,
      bottom_row_y + text_offset_y,
      data_text_w,
      data_text_h);

#ifdef PBL_HEALTH
  computed->steps.icon = GRect(
      left_icon_x,
      top_row_y + icon_offset_y,
      icon_w,
      icon_h);
  computed->steps.text = GRect(
      left_text_x,
      top_row_y + text_offset_y,
      data_text_w,
      data_text_h);

  computed->bpm.icon = GRect(
      right_icon_x,
      bottom_row_y + icon_offset_y,
      icon_w,
      icon_h);
  computed->bpm.text = GRect(
      right_text_x,
      bottom_row_y + text_offset_y,
      data_text_w,
      data_text_h);
#endif
}

bool layout_watchface_initialize(
    int16_t face_width,
    int16_t face_height,
    WatchfaceSurface* surface) {
  if (!surface) {
    return false;
  }

  memset(surface, 0, sizeof(*surface));

  bool is_compact = face_width < DESIGN_FACE_WIDTH &&
      face_height < DESIGN_FACE_HEIGHT;
  surface->style.is_compact = is_compact;
  surface->face_width = face_width;
  surface->face_height = face_height;

  const LayoutBlueprint* blueprint = &c_round_blueprint;
  CalculatedLayout computed = {0};
  architect_get_layout_from_blueprint(
      blueprint,
      &computed,
      face_width,
      face_height);

  // Calculating the surface, one stratum at a time
  // Background
  surface->background = (WatchfaceBackgroundStratum) {
    .frame = GRect(0, 0, face_width, face_height),
    .rule_enabled = true,
    .rule = computed.rule,
  };

  // Time
  surface->time.text = (WatchfaceTextSubstratum) {
    .frame = computed.time,
    .alignment = GTextAlignmentCenter,
    .font_role = WATCHFACE_FONT_ROLE_TIME,
    .color_role = WATCHFACE_COLOR_ROLE_TIME,
  };

  // Date
  surface->date.text = (WatchfaceTextSubstratum) {
    .frame = computed.date,
    .alignment = GTextAlignmentCenter,
    .font_role = WATCHFACE_FONT_ROLE_DATE,
    .color_role = WATCHFACE_COLOR_ROLE_DATE,
  };

  // Battery: top-right
  surface->battery.icon = (WatchfaceIconSubstratum) {
    .frame = computed.battery.icon,
    .is_enabled = true,
  };
  surface->battery.text = (WatchfaceTextSubstratum) {
    .frame = computed.battery.text,
    .alignment = GTextAlignmentLeft,
    .font_role = WATCHFACE_FONT_ROLE_BATTERY,
  };

#ifdef PBL_HEALTH
  // Steps: top-left
  surface->steps.icon = (WatchfaceIconSubstratum) {
    .frame = computed.steps.icon,
    .is_enabled = true,
  };

  surface->steps.text = (WatchfaceTextSubstratum) {
    .frame = computed.steps.text,
    .alignment = GTextAlignmentLeft,
    .font_role = WATCHFACE_FONT_ROLE_STEPS,
  };
#endif

  // Climate: bottom-left
  surface->climate.icon = (WatchfaceIconSubstratum) {
    .frame = computed.climate.icon,
    .is_enabled = true,
  };

  surface->climate.text = (WatchfaceTextSubstratum) {
    .frame = computed.climate.text,
    .alignment = GTextAlignmentLeft,
    .font_role = WATCHFACE_FONT_ROLE_CLIMATE,
  };

#ifdef PBL_HEALTH
  // BPM: bottom-right
  surface->bpm.icon = (WatchfaceIconSubstratum) {
    .frame = computed.bpm.icon,
    .is_enabled = true,
  };

  surface->bpm.text = (WatchfaceTextSubstratum) {
    .frame = computed.bpm.text,
    .alignment = GTextAlignmentLeft,
    .font_role = WATCHFACE_FONT_ROLE_BPM,
  };
#endif
  return true;
}
#endif
