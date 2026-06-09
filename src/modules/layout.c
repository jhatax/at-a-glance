#include "layout.h"
#include "helper.h"
#include "settings.h"
#include "../c/ataglance.h"

static const ColorPalette c_dark_palette = {
  .background = GColorBlack,
  .primary_text = PBL_IF_COLOR_ELSE(GColorLightGray, GColorWhite),
  .unavailable_text = PBL_IF_COLOR_ELSE(GColorWindsorTan, GColorWhite),
  .date = PBL_IF_COLOR_ELSE(GColorRichBrilliantLavender, GColorWhite),
  .time = PBL_IF_COLOR_ELSE(GColorSunsetOrange, GColorWhite),
  .rule = PBL_IF_COLOR_ELSE(GColorLightGray, GColorWhite),
  .steps_icon = PBL_IF_COLOR_ELSE(GColorChromeYellow, GColorWhite),
};

static const ColorPalette c_light_palette = {
  .background = GColorWhite,
  .primary_text = GColorBlack,
  .unavailable_text = PBL_IF_COLOR_ELSE(GColorLightGray, GColorBlack),
  .date = PBL_IF_COLOR_ELSE(GColorImperialPurple, GColorBlack),
  .time = PBL_IF_COLOR_ELSE(GColorSunsetOrange, GColorBlack),
  .rule = PBL_IF_COLOR_ELSE(GColorLightGray, GColorBlack),
  .steps_icon = PBL_IF_COLOR_ELSE(GColorChromeYellow, GColorBlack),
};

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

static bool is_valid_design_x_coord(int16_t x, int16_t design_width) {
  return (x >=0 && x <=design_width);
}

static bool is_valid_design_y_coord(int16_t y, int16_t design_height) {
  return (y >=0 && y <=design_height);
}

static bool layout_is_compact_display(
    int16_t face_width,
    int16_t face_height) {
#if defined(PBL_ROUND)
  return face_width < ATAGLANCE_ROUND_FULL_FACE_WIDTH ||
      face_height < ATAGLANCE_ROUND_FULL_FACE_HEIGHT;
#else
  return face_width < ATAGLANCE_DESIGN_FACE_WIDTH ||
      face_height < ATAGLANCE_DESIGN_FACE_HEIGHT;
#endif
}

static const char* layout_font_key_for_role(
    WatchfaceFontRole role,
    bool is_compact) {
  switch (role) {
    case WATCHFACE_FONT_ROLE_DATE:
      return is_compact ?
          ATAGLANCE_FONT_KEY_DATE_COMPACT :
          ATAGLANCE_FONT_KEY_DATE_FULL;
    case WATCHFACE_FONT_ROLE_TIME:
      return is_compact ?
          ATAGLANCE_FONT_KEY_TIME_COMPACT :
          ATAGLANCE_FONT_KEY_TIME_FULL;
    case WATCHFACE_FONT_ROLE_BPM:
      return is_compact ?
          ATAGLANCE_FONT_KEY_BPM_COMPACT :
          ATAGLANCE_FONT_KEY_BPM_FULL;
    case WATCHFACE_FONT_ROLE_STEPS:
      return is_compact ?
          ATAGLANCE_FONT_KEY_STEPS_COMPACT :
          ATAGLANCE_FONT_KEY_STEPS_FULL;
    case WATCHFACE_FONT_ROLE_BATTERY:
      return is_compact ?
          ATAGLANCE_FONT_KEY_BATTERY_COMPACT :
          ATAGLANCE_FONT_KEY_BATTERY_FULL;
    case WATCHFACE_FONT_ROLE_CLIMATE:
      return is_compact ?
          ATAGLANCE_FONT_KEY_CLIMATE_COMPACT :
          ATAGLANCE_FONT_KEY_CLIMATE_FULL;
    case WATCHFACE_FONT_ROLE_COUNT:
      return ATAGLANCE_FONT_KEY_BPM_FULL;
    default:
      return ATAGLANCE_FONT_KEY_BPM_FULL;
  }
}

static const ColorPalette* layout_palette_for_display_mode(
    uint8_t display_mode) {
  if (display_mode == DISPLAY_MODE_LIGHT) {
    return &c_light_palette;
  }

  return &c_dark_palette;
}

