#include "helper_computations.h"
#include "layout_blueprints.h"
#include "layout_surface.h"
#include "watchface_components.h"
#include "watchface_layout.h"

/*
 * File invariants:
 *
 * - geometry and prepared-surface ownership only
 *   This file computes watchface geometry and writes the calculated frames into
 *   a caller-owned WatchfaceSurface.
 *
 * - blueprint-driven layout calculation
 *   Layout is derived from blueprint constants and display dimensions.
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

#ifdef PBL_HEALTH
static void architect_calculate_health_layout(
    CalculatedLayout* computed,
    int16_t face_width,
    int16_t face_height) {
  // Icon and text are on either side of the face_center
  // Steps progress spreads on either side of x_center
  // Y: anchored at the same Y for this row
  const int16_t face_center = (face_width >> 1);
  const int16_t icon_x = face_center - (HELPER_SCALE_ROUND(ICON_WIDTH, 3, 2) + ICON_TEXT_GAP);
  const int16_t metric_x = icon_x + ICON_WIDTH + ICON_TEXT_GAP;

  // Icon and text are on either side of the face_center
  // Module Y stays unchanged from icon's Y
  int16_t module_w = STEPS_TEXT_WIDTH;
  int16_t current_row_y = Y_MARGIN;
  int16_t module_y = current_row_y;
  computed->steps_layer.steps.text = GRect(metric_x, module_y, module_w, DATA_TEXT_HEIGHT);

  // Icon
  module_w = ICON_WIDTH;
  // The icon is the next module, set its width
  computed->steps_layer.steps.icon = GRect(icon_x, module_y, module_w, ICON_HEIGHT);

  // Progress Bar is at the bottom and starts at the icon's X and extends
  // until the end of the text-box. Bar's width uses Progress Bar width %
  // Set the progress bar below the steps icon and text
  // Progress width = width of the icon + gap + text
  // Center horizontally
  // Start by using the module_w to save the interim width
  current_row_y += ICON_HEIGHT;
  module_w = ICON_WIDTH + ICON_TEXT_GAP + STEPS_TEXT_WIDTH;
  // Set the x to be that of the icon so that the rect starts exactly where the
  // icon starts, which is the current value of module_x
  module_y = current_row_y;
  // height is STEPS_PROGRESS_HEIGHT
  computed->steps_layer.progress = GRect(icon_x, module_y, module_w, STEPS_PROGRESS_HEIGHT);

  // BPM at the bottom of the screen
  current_row_y = face_height - Y_MARGIN - ICON_HEIGHT;

  module_w = BPM_TEXT_WIDTH;
  module_y = current_row_y;
  computed->bpm.text = GRect(metric_x, module_y, module_w, DATA_TEXT_HEIGHT);

  // Icon
  module_w = ICON_WIDTH;
  computed->bpm.icon = GRect(icon_x, module_y, module_w, ICON_HEIGHT);
}
#endif

static int16_t place_battery_bar_vertically(
    CalculatedLayout* computed,
    GRect* time,
    int16_t face_width,
    int16_t face_height) {
#ifdef PBL_RECT
  // The battery bar is on the right, so the X calculation is easy
  // All modules calculate their X coordinate relative to this first value
  // Don't reset it without understand consequences
  face_width -= ICON_TEXT_GAP;   // inset by icon_text_gap
  face_height -= ICON_TEXT_GAP;  // inset by icon_text_gap
  // All icons sit right below time to the left of the battery bar
  // And to the left of the battery bar
  int16_t module_x = face_width - (BATTERY_TRACK_HEIGHT + ICON_TEXT_GAP + BATTERY_BOLT_WIDTH +
                                   ICON_TEXT_GAP + BT_ICON_WIDTH);
  int16_t module_h = BT_ICON_HEIGHT;
  int16_t module_y = time->origin.y + time->size.h;
  computed->bt_icon = GRect(module_x, module_y, BT_ICON_WIDTH, BT_ICON_HEIGHT);

  module_x += (BT_ICON_WIDTH + ICON_TEXT_GAP);
  computed->battery.bolt = GRect(module_x, module_y, BATTERY_BOLT_WIDTH, BATTERY_BOLT_HEIGHT);

  module_x += (BATTERY_BOLT_WIDTH + ICON_TEXT_GAP);
  module_h = HELPER_SCALE_ROUND(face_height, BATTERY_BAR_SIZE_PERCENT, 100);
  module_y = (face_height - module_h) >> 1;
  computed->battery.track = GRect(module_x, module_y, BATTERY_TRACK_HEIGHT, module_h);
  computed->battery.fill = GRect(
      module_x + BATTERY_TRACK_FILL_OFFSET,
      module_y + BATTERY_TRACK_FILL_OFFSET,
      BATTERY_FILL_HEIGHT,
      module_h - BATTERY_TRACK_FILL_DIFF);
#else
  face_width -= (ICON_TEXT_GAP << 1);   // inset by icon_text_gap: left and right
  face_height -= (ICON_TEXT_GAP << 1);  // inset by icon_text_gap: top and bottom
  computed->battery.track = GRect(ICON_TEXT_GAP, ICON_TEXT_GAP, face_width, face_height);
  computed->battery.fill =
      grect_inset(computed->battery.track, GEdgeInsets(BATTERY_TRACK_FILL_OFFSET));
#endif

  // Battery is vertical regardless of geometry
  computed->battery.is_vertical = true;
  return HELPER_MAX(BATTERY_BOLT_HEIGHT, BT_ICON_HEIGHT);
}

static int16_t place_battery_bar_horizontally(
    CalculatedLayout* computed,
    GRect* time,
    int16_t face_width,
    int16_t face_height) {
  // Battery Band's width
  int16_t module_w = HELPER_SCALE_ROUND(face_width, BATTERY_BAR_SIZE_PERCENT, 100);

  // Center the module horizontally (centered regardless of charging bolt
  // visibility)
  int16_t module_x = (face_width - module_w) >> 1;
  int16_t current_row_y = time->origin.y + time->size.h;

  // The BT icon is at the same Y coordinate as the battery band
  computed->bt_icon =
      GRect(module_x - BT_ICON_WIDTH - ICON_TEXT_GAP, current_row_y, BT_ICON_WIDTH, BT_ICON_WIDTH);

  // Y2: Y1 + 1/2 (height_band - height_track)
  int16_t module_y = current_row_y + ((BATTERY_BAND_HEIGHT - BATTERY_TRACK_HEIGHT) >> 1);
  computed->battery.track = GRect(module_x, module_y, module_w, BATTERY_TRACK_HEIGHT);

  // Y3: Y2 + 1/2 (height_track - height_fill)
  // The exception to computing module_x and module_w before using them
  computed->battery.fill = GRect(
      module_x + BATTERY_TRACK_FILL_OFFSET,
      module_y + BATTERY_TRACK_FILL_OFFSET,
      module_w - BATTERY_TRACK_FILL_DIFF,
      BATTERY_FILL_HEIGHT);

  // Add the bolt to the right of the battery track
  module_y = current_row_y;
  module_x += computed->battery.track.size.w + ICON_TEXT_GAP;
  computed->battery.bolt = GRect(module_x, module_y, BATTERY_BOLT_WIDTH, BATTERY_BOLT_HEIGHT);

  // Battery is horizontal regardless of geometry
  computed->battery.is_vertical = false;
  return BATTERY_BAND_HEIGHT;
}

static void architect_calculate_must_have_layout(
    CalculatedLayout* computed,
    int16_t face_width,
    int16_t face_height,
    bool is_battery_vertical) {
  if (!computed) {
    return;
  }

  // Wipe the layout's state clean
  memset(computed, 0, sizeof(*computed));

  const int16_t x_end = face_width - X_MARGIN;
  // Use these transient values to establish x and y for each module
  // Time, Battery, Date are separated by stacked together, no gaps

  // TIME is center aligned text that starts at the left margin
  // Anchor current row-y using TIME_Y_PERCENT
  int16_t current_row_y = HELPER_ROUND_UP((face_height * TIME_Y_PERCENT), 100);
  int16_t module_x = X_MARGIN;

  // Current module's WIDTH
  int16_t module_w = x_end - X_MARGIN;
  computed->time = GRect(module_x, current_row_y, module_w, TIME_TEXT_HEIGHT);

  // Advance the current row's y-position by TIME's HEIGHT: Y1
  // The Y-position of the charging bolt and BT-icon are going to be the same
  // regardless of whether the bar is horizontal or vertical
  current_row_y += TIME_TEXT_HEIGHT;
  // Advance the curent row's Y by BATTERY_BAND_HEIGHT
  if (is_battery_vertical) {
    current_row_y +=
        place_battery_bar_vertically(computed, &computed->time, face_width, face_height);
  } else {
    current_row_y +=
        place_battery_bar_horizontally(computed, &computed->time, face_width, face_height);
  }

  module_w = HELPER_SCALE_ROUND(HORIZ_RULE_SIZE_PERCENT, face_width, 100);
  module_x = (face_width - module_w) >> 1;
  computed->horiz_rule = GRect(
      module_x,
      current_row_y - ((current_row_y - computed->time.origin.y + HORIZ_RULE_HEIGHT) >> 1),
      module_w,
      HORIZ_RULE_HEIGHT);

  // Climate and Date
  // For this row, anchor all modules at current-row's y
  // Climate Icon is at X_MARGIN
  module_w = ICON_WIDTH;
  module_x = X_MARGIN;
  computed->climate.icon = GRect(module_x, current_row_y, module_w, ICON_HEIGHT);

  // Climate Text Width can be computed
  module_x += (ICON_WIDTH + ICON_TEXT_GAP);
  // width split between climate and date is 35%:65% in favor of date
  computed->climate.text = GRect(
      module_x,
      current_row_y,
      HELPER_SCALE_ROUND(x_end - module_x, TEMP_X_PERCENT, 100),
      DATA_TEXT_HEIGHT);

  // Date text
  module_x += computed->climate.text.size.w;
  module_w = x_end - module_x;
  computed->date = GRect(module_x, current_row_y, module_w, DATE_TEXT_HEIGHT);

  // Location text
  current_row_y += (HELPER_MAX(ICON_HEIGHT, DATE_TEXT_HEIGHT) - 1);
  module_w = HELPER_SCALE_ROUND(face_width, LOCATION_TEXT_WIDTH_PERCENT, 100);
  module_x = ((face_width - module_w) >> 1);
  computed->location = GRect(module_x, current_row_y, module_w, LOCATION_TEXT_HEIGHT);
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
      .alignment = GTextAlignmentCenter,
      .font_role = WATCHFACE_FONT_ROLE_DATE,
      .color_role = WATCHFACE_COLOR_ROLE_DATE,
  };

  surface->horiz_rule = computed->horiz_rule;

  // Battery
  surface->battery = (WatchfaceBatteryStratum){
      .fill = computed->battery.fill,
      .track = computed->battery.track,
      .bolt = computed->battery.bolt,
      .is_vertical = computed->battery.is_vertical,
  };

  // Climate
  surface->climate.icon = (WatchfaceIconSubstratum){
      .frame = computed->climate.icon,
  };

  surface->climate.text = (WatchfaceTextSubstratum){
      .frame = computed->climate.text,
      .alignment = GTextAlignmentLeft,
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
    bool is_battery_vertical,
    WatchfaceSurface* surface) {
  if (!surface) {
    return false;
  }

  // Wipe the "surface" clean
  memset(surface, 0, sizeof(*surface));

  surface->face_width = face_width;
  surface->face_height = face_height;

  CalculatedLayout computed = {0};
  architect_calculate_must_have_layout(&computed, face_width, face_height, is_battery_vertical);

#ifdef PBL_HEALTH
  architect_calculate_health_layout(&computed, face_width, face_height);
#endif

  architect_apply_calculated_layout_to_watchface(surface, &computed);

  return true;
}

// Every component that is re-layed out should be re-framed using
// the Pebble SDK's layer_set_frame(GRect) and a layer_mark_dirty
void relayout_battery_bolt_bticon(
    int16_t face_width,
    int16_t face_height,
    bool is_battery_vertical,
    WatchfaceSurface* surface) {
  CalculatedLayout computed = {0};
  if (is_battery_vertical) {
    (void)place_battery_bar_vertically(
        &computed,
        &(surface->time.text.frame),
        face_width,
        face_height);
  } else {
    (void)place_battery_bar_horizontally(
        &computed,
        &(surface->time.text.frame),
        face_width,
        face_height);
  }

  // Battery and bolt
  surface->battery = (WatchfaceBatteryStratum){
      .fill = computed.battery.fill,
      .track = computed.battery.track,
      .bolt = computed.battery.bolt,
      .is_vertical = is_battery_vertical,
  };

  // Bluetooth icon
  surface->bt_icon = (WatchfaceIconStratum){
      .icon.frame = computed.bt_icon,
  };
}
