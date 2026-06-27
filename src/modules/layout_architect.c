#include <stdint.h>
#include "layout.h"
#include "layout_style.h"
#include "layout_surface.h"
#include "helper.h"

// Blueprint support structures
typedef struct {
  int16_t margin;
  int16_t icon_w;
  int16_t icon_h;
  int16_t time_y_percent;
  int16_t time_text_height;
  int16_t date_text_height;
  int16_t data_text_height;
  int16_t climate_text_width;
#ifdef PBL_HEALTH
  int16_t steps_text_width;
  int16_t bpm_text_width;
#endif
} LayoutBlueprint;

static const LayoutBlueprint c_rect_reference_blueprint = {
    .margin = DESIGN_FULL_MARGIN,
    .icon_w = DESIGN_FULL_ICON_WIDTH,
    .icon_h = DESIGN_FULL_ICON_HEIGHT,
    .time_y_percent = DESIGN_TIME_Y_PERCENT,
    .date_text_height = DESIGN_FULL_DATE_TEXT_HEIGHT,
    .time_text_height = DESIGN_FULL_TIME_TEXT_HEIGHT,
    .data_text_height = DESIGN_FULL_DATA_TEXT_HEIGHT,
    .climate_text_width = DESIGN_FULL_CLIMATE_TEXT_WIDTH,
#ifdef PBL_HEALTH
    .steps_text_width = DESIGN_FULL_STEPS_TEXT_WIDTH,
    .bpm_text_width = DESIGN_FULL_BPM_TEXT_WIDTH,
#endif
};

static const LayoutBlueprint c_rect_compact_blueprint = {
    .margin = DESIGN_COMPACT_MARGIN,
    .icon_w = DESIGN_COMPACT_ICON_WIDTH,
    .icon_h = DESIGN_COMPACT_ICON_HEIGHT,
    .time_y_percent = DESIGN_COMPACT_TIME_Y_PERCENT,
    .time_text_height = DESIGN_COMPACT_TIME_TEXT_HEIGHT,
    .date_text_height = DESIGN_COMPACT_DATE_TEXT_HEIGHT,
    .data_text_height = DESIGN_COMPACT_DATA_TEXT_HEIGHT,
    .climate_text_width = DESIGN_COMPACT_CLIMATE_TEXT_WIDTH,
#ifdef PBL_HEALTH
    .steps_text_width = DESIGN_COMPACT_STEPS_TEXT_WIDTH,
    .bpm_text_width = DESIGN_COMPACT_BPM_TEXT_WIDTH,
#endif
};

