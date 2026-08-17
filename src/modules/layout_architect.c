#include "helper.h"
#include "layout_blueprints.h"
#include "layout_surface.h"
#include "watchface_layout.h"

/*
 * File invariants:
 *
 * - geometry and prepared-surface ownership only
 *   This file computes watchface geometry and writes the calculated frames into
 *   a caller-owned WatchfaceSurface.
 *
 * - blueprint-driven layout calculation
 *   Layout is derived from blueprint constants and display dimensions, not from
 *   live runtime state, module behavior, or transport inputs.
 *
 * - must-have first, optional strata second
 *   The file calculates required watchface layout first, then optional health
 *   strata, then applies the computed result to the surface.
 *
 * - no style, runtime, or lifecycle ownership
 *   Palette selection, font loading, service events, module creation, and
 *   Pebble layer lifecycle belong elsewhere.
 *
 * - no feature behavior leakage
 *   This file decides frames and alignments only. It must not interpret source
 *   state or module refresh policy.
 */

// Blueprint support structures
typedef struct {
  int16_t margin_x;
  int16_t margin_y;
  int16_t icon_w;
  int16_t icon_h;
  int16_t icon_text_gap;
  int16_t time_y_percent;
  int16_t time_text_height;
  int16_t date_text_height;
  int16_t data_text_height;
  int16_t location_text_height;
#ifdef PBL_HEALTH
  int16_t steps_text_width;
  int16_t bpm_text_width;
#endif
} LayoutBlueprint;

static const LayoutBlueprint c_blueprint = {
    .margin_x = HELPER_IF_ELSE(IS_LARGE_DISPLAY, DESIGN_FULL_X_MARGIN, DESIGN_COMPACT_X_MARGIN),
    .margin_y = HELPER_IF_ELSE(IS_LARGE_DISPLAY, DESIGN_FULL_Y_MARGIN, DESIGN_COMPACT_Y_MARGIN),
    .icon_w = HELPER_IF_ELSE(IS_LARGE_DISPLAY, DESIGN_FULL_ICON_WIDTH, DESIGN_COMPACT_ICON_WIDTH),
    .icon_h = HELPER_IF_ELSE(IS_LARGE_DISPLAY, DESIGN_FULL_ICON_HEIGHT, DESIGN_COMPACT_ICON_HEIGHT),
    .icon_text_gap =
        HELPER_IF_ELSE(IS_LARGE_DISPLAY, DESIGN_ICON_TEXT_GAP, DESIGN_COMPACT_ICON_TEXT_GAP),
    .time_y_percent =
        HELPER_IF_ELSE(IS_LARGE_DISPLAY, DESIGN_TIME_Y_PERCENT, DESIGN_COMPACT_TIME_Y_PERCENT),
    .date_text_height = HELPER_IF_ELSE(
        IS_LARGE_DISPLAY,
        DESIGN_FULL_DATE_TEXT_HEIGHT,
        DESIGN_COMPACT_DATE_TEXT_HEIGHT),
    .time_text_height = HELPER_IF_ELSE(
        IS_LARGE_DISPLAY,
        DESIGN_FULL_TIME_TEXT_HEIGHT,
        DESIGN_COMPACT_TIME_TEXT_HEIGHT),
    .data_text_height = HELPER_IF_ELSE(
        IS_LARGE_DISPLAY,
        DESIGN_FULL_DATA_TEXT_HEIGHT,
        DESIGN_COMPACT_DATA_TEXT_HEIGHT),
    .location_text_height = HELPER_IF_ELSE(
        IS_LARGE_DISPLAY,
        DESIGN_FULL_LOCATION_TEXT_HEIGHT,
        DESIGN_COMPACT_LOCATION_TEXT_HEIGHT),
#ifdef PBL_HEALTH
    .steps_text_width = HELPER_IF_ELSE(
        IS_LARGE_DISPLAY,
        DESIGN_FULL_STEPS_TEXT_WIDTH,
        DESIGN_COMPACT_STEPS_TEXT_WIDTH),
    .bpm_text_width =
        HELPER_IF_ELSE(IS_LARGE_DISPLAY, DESIGN_FULL_BPM_TEXT_WIDTH, DESIGN_COMPACT_BPM_TEXT_WIDTH),
#endif
};

