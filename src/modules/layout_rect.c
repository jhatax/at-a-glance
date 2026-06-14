#ifdef PBL_RECT
#include "layout.h"
#include "../c/ataglance.h"
#include "helper.h"

typedef enum {
  DESIGN_RECT_MARGIN = 7,
  DESIGN_RECT_ICON_TEXT_GAP = 2,
  DESIGN_RECT_ROW_GAP = 4,
  DESIGN_RECT_RULE_HEIGHT = 1,
  DESIGN_RECT_TIME_Y_PERCENT = 25,
  DESIGN_RECT_RULE_WIDTH_PERCENT = 50,
  DESIGN_RECT_DATE_WIDTH_PERCENT = 80,
} DesignCommonRect;

typedef enum {
  DESIGN_RECT_REFERENCE_DATE_TEXT_HEIGHT = 28,
  DESIGN_RECT_REFERENCE_TIME_TEXT_HEIGHT = 54,
  DESIGN_RECT_REFERENCE_DATA_TEXT_HEIGHT = 20,
  DESIGN_RECT_REFERENCE_RIGHT_TEXT_X = 153,
  DESIGN_RECT_REFERENCE_BATTERY_TEXT_WIDTH = 40,
  DESIGN_RECT_REFERENCE_CLIMATE_TEXT_WIDTH = 40,
#ifdef PBL_HEALTH
  DESIGN_RECT_REFERENCE_STEPS_TEXT_WIDTH = 40,
  DESIGN_RECT_REFERENCE_BPM_TEXT_WIDTH = 40,
#endif
} DesignReferenceRect;

typedef enum {
  DESIGN_RECT_COMPACT_ICON_WIDTH = 20,
  DESIGN_RECT_COMPACT_ICON_HEIGHT = 20,
  DESIGN_RECT_COMPACT_DATE_TEXT_HEIGHT = 20,
  DESIGN_RECT_COMPACT_TIME_TEXT_HEIGHT = 42,
  DESIGN_RECT_COMPACT_DATA_TEXT_HEIGHT = 16,
  DESIGN_RECT_COMPACT_RIGHT_TEXT_X = 106,
  DESIGN_RECT_COMPACT_BATTERY_TEXT_WIDTH = 33,
  DESIGN_RECT_COMPACT_CLIMATE_TEXT_WIDTH = 40,
#ifdef PBL_HEALTH
  DESIGN_RECT_COMPACT_STEPS_TEXT_WIDTH = 40,
  DESIGN_RECT_COMPACT_BPM_TEXT_WIDTH = 33,
#endif
} DesignCompactRect;

