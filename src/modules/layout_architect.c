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
    WatchfaceSurface* surface) {
  if (!surface) {
    return;
  }

  const int16_t face_width = surface->face_width;
  const int16_t face_height = surface->face_height;

  // Y: anchored at the same Y for this row
  const int16_t face_center = (face_width >> 1);
  const int16_t icon_x = face_center - (HELPER_SCALE_ROUND(ICON_WIDTH, 3, 2) + ICON_TEXT_GAP);
  const int16_t metric_x = icon_x + ICON_WIDTH + ICON_TEXT_GAP;

  int16_t module_w = STEPS_TEXT_WIDTH;
  int16_t current_row_y = Y_MARGIN;
  int16_t module_y = current_row_y;
  // Steps Text
  surface->steps.text = (WatchfaceTextSubstratum){
      .frame = GRect(metric_x, module_y, module_w, DATA_TEXT_HEIGHT),
      .alignment = GTextAlignmentLeft,
      .font_role = WATCHFACE_FONT_ROLE_TEXT,
  };

  // Icon
  module_w = ICON_WIDTH;
  // The icon is the next module, set its width
  surface->steps.icon = GRect(icon_x, module_y, module_w, ICON_HEIGHT);

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
  surface->steps.progress = GRect(icon_x, module_y, module_w, STEPS_PROGRESS_HEIGHT);

  // BPM at the bottom of the screen
  current_row_y = face_height - Y_MARGIN - ICON_HEIGHT;

  module_w = BPM_TEXT_WIDTH;
  module_y = current_row_y;
  surface->bpm.text = (WatchfaceTextSubstratum){
      .frame = GRect(metric_x, module_y, module_w, DATA_TEXT_HEIGHT),
      .alignment = GTextAlignmentLeft,
      .font_role = WATCHFACE_FONT_ROLE_TEXT,
  };

  // BPM Icon
  module_w = ICON_WIDTH;
  surface->bpm.icon = GRect(icon_x, module_y, module_w, ICON_HEIGHT);
}
#endif

static int16_t place_battery_bar_vertically(
    WatchfaceSurface* surface) {
  if (!surface) {
    return 0;
  }
  int16_t module_x = 0;
  int16_t module_y = 0;
  GRect* time = &(surface->time.frame);
  int16_t face_width = surface->face_width;
  int16_t face_height = surface->face_height;

#ifdef PBL_RECT
  // Inset the x-coordinate by icon_text_gap which is equal to or greater than the gap
  // recommended by Pebble for x-margin
  face_width -= ICON_TEXT_GAP;
  // The battery bar is on the right, so the X calculation is easy
  // All modules calculate their X coordinate relative to this first value
  // Don't reset it without understand consequences
  // All icons sit right below time to the left of the battery bar
  // And to the left of the battery bar
  module_x = face_width - (BATTERY_TRACK_HEIGHT);
  int16_t module_h = HELPER_SCALE_ROUND(face_height, BATTERY_BAR_SIZE_PERCENT, 100);
  module_y = (face_height - module_h) >> 1;
  surface->battery.track = GRect(module_x, module_y, BATTERY_TRACK_HEIGHT, module_h);
  surface->battery.fill = GRect(
      module_x + BATTERY_TRACK_FILL_OFFSET,
      module_y + BATTERY_TRACK_FILL_OFFSET,
      BATTERY_FILL_HEIGHT,
      module_h - BATTERY_TRACK_FILL_DIFF);
#else
  face_width -= (ICON_TEXT_GAP << 1);   // inset by icon_text_gap: left and right
  face_height -= (ICON_TEXT_GAP << 1);  // inset by icon_text_gap: top and bottom
  surface->battery.track = GRect(ICON_TEXT_GAP, ICON_TEXT_GAP, face_width, face_height);
  surface->battery.fill =
      grect_inset(surface->battery.track, GEdgeInsets(BATTERY_TRACK_FILL_OFFSET));
#endif
  // All icons sit right below time to the left of the battery bar
  module_x = face_width - (BATTERY_TRACK_HEIGHT + ICON_TEXT_GAP + BATTERY_BOLT_WIDTH);
  module_y = time->origin.y + time->size.h;
  surface->battery.bolt = GRect(module_x, module_y, BATTERY_BOLT_WIDTH, BATTERY_BOLT_HEIGHT);

  module_x -= (BT_ICON_WIDTH + ICON_TEXT_GAP);
  surface->bt_icon = GRect(module_x, module_y, BT_ICON_WIDTH, BT_ICON_HEIGHT);

  return HELPER_MAX(BATTERY_BOLT_HEIGHT, BT_ICON_HEIGHT);
}

static int16_t place_battery_bar_horizontally(
    WatchfaceSurface* surface) {
  if (!surface) {
    return 0;
  }
  GRect* time = &(surface->time.frame);
  int16_t face_width = surface->face_width;

  // Battery Band's width
  int16_t battery_track_w = HELPER_SCALE_ROUND(face_width, BATTERY_BAR_SIZE_PERCENT, 100);

  // Center the module horizontally (centered regardless of charging bolt
  // visibility)
  int16_t module_x = (face_width - battery_track_w) >> 1;
  int16_t current_row_y = time->origin.y + time->size.h;

  // The BT icon is at the same Y coordinate as the battery band
  surface->bt_icon =
      GRect(module_x - BT_ICON_WIDTH - ICON_TEXT_GAP, current_row_y, BT_ICON_WIDTH, BT_ICON_WIDTH);
  // Y2: Y1 + 1/2 (height_band - height_track)
  int16_t module_y = current_row_y + ((BATTERY_BAND_HEIGHT - BATTERY_TRACK_HEIGHT) >> 1);
  // Battery: Track first
  surface->battery.track = GRect(module_x, module_y, battery_track_w, BATTERY_TRACK_HEIGHT);

  // Battery: Fill
  // Y3: Y2 + 1/2 (height_track - height_fill)
  // The exception to computing module_x and module_w before using them
  surface->battery.fill = GRect(
      module_x + BATTERY_TRACK_FILL_OFFSET,
      module_y + BATTERY_TRACK_FILL_OFFSET,
      battery_track_w - BATTERY_TRACK_FILL_DIFF,
      BATTERY_FILL_HEIGHT);

  // Add the bolt to the right of the battery track
  module_y = current_row_y;
  module_x += battery_track_w + ICON_TEXT_GAP;
  surface->battery.bolt = GRect(module_x, module_y, BATTERY_BOLT_WIDTH, BATTERY_BOLT_HEIGHT);

  return BATTERY_BAND_HEIGHT;
}