#ifdef PBL_HEALTH
static void architect_calculate_health_layout_from_blueprint(
    const LayoutBlueprint* blueprint,
    CalculatedLayout* computed,
    int16_t face_width,
    int16_t face_height) {
  const int16_t margin_y = blueprint->margin_y;
  // Icon and text are on either side of the face_center
  // Steps progress spreads on either side of x_center
  // Y: anchored at the same Y for this row
  const int16_t icon_w = blueprint->icon_w;
  const int16_t icon_h = blueprint->icon_h;
  const int16_t face_center = (face_width >> 1);
  const int16_t icon_text_gap = blueprint->icon_text_gap;
  const int16_t icon_x = face_center - (icon_w + (icon_w >> 1) + icon_text_gap);
  const int16_t metric_x = icon_x + icon_w + icon_text_gap;

  // Icon and text are on either side of the face_center
  // Module Y stays unchanged from icon's Y
  int16_t module_w = blueprint->steps_text_width;
  int16_t current_row_y = margin_y;
  int16_t module_y = current_row_y;
  computed->steps_layer.steps.text =
      GRect(metric_x, module_y, module_w, blueprint->data_text_height);

  // Icon
  module_w = icon_w;
  // The icon is the next module, set its width
  computed->steps_layer.steps.icon = GRect(icon_x, module_y, module_w, icon_h);

  // Progress Bar is at the bottom and starts at the icon's X and extends
  // until the end of the text-box. Bar's width uses Progress Bar width %
  // Set the progress bar below the steps icon and text
  // Progress width = width of the icon + gap + text
  // Center horizontally
  // Start by using the module_w to save the interim width
  current_row_y += icon_h;
  module_w = icon_w + icon_text_gap + blueprint->steps_text_width;
  // Set the x to be that of the icon so that the rect starts exactly where the
  // icon starts, which is the current value of module_x
  module_y = current_row_y;
  // height is DESIGN_STEPS_PROGRESS_HEIGHT
  computed->steps_layer.progress = GRect(icon_x, module_y, module_w, DESIGN_STEPS_PROGRESS_HEIGHT);

  // BPM at the bottom of the screen
  current_row_y = face_height - margin_y - icon_h;

  module_w = blueprint->bpm_text_width;
  module_y = current_row_y;
  computed->bpm.text = GRect(metric_x, module_y, module_w, blueprint->data_text_height);

  // Icon
  module_w = icon_w;
  computed->bpm.icon = GRect(icon_x, module_y, module_w, icon_h);
}
#endif

static void architect_calculate_must_have_layout_from_blueprint(
    const LayoutBlueprint* blueprint,
    CalculatedLayout* computed,
    int16_t face_width,
    int16_t face_height) {
  if (!blueprint || !computed) {
    return;
  }

  // Wipe the layout's state clean
  memset(computed, 0, sizeof(*computed));

  const int16_t margin_x = blueprint->margin_x;
  const int16_t x_start = margin_x;
  const int16_t x_end = face_width - margin_x;
  const int16_t content_width = x_end - x_start;
  const int16_t icon_text_gap = blueprint->icon_text_gap;

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
  // H1: DESIGN_BATTERY_BAND_HEIGHT
  // H2:DESIGN_BATTERY_TRACK_HEIGHT
  // H3:DESIGN_BATTERY_FILL_HEIGHT

  // Advance the current row's y-position by TIME's HEIGHT: Y1
  current_row_y += blueprint->time_text_height;

  // Battery Band's width
  module_w = (face_width * DESIGN_BATTERY_BAR_WIDTH_PERCENT) / 100;

  // Center the module horizontally (centered regardless of charging bolt
  // visibility)
  module_x = (face_width - module_w) >> 1;

  // The BT icon is at the same Y coordinate as the battery band
  computed->bt_icon = GRect(
      module_x - DESIGN_BT_ICON_DIMS - icon_text_gap,
      current_row_y,
      DESIGN_BT_ICON_DIMS,
      DESIGN_BT_ICON_DIMS);

  // Y2: Y1 + 1/2 (height_band - height_track)
  module_y = current_row_y + ((DESIGN_BATTERY_BAND_HEIGHT - DESIGN_BATTERY_TRACK_HEIGHT) >> 1);
  computed->battery.track = GRect(module_x, module_y, module_w, DESIGN_BATTERY_TRACK_HEIGHT);

  // Y3: Y2 + 1/2 (height_track - height_fill)
  // The exception to computing module_x and module_w before using them
  module_y += (DESIGN_BATTERY_TRACK_HEIGHT - DESIGN_BATTERY_FILL_HEIGHT) >> 1;
  computed->battery.fill = GRect(module_x + 1, module_y, module_w - 2, DESIGN_BATTERY_FILL_HEIGHT);

  // Add the bolt relative to the top of the BATTERY BAND
  module_y = current_row_y + ((DESIGN_BATTERY_BAND_HEIGHT - DESIGN_BATTERY_BOLT_HEIGHT) >> 1);
  module_x += module_w;
  module_w = DESIGN_BATTERY_BOLT_WIDTH;
  computed->battery.bolt = GRect(module_x, module_y, module_w, DESIGN_BATTERY_BOLT_HEIGHT);

  // The CLIMATE and DATE row
  // Climate:
  // X: center-oriented
  // Y: TIME_Y+BATTERY_BAND_HEIGHT

  // Advance the curent row's Y by DESIGN_BATTERY_BAND_HEIGHT
  current_row_y += DESIGN_BATTERY_BAND_HEIGHT;
  module_y = current_row_y;

  // Date
  // For this row, anchor all modules at current-row's y
  const int16_t icon_w = blueprint->icon_w;
  const int16_t icon_h = blueprint->icon_h;

  // Climate Icon is to the left of x_center
  const int16_t face_center = (face_width >> 1);
  module_w = icon_w;
  module_x = face_center - (icon_w + (icon_w >> 1) + icon_text_gap);
  computed->climate.icon = GRect(module_x, module_y, module_w, icon_h);

  // Climate Text Width can be computed
  computed->climate.text =
      GRect(x_start, module_y, module_x - icon_text_gap - x_start, blueprint->data_text_height);

  // Date text
  module_x += icon_w + icon_text_gap;
  module_w = face_width - module_x - 1;  // last X - current x-coordinate yields available width
  computed->date = GRect(module_x, module_y, module_w, blueprint->date_text_height);

  // Location text
  current_row_y += HELPER_MAX(icon_h, blueprint->data_text_height);
  computed->location = GRect(
      computed->battery.track.origin.x,
      current_row_y - 1,
      computed->battery.track.size.w,
      blueprint->location_text_height);
}