typedef struct {
  int16_t margin;
  int16_t row_gap;
  int16_t icon_w;
  int16_t icon_h;
  int16_t icon_text_gap;
  int16_t date_text_height;
  int16_t time_text_height;
  int16_t data_text_height;
  int16_t icon_text_pair_height;
  int16_t right_text_x;
  int16_t battery_text_width;
  int16_t climate_text_width;
#ifdef PBL_HEALTH
  int16_t steps_text_width;
  int16_t bpm_text_width;
#endif
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

static const LayoutBlueprint c_rect_reference_blueprint = {
  .margin = DESIGN_RECT_MARGIN,
  .row_gap = DESIGN_RECT_ROW_GAP,
  .icon_w = DESIGN_ICON_WIDTH,
  .icon_h = DESIGN_ICON_HEIGHT,
  .icon_text_gap = DESIGN_RECT_ICON_TEXT_GAP,
  .date_text_height = DESIGN_RECT_REFERENCE_DATE_TEXT_HEIGHT,
  .time_text_height = DESIGN_RECT_REFERENCE_TIME_TEXT_HEIGHT,
  .data_text_height = DESIGN_RECT_REFERENCE_DATA_TEXT_HEIGHT,
  .icon_text_pair_height = HELPER_MAX((int)DESIGN_ICON_HEIGHT, (int)DESIGN_RECT_REFERENCE_DATA_TEXT_HEIGHT),
  .right_text_x = DESIGN_RECT_REFERENCE_RIGHT_TEXT_X,
  .battery_text_width = DESIGN_RECT_REFERENCE_BATTERY_TEXT_WIDTH,
  .climate_text_width = DESIGN_RECT_REFERENCE_CLIMATE_TEXT_WIDTH,
#ifdef PBL_HEALTH
  .steps_text_width = DESIGN_RECT_REFERENCE_STEPS_TEXT_WIDTH,
  .bpm_text_width = DESIGN_RECT_REFERENCE_BPM_TEXT_WIDTH,
#endif
};

static const LayoutBlueprint c_rect_compact_blueprint = {
  .margin = DESIGN_RECT_MARGIN,
  .row_gap = DESIGN_RECT_ROW_GAP,
  .icon_w = DESIGN_RECT_COMPACT_ICON_WIDTH,
  .icon_h = DESIGN_RECT_COMPACT_ICON_HEIGHT,
  .icon_text_gap = DESIGN_RECT_ICON_TEXT_GAP,
  .date_text_height = DESIGN_RECT_COMPACT_DATE_TEXT_HEIGHT,
  .time_text_height = DESIGN_RECT_COMPACT_TIME_TEXT_HEIGHT,
  .data_text_height = DESIGN_RECT_COMPACT_DATA_TEXT_HEIGHT,
  .icon_text_pair_height = HELPER_MAX(DESIGN_RECT_COMPACT_ICON_HEIGHT, DESIGN_RECT_COMPACT_DATA_TEXT_HEIGHT),
  .right_text_x = DESIGN_RECT_COMPACT_RIGHT_TEXT_X,
  .battery_text_width = DESIGN_RECT_COMPACT_BATTERY_TEXT_WIDTH,
  .climate_text_width = DESIGN_RECT_COMPACT_CLIMATE_TEXT_WIDTH,
#ifdef PBL_HEALTH
  .steps_text_width = DESIGN_RECT_COMPACT_STEPS_TEXT_WIDTH,
  .bpm_text_width = DESIGN_RECT_COMPACT_BPM_TEXT_WIDTH,
#endif
};

static void architect_get_layout_from_blueprint(
    const LayoutBlueprint* blueprint,
    CalculatedLayout* computed,
    int16_t face_width,
    int16_t face_height) {
  if (!blueprint || !computed) {
    return;
  }

  // Wipe the layout's state clean
  memset(computed, 0, sizeof(*computed));

  const int16_t margin = blueprint->margin;
  const int16_t x_start = margin;
  const int16_t x_end = face_width - margin;
  const int16_t content_width = x_end - x_start;
  const int16_t top_row_y = margin;
  const int16_t icon_w = blueprint->icon_w;
  const int16_t icon_h = blueprint->icon_h;
  const int16_t data_text_height = blueprint->data_text_height;
  const int16_t icon_text_pair_height = HELPER_MAX(icon_h, data_text_height);
  const int16_t bottom_row_y = face_height - margin - icon_text_pair_height;
  const int16_t time_y = (face_height * DESIGN_RECT_TIME_Y_PERCENT) / 100;
  const int16_t rule_w = (face_width * DESIGN_RECT_RULE_WIDTH_PERCENT) / 100;
  const int16_t rule_x = (face_width - rule_w) >> 1;

  const int16_t rule_y =
    time_y + blueprint->time_text_height + blueprint->row_gap;

  const int16_t date_w = (content_width * DESIGN_RECT_DATE_WIDTH_PERCENT) / 100;
  const int16_t date_x = x_start + ((content_width - date_w) >> 1);

  const int16_t date_y =
    rule_y + DESIGN_RECT_RULE_HEIGHT + blueprint->row_gap;

  const int16_t left_icon_x = x_start;

  const int16_t left_text_x =
    left_icon_x + icon_w + blueprint->icon_text_gap;

  const int16_t right_text_x = blueprint->right_text_x;

  const int16_t right_icon_x =
    right_text_x - blueprint->icon_text_gap - icon_w;

  int16_t text_offset_y = 0;
  int16_t icon_offset_y = 0;
  int16_t height_diff = (icon_h - data_text_height) / 2;
  if (height_diff > 0) {
    text_offset_y = height_diff;
  } else if (height_diff < 0) {
    icon_offset_y = -height_diff;
  }

  computed->rule = GRect(rule_x, rule_y, rule_w, DESIGN_RECT_RULE_HEIGHT);

  computed->time = GRect(
      x_start,
      time_y,
      content_width,
      blueprint->time_text_height);

  computed->date = GRect(
      date_x,
      date_y,
      date_w,
      blueprint->date_text_height);

  computed->battery.icon = GRect(
      right_icon_x,
      top_row_y + icon_offset_y,
      icon_w,
      icon_h);

  computed->battery.text = GRect(
      right_text_x,
      top_row_y + text_offset_y,
      blueprint->battery_text_width,
      data_text_height);

  computed->climate.icon = GRect(
      left_icon_x,
      bottom_row_y + icon_offset_y,
      icon_w,
      icon_h);

  computed->climate.text = GRect(
      left_text_x,
      bottom_row_y + text_offset_y,
      blueprint->climate_text_width,
      data_text_height);

#ifdef PBL_HEALTH
  computed->steps.icon = GRect(
      left_icon_x,
      top_row_y + icon_offset_y,
      icon_w,
      icon_h);

  computed->steps.text = GRect(
      left_text_x,
      top_row_y + text_offset_y,
      blueprint->steps_text_width,
      data_text_height);

  computed->bpm.icon = GRect(
      right_icon_x,
      bottom_row_y + icon_offset_y,
      icon_w,
      icon_h);
  computed->bpm.text = GRect(
      right_text_x,
      bottom_row_y + text_offset_y,
      blueprint->bpm_text_width,
      data_text_height);
#endif
}

bool layout_watchface_initialize(
    int16_t face_width,
    int16_t face_height,
    WatchfaceSurface* surface) {
  if (!surface) {
    return false;
  }

  // Wipe the slate clean
  memset(surface, 0, sizeof(*surface));

  bool is_compact = face_width < DESIGN_FACE_WIDTH &&
      face_height < DESIGN_FACE_HEIGHT;
  surface->style.is_compact = is_compact;
  surface->face_width = face_width;
  surface->face_height = face_height;

  const LayoutBlueprint* blueprint = is_compact ?
      &c_rect_compact_blueprint : &c_rect_reference_blueprint;
  CalculatedLayout computed = {0};
  architect_get_layout_from_blueprint(
      blueprint,
      &computed,
      face_width,
      face_height);

  // Initialize the surface, one stratum at a time
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

  surface->battery.icon = (WatchfaceIconSubstratum) {
    .frame = computed.battery.icon,
    .is_enabled = true,
  };

  surface->battery.text = (WatchfaceTextSubstratum) {
    .frame = computed.battery.text,
    .alignment = GTextAlignmentLeft,
    .font_role = WATCHFACE_FONT_ROLE_BATTERY,
  };

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
  surface->steps.icon = (WatchfaceIconSubstratum) {
    .frame = computed.steps.icon,
    .is_enabled = true,
  };

  surface->steps.text = (WatchfaceTextSubstratum) {
    .frame = computed.steps.text,
    .alignment = GTextAlignmentLeft,
    .font_role = WATCHFACE_FONT_ROLE_STEPS,
  };

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