static void architect_get_layout_from_blueprint(const LayoutBlueprint *blueprint,
                                                CalculatedLayout *computed, int16_t face_width,
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

  // Use these transient values to establish x and y for each module
  // Time, Battery, Date are separated by stacked together, no gaps

  // TIME is center aligned text
  // X: computed x_start
  // Y: computed using blueprint->time_y_percent
  // W: content_width
  // H: blueprint->time_text_height
  // Anchor current row-y using blueprint->time_y_percent
  int16_t current_row_y = HELPER_ROUND_UP((face_height * blueprint->time_y_percent), 100);

  // Current module's X
  int16_t module_x = x_start;

  // Current module's Y
  int16_t module_y = current_row_y;

  // Current module's WIDTH
  int16_t module_w = content_width;

  computed->time = GRect(module_x, module_y, module_w, blueprint->time_text_height);

  // BATTERY BAND is centered on the viewport and has 4-parts:
  // 1/ Negative space from where time ends until the battery track
  // 2/ Battery track: legible-color band @ center for contrast
  // 3/ State fill: centered showing battery & with calculated color
  // 4/ Negative space for visual separation from date
  // X1, X2, X4: all start at X computed using the module's width
  // X3: offset by 1 for halo, width = total-width - 2
  // Y1: computed using DESIGN_TIME_Y_PERCENT
  // Y2: Y1 + 1/2 (height_band - height_track)
  // Y3: Y2 + 1/2 (height_track - height_fill)
  // Y4: irrelevant as it is negative space
  // W: computed using DESIGN_BATTERY_BAND_WIDTH_PERCENT
  // H: DESIGN_BATTERY_BAND_HEIGHT
  // H2:DESIGN_BATTERY_TRACK_HEIGHT
  // H3:DESIGN_BATTERY_FILL_HEIGHT

  // Advance the current row's y-position by TIME's HEIGHT: Y1
  current_row_y += blueprint->time_text_height;

  // Battery Band's width
  module_w = ((face_width * DESIGN_BATTERY_BAND_WIDTH_PERCENT) / 100);

  // Center the module horizontally (centered regardless of charging bolt visibility)
  module_x = (face_width - module_w) >> 1;

  // Y2: Y1 + 1/2 (height_band - height_track)
  module_y = current_row_y + ((DESIGN_BATTERY_BAND_HEIGHT - DESIGN_BATTERY_TRACK_HEIGHT) >> 1);
  computed->battery.track = GRect(module_x, module_y, module_w, DESIGN_BATTERY_TRACK_HEIGHT);

  // Y3: Y2 + 1/2 (height_track - height_fill)
  // The exception to computing module_x and module_w before using them
  module_y += (DESIGN_BATTERY_TRACK_HEIGHT - DESIGN_BATTERY_FILL_HEIGHT) >> 1;
  computed->battery.fill = GRect(module_x + 1, module_y, module_w - 2, DESIGN_BATTERY_FILL_HEIGHT);

  // Add the bolt relative to the top of the BATTERY BAND
  module_y = current_row_y + ((DESIGN_BATTERY_BAND_HEIGHT - DESIGN_BATTERY_BOLT_HEIGHT) >> 1);
  module_x += module_w + DESIGN_ICON_TEXT_GAP;
  module_w = DESIGN_BATTERY_BOLT_WIDTH;
  computed->battery.bolt = GRect(module_x, module_y, module_w, DESIGN_BATTERY_BOLT_HEIGHT);

  // The CLIMATE and DATE row
  // Climate:
  // X: center-oriented
  // Y: TIME_Y+BATTERY_BAND_HEIGHT

  // Advance the curent row's Y by DESIGN_BATTERY_BAND_HEIGHT
  current_row_y += DESIGN_BATTERY_BAND_HEIGHT;

  // Date
  // For this row, anchor all modules at current-row's y
  // Center horizontally
  const int16_t face_center = ((face_width - module_w) >> 1);
  const int16_t icon_w = blueprint->icon_w;
  const int16_t icon_h = blueprint->icon_h;
  const int16_t data_text_height = blueprint->data_text_height;

  // Climate Icon
  module_x = face_center - (icon_w >> 1);
  module_y = current_row_y;
  module_w = icon_w;
  computed->climate.icon = GRect(module_x, module_y, module_w, icon_h);

  // Climate Text
  module_w = module_x - (DESIGN_ICON_TEXT_GAP)-x_start;
  module_x = x_start;
  computed->climate.text = GRect(module_x, module_y, module_w, data_text_height);

  // Date text
  module_x = face_center + DESIGN_ICON_TEXT_GAP + (icon_w >> 1);
  module_y = current_row_y;
  module_w = x_end - module_x; // last X - current x-coordinate yields available width
  computed->date = GRect(module_x, module_y, module_w, blueprint->date_text_height);

#ifdef PBL_HEALTH
  // BPM centered at top on screen, icon - gap - text
  // X_module: computed using face_width
  // X_icon: X_module
  // X_text: X_icon + icon_module_width
  // Y: margin
  current_row_y = margin;
  // Center horizontally
  module_x = face_center - icon_w - DESIGN_ICON_TEXT_GAP;

  // The icon is the next module, set its width
  module_w = icon_w;

  const int16_t icon_text_row_height = HELPER_MAX(icon_h, data_text_height);
  const int16_t icon_offset_y = (icon_text_row_height - icon_h) >> 1;

  // Icon
  module_y = current_row_y + icon_offset_y;
  computed->bpm.icon = GRect(module_x, module_y, module_w, icon_h);

  // Text: X_text = X_icon + icon module's width
  const int16_t text_offset_y = (icon_text_row_height - data_text_height) >> 1;
  module_x = face_center + DESIGN_ICON_TEXT_GAP;
  module_y = current_row_y + text_offset_y;
  module_w = blueprint->bpm_text_width;
  computed->bpm.text = GRect(module_x, module_y, module_w, data_text_height);

  // Steps centered at bottom on screen, icon - gap - text
  // X_module: computed using face_width
  // X_icon: X_module
  // X_text: X_icon + icon_module_width
  // Y: computed using face_height, row-top-offset, row_height
  // Progress Bar is at the bottom and starts at the icon's X and extends
  // until the end of the text-box.

  // Center horizontally
  module_x = face_center - icon_w - DESIGN_ICON_TEXT_GAP;

  // Set the progress bar at the bottom first
  current_row_y = face_height - margin;
  // The width of icon + gap + text = progress_width
  // height is the margin >> 1 (half)
  module_w = icon_w + DESIGN_ICON_TEXT_GAP + blueprint->steps_text_width;
  module_y = current_row_y + 1;
  computed->steps_layer.progress = GRect(module_x,module_y,module_w,DESIGN_STEPS_PROGRESS_HEIGHT);

  current_row_y -= icon_text_row_height;
  // The icon is the next module, set its width
  module_w = icon_w;

  // Icon
  module_y = current_row_y + icon_offset_y;
  computed->steps_layer.steps.icon = GRect(module_x, module_y, module_w, icon_h);

  // Text: X_text = X_icon + icon module's width
  module_x = face_center + DESIGN_ICON_TEXT_GAP;
  module_y = current_row_y + text_offset_y;
  module_w = blueprint->steps_text_width;
  computed->steps_layer.steps.text = GRect(module_x, module_y, module_w, data_text_height);
#endif
}