int16_t layout_scale_icon_x(
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

int16_t layout_scale_icon_y(
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

GPoint layout_scaled_icon_point(const GSize* size, int16_t x, int16_t y) {
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

GColor layout_color_for_role(
    const ColorPalette* palette,
    WatchfaceColorRole role) {
  if (!palette) {
    return GColorWhite;
  }

  switch (role) {
    case WATCHFACE_COLOR_ROLE_PRIMARY_TEXT:
      return palette->primary_text;
    case WATCHFACE_COLOR_ROLE_UNAVAILABLE_TEXT:
      return palette->unavailable_text;
    case WATCHFACE_COLOR_ROLE_DATE:
      return palette->date;
    case WATCHFACE_COLOR_ROLE_TIME:
      return palette->time;
    case WATCHFACE_COLOR_ROLE_RULE:
      return palette->rule;
    case WATCHFACE_COLOR_ROLE_STEPS_ICON:
      return palette->steps_icon;
    case WATCHFACE_COLOR_ROLE_DYNAMIC:
      return palette->primary_text;
    default:
      return palette->primary_text;
  }
}

void layout_update_text_layer(
    TextLayer* layer,
    const char* text,
    GColor text_color) {
  if (!layer || !text) {
    return;
  }

  text_layer_set_background_color(layer, GColorClear);
  text_layer_set_text_color(layer, text_color);
  text_layer_set_text(layer, text);
}

void layout_update_surface_style(
    WatchfaceSurface* surface,
    uint8_t display_mode) {
  if (!surface) {
    return;
  }

  const bool is_compact = layout_is_compact_display(
      surface->face_width,
      surface->face_height);
  surface->style.palette = layout_palette_for_display_mode(display_mode);

  for (uint8_t i = 0; i < WATCHFACE_FONT_ROLE_COUNT; ++i) {
    surface->style.fonts[i] = fonts_get_system_font(
        layout_font_key_for_role((WatchfaceFontRole)i, is_compact));
  }
}

void layout_calculate_surface(
    int16_t face_width,
    int16_t face_height,
    uint8_t display_mode,
    WatchfaceSurface* surface) {
  if (!surface) {
    return;
  }

  memset(surface, 0, sizeof(*surface));

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

  surface->face_width = face_width;
  surface->face_height = face_height;
  surface->background_frame = GRect(0, 0, face_width, face_height);
  surface->content_x = x_content_start;
  surface->row_gap = row_gap;
  surface->column_gap = column_gap;
  surface->content_width_date = content_width;
  surface->content_width_time = content_width;
  surface->content_width_health = content_width;
  surface->content_width_bottom = content_width;

  layout_update_surface_style(surface, display_mode);

  surface->date.text = (WatchfaceTextSubstratum) {
    .frame = GRect(x_content_start,
                   date_row_top,
                   surface->content_width_date,
                   date_row_height),
    .alignment = GTextAlignmentLeft,
    .font_role = WATCHFACE_FONT_ROLE_DATE,
    .color_role = WATCHFACE_COLOR_ROLE_DATE,
  };
  surface->time.text = (WatchfaceTextSubstratum) {
    .frame = GRect(x_content_start,
                   time_row_top,
                   surface->content_width_time,
                   time_row_height),
    .alignment = GTextAlignmentLeft,
    .font_role = WATCHFACE_FONT_ROLE_TIME,
    .color_role = WATCHFACE_COLOR_ROLE_TIME,
  };
  surface->bpm.icon = (WatchfaceIconSubstratum) {
    .frame = GRect(bpm_icon_x, metrics_icon_top, icon_w, icon_h),
    .is_enabled = true,
    .requires_update_proc = true,
    .color_role = WATCHFACE_COLOR_ROLE_DYNAMIC,
  };
  surface->bpm.text = (WatchfaceTextSubstratum) {
    .frame = GRect(bpm_value_x,
                   metrics_text_top,
                   metrics_left_text_width,
                   metrics_text_height),
    .alignment = GTextAlignmentLeft,
    .font_role = WATCHFACE_FONT_ROLE_BPM,
    .color_role = WATCHFACE_COLOR_ROLE_DYNAMIC,
  };
  surface->steps.icon = (WatchfaceIconSubstratum) {
    .frame = GRect(steps_icon_x, metrics_icon_top, icon_w, icon_h),
    .is_enabled = true,
    .requires_update_proc = true,
    .color_role = WATCHFACE_COLOR_ROLE_DYNAMIC,
  };
  surface->steps.text = (WatchfaceTextSubstratum) {
    .frame = GRect(steps_value_x,
                   metrics_text_top,
                   metrics_right_text_width,
                   metrics_text_height),
    .alignment = GTextAlignmentLeft,
    .font_role = WATCHFACE_FONT_ROLE_STEPS,
    .color_role = WATCHFACE_COLOR_ROLE_DYNAMIC,
  };
  surface->climate.icon = (WatchfaceIconSubstratum) {
    .frame = GRect(weather_icon_x, bottom_icon_top, icon_w, icon_h),
    .is_enabled = true,
    .requires_update_proc = true,
    .color_role = WATCHFACE_COLOR_ROLE_DYNAMIC,
  };
  surface->climate.text = (WatchfaceTextSubstratum) {
    .frame = GRect(temperature_value_x,
                   bottom_text_top,
                   bottom_row_left_text_width,
                   bottom_text_height),
    .alignment = GTextAlignmentLeft,
    .font_role = WATCHFACE_FONT_ROLE_CLIMATE,
    .color_role = WATCHFACE_COLOR_ROLE_DYNAMIC,
  };
  surface->battery.icon = (WatchfaceIconSubstratum) {
    .frame = GRect(battery_icon_x, bottom_icon_top, icon_w, icon_h),
    .is_enabled = true,
    .requires_update_proc = true,
    .color_role = WATCHFACE_COLOR_ROLE_DYNAMIC,
  };
  surface->battery.text = (WatchfaceTextSubstratum) {
    .frame = GRect(battery_value_x,
                   bottom_text_top,
                   bottom_row_right_text_width,
                   bottom_text_height),
    .alignment = GTextAlignmentLeft,
    .font_role = WATCHFACE_FONT_ROLE_BATTERY,
    .color_role = WATCHFACE_COLOR_ROLE_DYNAMIC,
  };
  surface->rule = (WatchfaceRuleSubstratum) {
    .is_enabled = true,
    .start = GPoint(rule_left, display_center),
    .end = GPoint(rule_right, display_center),
    .color_role = WATCHFACE_COLOR_ROLE_RULE,
  };
}