static void architect_calculate_must_have_layout(
    WatchfaceSurface* surface) {
  if (!surface) {
    return;
  }

  const int16_t face_width = surface->face_width;
  const int16_t face_height = surface->face_height;
  const int16_t x_end = face_width - X_MARGIN;
  // Use these transient values to establish x and y for each module
  // Time, Battery, Date are separated by stacked together, no gaps

  // TIME is center aligned text that starts at the left margin
  // Anchor current row-y using TIME_Y_PERCENT
  int16_t current_row_y = HELPER_ROUND_UP((face_height * TIME_Y_PERCENT), 100);
  int16_t module_x = X_MARGIN;

  // Time
  // Current module's WIDTH
  int16_t module_w = x_end - X_MARGIN;
  surface->time = (WatchfaceTextStratum){
      .frame = GRect(module_x, current_row_y, module_w, TIME_TEXT_HEIGHT),
      .alignment = GTextAlignmentCenter,
      .font_role = WATCHFACE_FONT_ROLE_TIME,
      .color_role = WATCHFACE_COLOR_ROLE_TIME,
  };

  // Advance the current row's y-position by TIME's HEIGHT
  // The Y-position of the charging bolt and BT-icon are going to be the same
  // regardless of whether the bar is horizontal or vertical
  current_row_y += TIME_TEXT_HEIGHT;
  // Advance the curent row's Y by BATTERY_BAND_HEIGHT
  int battery_or_rule_h = 0;
  if (surface->battery.is_vertical) {
    battery_or_rule_h = place_battery_bar_vertically(surface);
  } else {
    battery_or_rule_h = place_battery_bar_horizontally(surface);
  }

  module_w = HELPER_SCALE_ROUND(HORIZ_RULE_SIZE_PERCENT, face_width, 100);
  module_x = (face_width - module_w) >> 1;
  // horiz_row_offset = (next_row_h - HORIZ_RULE_HEIGHT) / 2
  surface->horiz_rule = GRect(
      module_x,
      current_row_y + ((battery_or_rule_h - HORIZ_RULE_HEIGHT) >> 1),
      module_w,
      HORIZ_RULE_HEIGHT);

  // Climate and Date
  // Advance the current_row_y by battery_or_rule_h
  current_row_y += battery_or_rule_h;
  // For this row, anchor all modules at current-row's y
  // Climate Icon is at X_MARGIN
  module_w = ICON_WIDTH;
  module_x = X_MARGIN;
  surface->climate.icon = GRect(module_x, current_row_y, module_w, ICON_HEIGHT);

  // Climate Text Width computed based on TEMP_X_PERCENT
  module_x += (ICON_WIDTH + ICON_TEXT_GAP);
  // width split between climate and date is 35%:65% in favor of date
  module_w = HELPER_SCALE_ROUND(x_end - module_x, TEMP_X_PERCENT, 100);
  surface->climate.text = (WatchfaceTextSubstratum){
      .frame = GRect(module_x, current_row_y, module_w, DATA_TEXT_HEIGHT),
      .alignment = GTextAlignmentLeft,
      .font_role = WATCHFACE_FONT_ROLE_TEXT,
  };

  // Date text
  module_x += module_w;
  module_w = x_end - module_x;
  surface->date = (WatchfaceTextStratum){
      .frame = GRect(module_x, current_row_y, module_w, DATE_TEXT_HEIGHT),
      .alignment = GTextAlignmentCenter,
      .font_role = WATCHFACE_FONT_ROLE_DATE,
      .color_role = WATCHFACE_COLOR_ROLE_DATE,
  };

  // Location text
  current_row_y += (HELPER_MAX(ICON_HEIGHT, DATE_TEXT_HEIGHT) - 2);
  module_w = HELPER_SCALE_ROUND(face_width, LOCATION_TEXT_WIDTH_PERCENT, 100);
  module_x = ((face_width - module_w) >> 1);
  surface->location = (WatchfaceTextStratum){
      .frame = GRect(module_x, current_row_y, module_w, LOCATION_TEXT_HEIGHT),
      .alignment = GTextAlignmentCenter,
      .font_role = WATCHFACE_FONT_ROLE_LOCATION,
  };
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
  surface->battery.is_vertical = is_battery_vertical;

  architect_calculate_must_have_layout(surface);

#ifdef PBL_HEALTH
  architect_calculate_health_layout(surface);
#endif

  return true;
}

// Every component that is re-layed out should be re-framed using
// the Pebble SDK's layer_set_frame(GRect) and a layer_mark_dirty
void relayout_battery_bolt_bticon(
    WatchfaceSurface* surface) {
  if (!surface) {
    return;
  }
  if (surface->battery.is_vertical) {
    (void)place_battery_bar_vertically(surface);
  } else {
    (void)place_battery_bar_horizontally(surface);
  }
}