static void architect_apply_calculated_layout_to_watchface(
    WatchfaceSurface* surface,
    CalculatedLayout* computed) {
  // Time
  surface->time.text = (WatchfaceTextSubstratum){
      .frame = computed->time,
      .alignment = GTextAlignmentCenter,
      .font_role = WATCHFACE_FONT_ROLE_TIME,
      .color_role = WATCHFACE_COLOR_ROLE_TIME,
  };

  // Date
  surface->date.text = (WatchfaceTextSubstratum){
      .frame = computed->date,
      .alignment = GTextAlignmentLeft,
      .font_role = WATCHFACE_FONT_ROLE_DATE,
      .color_role = WATCHFACE_COLOR_ROLE_DATE,
  };

  // Battery
  surface->battery = (WatchfaceBatteryStratum){
      .fill = computed->battery.fill,
      .track = computed->battery.track,
      .bolt = computed->battery.bolt,
  };

  // Climate
  surface->climate.icon = (WatchfaceIconSubstratum){
      .frame = computed->climate.icon,
  };

  surface->climate.text = (WatchfaceTextSubstratum){
      .frame = computed->climate.text,
      .alignment = GTextAlignmentRight,
      .font_role = WATCHFACE_FONT_ROLE_TEXT,
  };

  surface->location = (WatchfaceTextStratum){
      .text.frame = computed->location,
      .text.alignment = GTextAlignmentCenter,
      .text.font_role = WATCHFACE_FONT_ROLE_LOCATION,
  };

  surface->bt_icon = (WatchfaceIconStratum){
      .icon.frame = computed->bt_icon,
  };

#ifdef PBL_HEALTH
  // Steps
  surface->steps.icon = (WatchfaceIconSubstratum){
      .frame = computed->steps_layer.steps.icon,
  };

  surface->steps.text = (WatchfaceTextSubstratum){
      .frame = computed->steps_layer.steps.text,
      .alignment = GTextAlignmentLeft,
      .font_role = WATCHFACE_FONT_ROLE_TEXT,
  };

  surface->steps.progress = computed->steps_layer.progress;

  // BPM
  surface->bpm.icon = (WatchfaceIconSubstratum){
      .frame = computed->bpm.icon,
  };
  surface->bpm.text = (WatchfaceTextSubstratum){
      .frame = computed->bpm.text,
      .alignment = GTextAlignmentLeft,
      .font_role = WATCHFACE_FONT_ROLE_TEXT,
  };
#endif
}

bool layout_watchface_prepare(
    int16_t face_width,
    int16_t face_height,
    WatchfaceSurface* surface) {
  if (!surface) {
    return false;
  }

  // Wipe the "surface" clean
  memset(surface, 0, sizeof(*surface));

  surface->face_width = face_width;
  surface->face_height = face_height;

  const LayoutBlueprint* blueprint = &c_blueprint;
  CalculatedLayout computed = {0};
  architect_calculate_must_have_layout_from_blueprint(
      blueprint,
      &computed,
      face_width,
      face_height);

#ifdef PBL_HEALTH
  architect_calculate_health_layout_from_blueprint(blueprint, &computed, face_width, face_height);
#endif

  architect_apply_calculated_layout_to_watchface(surface, &computed);

  return true;
}
