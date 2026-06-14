#ifdef PBL_RECT
#include "layout_rect.h"

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
  const int16_t icon_w = blueprint->icon_w;
  const int16_t icon_h = blueprint->icon_h;
  const int16_t data_text_height = blueprint->data_text_height;
  const int16_t icon_text_row_height = HELPER_MAX(icon_h, data_text_height);
#ifdef PBL_HEALTH
  const int16_t bottom_row_y = face_height - margin - icon_text_row_height;
#endif
  const int16_t time_y = (face_height * DESIGN_RECT_TIME_Y_PERCENT) / 100;
  const int16_t rule_w = (face_width * DESIGN_RECT_RULE_WIDTH_PERCENT) / 100;
  const int16_t rule_x = (face_width - rule_w) >> 1;

  const int16_t battery_band_y = time_y + blueprint->time_text_height;
  const int16_t battery_band_height = DESIGN_RECT_BATTERY_BAND_HEIGHT;
  const int16_t battery_track_height = DESIGN_RECT_BATTERY_TRACK_HEIGHT;
  const int16_t battery_fill_height = DESIGN_RECT_BATTERY_FILL_HEIGHT;
  const int16_t battery_bolt_gap = DESIGN_RECT_BATTERY_BOLT_GAP;
  const int16_t battery_bolt_width = DESIGN_RECT_BATTERY_BOLT_WIDTH;
  const int16_t battery_bolt_height = DESIGN_RECT_BATTERY_BOLT_HEIGHT;
  const int16_t track_y = battery_band_y +
      ((battery_band_height - battery_track_height) >> 1);
  const int16_t fill_y = track_y +
      ((battery_track_height - battery_fill_height) >> 1);
  const int16_t weather_date_row_y = battery_band_y + battery_band_height;
  const int16_t date_text_height = blueprint->date_text_height;
  const int16_t weather_date_row_height = HELPER_MAX(
      icon_text_row_height,
      date_text_height);
  const int16_t climate_row_offset_y =
      (weather_date_row_height - icon_text_row_height) / 2;
  const int16_t date_text_offset_y =
      (weather_date_row_height - date_text_height) / 2;
  const int16_t climate_y = weather_date_row_y + climate_row_offset_y;
  const int16_t date_text_y = weather_date_row_y + date_text_offset_y;

  const int16_t left_icon_x = x_start;

  const int16_t left_text_x =
    left_icon_x + icon_w + blueprint->icon_text_gap;
  const int16_t weather_end_x = left_text_x +
      blueprint->climate_text_width;
  const int16_t date_x = weather_end_x + blueprint->icon_text_gap;
  const int16_t date_text_width = x_end - date_x;

#ifdef PBL_HEALTH
  const int16_t right_text_x = blueprint->right_text_x;
  const int16_t right_icon_x =
    right_text_x - blueprint->icon_text_gap - icon_w;
#endif

  const int16_t text_offset_y = (icon_text_row_height - data_text_height) >> 1;
  const int16_t icon_offset_y = (icon_text_row_height - icon_h) >> 1;

  computed->time = GRect(
      x_start,
      time_y,
      content_width,
      blueprint->time_text_height);

  computed->date = GRect(
      date_x,
      date_text_y,
      date_text_width,
      date_text_height);

  computed->battery.track = GRect(
      rule_x,
      track_y,
      rule_w,
      battery_track_height);
  computed->battery.fill = GRect(
      rule_x + 1,
      fill_y,
      rule_w - 2,
      battery_fill_height);
  computed->battery.bolt = GRect(
        rule_x + rule_w + battery_bolt_gap,
        battery_band_y + ((battery_band_height - battery_bolt_height) >> 1),
        battery_bolt_width,
        battery_bolt_height);

  computed->climate.icon = GRect(
      left_icon_x,
      climate_y + icon_offset_y,
      icon_w,
      icon_h);

  computed->climate.text = GRect(
      left_text_x,
      climate_y + text_offset_y,
      blueprint->climate_text_width,
      data_text_height);

#ifdef PBL_HEALTH
  computed->steps.icon = GRect(
      left_icon_x,
      bottom_row_y + icon_offset_y,
      icon_w,
      icon_h);

  computed->steps.text = GRect(
      left_text_x,
      bottom_row_y + text_offset_y,
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
    .rule_enabled = false,
    .rule = GRectZero,
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
    .alignment = GTextAlignmentRight,
    .font_role = WATCHFACE_FONT_ROLE_DATE,
    .color_role = WATCHFACE_COLOR_ROLE_DATE,
  };

  // Battery
  surface->battery = computed.battery;

  // Climate
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
// Steps
surface->steps.icon = (WatchfaceIconSubstratum) {
    .frame = computed.steps.icon,
    .is_enabled = true,
  };

  surface->steps.text = (WatchfaceTextSubstratum) {
    .frame = computed.steps.text,
    .alignment = GTextAlignmentLeft,
    .font_role = WATCHFACE_FONT_ROLE_STEPS,
  };

  // BPM
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