bool layout_watchface_initialize(int16_t face_width, int16_t face_height,
                                 WatchfaceSurface *surface) {
  if (!surface) {
    return false;
  }

  // Wipe the "surface" clean
  memset(surface, 0, sizeof(*surface));

  bool is_compact = face_width < DESIGN_FULL_FACE_WIDTH && face_height < DESIGN_FULL_FACE_HEIGHT;
  surface->style.is_compact = is_compact;
  surface->face_width = face_width;
  surface->face_height = face_height;

  const LayoutBlueprint *blueprint =
      is_compact ? &c_rect_compact_blueprint : &c_rect_reference_blueprint;
  CalculatedLayout computed = {0};
  architect_get_layout_from_blueprint(blueprint, &computed, face_width, face_height);

  // Time
  surface->time.text = (WatchfaceTextSubstratum){
      .frame = computed.time,
      .alignment = GTextAlignmentCenter,
      .font_role = WATCHFACE_FONT_ROLE_TIME,
      .color_role = WATCHFACE_COLOR_ROLE_TIME,
  };

  // Date
  surface->date.text = (WatchfaceTextSubstratum){
      .frame = computed.date,
      .alignment = GTextAlignmentLeft,
      .font_role = WATCHFACE_FONT_ROLE_DATE,
      .color_role = WATCHFACE_COLOR_ROLE_DATE,
  };

  // Battery
  surface->battery = computed.battery;

  // Climate
  surface->climate.icon = (WatchfaceIconSubstratum){
      .frame = computed.climate.icon,
      .is_enabled = true,
  };

  surface->climate.text = (WatchfaceTextSubstratum){
      .frame = computed.climate.text,
      .alignment = GTextAlignmentRight,
      .font_role = WATCHFACE_FONT_ROLE_TEXT,
  };

#ifdef PBL_HEALTH
  // Steps
  surface->steps.icon = (WatchfaceIconSubstratum){
      .frame = computed.steps_layer.steps.icon,
      .is_enabled = true,
  };

  surface->steps.text = (WatchfaceTextSubstratum){
      .frame = computed.steps_layer.steps.text,
      .alignment = GTextAlignmentLeft,
      .font_role = WATCHFACE_FONT_ROLE_TEXT,
  };

  surface->steps.progress = computed.steps_layer.progress;

  // BPM
  surface->bpm.icon = (WatchfaceIconSubstratum){
      .frame = computed.bpm.icon,
      .is_enabled = true,
  };
  surface->bpm.text = (WatchfaceTextSubstratum){
      .frame = computed.bpm.text,
      .alignment = GTextAlignmentLeft,
      .font_role = WATCHFACE_FONT_ROLE_TEXT,
  };
#endif
  return true;
}
