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

static void architect_get_layout_from_blueprint(
    const LayoutBlueprint* blueprint,
    CalculatedLayout* computed,
    int16_t face_width,
    int16_t face_height) {
  if (!blueprint || !computed) {
    return;
  }

  memset(computed, 0, sizeof(*computed));

  const int16_t x_start = blueprint->margin;
  const int16_t x_end = face_width - blueprint->margin;
  const int16_t content_width = x_end - x_start;
  const int16_t rule_y = face_height >> 1;
  const int16_t top_row_y = blueprint->y_start;
  const int16_t bottom_row_y = blueprint->y_end -
      blueprint->icon_text_pair_height;

  const int16_t left_icon_x = x_start;
  const int16_t left_text_x = left_icon_x + blueprint->icon_w +
      blueprint->icon_text_gap;
  const int16_t right_icon_x = x_end - blueprint->data_text_width -
      blueprint->icon_text_gap - blueprint->icon_w;
  const int16_t right_text_x = right_icon_x + blueprint->icon_w +
      blueprint->icon_text_gap;

  int16_t text_offset_y = 0;
  int16_t icon_offset_y = 0;
  int16_t height_diff = (blueprint->icon_h -
      blueprint->data_text_height) / 2;
  if (height_diff > 0) {
    text_offset_y = height_diff;
  } else if (height_diff < 0) {
    icon_offset_y = -height_diff;
  }

  computed->rule = GRect(x_start, rule_y, content_width, DESIGN_HORIZON_H);

  computed->time = GRect(
      x_start,
      rule_y - blueprint->time_text_height - blueprint->row_gap,
      content_width,
      blueprint->time_text_height);
  computed->date = GRect(
      x_start,
      rule_y + blueprint->row_gap,
      content_width,
      blueprint->date_text_height);

  computed->battery.icon = GRect(
      right_icon_x,
      top_row_y + icon_offset_y,
      blueprint->icon_w,
      blueprint->icon_h);
  computed->battery.text = GRect(
      right_text_x,
      top_row_y + text_offset_y,
      blueprint->data_text_width,
      blueprint->data_text_height);

  computed->climate.icon = GRect(
      left_icon_x,
      bottom_row_y + icon_offset_y,
      blueprint->icon_w,
      blueprint->icon_h);
  computed->climate.text = GRect(
      left_text_x,
      bottom_row_y + text_offset_y,
      blueprint->data_text_width,
      blueprint->data_text_height);

#ifdef PBL_HEALTH
  computed->steps.icon = GRect(
      left_icon_x,
      top_row_y + icon_offset_y,
      blueprint->icon_w,
      blueprint->icon_h);
  computed->steps.text = GRect(
      left_text_x,
      top_row_y + text_offset_y,
      blueprint->data_text_width,
      blueprint->data_text_height);

  computed->bpm.icon = GRect(
      right_icon_x,
      bottom_row_y + icon_offset_y,
      blueprint->icon_w,
      blueprint->icon_h);
  computed->bpm.text = GRect(
      right_text_x,
      bottom_row_y + text_offset_y,
      blueprint->data_text_width,
      blueprint->data_text_height);
#endif
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
}
#endif
