#include "ataglance.h"

static char s_text_buffers[BUF_TOTAL_COUNT][MAX_STR_LEN];

static GFont s_font_gothic_28;
static GFont s_font_time;
static GFont s_font_gothic_24_bold;

static Window* s_window;
static Layer* s_background_layer;

static TextLayer* s_date_layer;
static TextLayer* s_time_layer;

static Layer* s_bpm_icon_layer;
static GDrawCommandImage* s_bpm_icon_image;
static GPath* s_bpm_icon_fallback_path;

static TextLayer* s_bpm_layer;
static int s_bpm;
static GColor s_bpm_color;

static Layer* s_steps_icon_layer;
static TextLayer* s_steps_layer;

static TextLayer* s_temp_layer;
static Layer* s_battery_icon_layer;
static TextLayer* s_battery_layer;
static BatteryChargeState s_battery_state;

static struct {
  uint8_t temp_unit;
  uint8_t time_format;
  uint8_t hr_sample_minutes;
  uint8_t icon_fallback_mode;
  uint8_t display_mode;
  int16_t temp_celsius_tenths;
} s_settings;

typedef struct {
  GColor background;
  GColor primary_text;
  GColor unavailable_text;
  GColor date;
  GColor time;
  GColor rule;
  GColor steps_icon;
} VisualPalette;

typedef struct {
  GRect background_frame;
  GRect date_frame;
  GRect time_frame;
  GRect bpm_icon_frame;
  GRect bpm_text_frame;
  GRect steps_icon_frame;
  GRect steps_text_frame;
  GRect temp_text_frame;
  GRect battery_icon_frame;
  GRect battery_text_frame;
} WatchfaceLayout;

static const VisualPalette c_dark_palette = {
  .background = GColorBlack,
  .primary_text = GColorLightGray,
  .unavailable_text = GColorWindsorTan,
  .date = GColorRichBrilliantLavender,
  .time = GColorSunsetOrange,
  .rule = GColorLightGray,
  .steps_icon = GColorChromeYellow,
};

static const VisualPalette c_light_palette = {
  .background = GColorWhite,
  .primary_text = GColorBlack,
  .unavailable_text = GColorLightGray,
  .date = GColorImperialPurple,
  .time = GColorSunsetOrange,
  .rule = GColorLightGray,
  .steps_icon = GColorChromeYellow,
};

static const VisualPalette* s_palette = &c_dark_palette;
static const char c_unavailable_text[] = "---";

static inline void select_visual_palette();
static inline bool is_icon_fallback_enabled();
static void load_bpm_icon_assets();
static void unload_bpm_icon_assets();
static void refresh_watchface_display();
static void update_date();
static void update_time();
static void update_bpm();
static void update_steps();
static void update_battery();
static void update_temp();

static inline uint8_t get_hr_sample_minutes(uint8_t hr_sample_minutes) {
  switch ((HrSampleMinutes)hr_sample_minutes) {
    case HR_SAMPLE_MINUTES_10:
      return 10;
    case HR_SAMPLE_MINUTES_15:
      return 15;
    case HR_SAMPLE_MINUTES_30:
      return 30;
    case HR_SAMPLE_MINUTES_60:
      return 60;
    case HR_SAMPLE_MINUTES_120:
      return 120;
    default:
      return 10;
  }
}

static void apply_hr_sample_period() {
  uint8_t minutes = get_hr_sample_minutes(s_settings.hr_sample_minutes);
  uint16_t interval_sec = (uint16_t)minutes * 60;
  health_service_set_heart_rate_sample_period(interval_sec);
}

static void settings_load() {
  s_settings.temp_unit = TEMP_UNIT_DEFAULT;
  s_settings.time_format = TIME_FMT_DEFAULT;
  s_settings.hr_sample_minutes = HR_SAMPLE_MINUTES_DEFAULT;
  s_settings.icon_fallback_mode = ICON_FALLBACK_MODE_DEFAULT;
  s_settings.display_mode = DISPLAY_MODE_DEFAULT;
  s_settings.temp_celsius_tenths = TEMP_INVALID;

  if (persist_exists(PERSIST_SETTINGS)) {
    persist_read_data(PERSIST_SETTINGS, &s_settings, sizeof(s_settings));
  }
  if (!TEMP_UNIT_VALID(s_settings.temp_unit)) {
    s_settings.temp_unit = TEMP_UNIT_DEFAULT;
  }
  if (!TIME_FORMAT_VALID(s_settings.time_format)) {
    s_settings.time_format = TIME_FMT_DEFAULT;
  }
  if (!HR_SAMPLE_MINUTES_VALID(s_settings.hr_sample_minutes)) {
    s_settings.hr_sample_minutes = HR_SAMPLE_MINUTES_DEFAULT;
  }
  if (!ICON_FALLBACK_MODE_VALID(s_settings.icon_fallback_mode)) {
    s_settings.icon_fallback_mode = ICON_FALLBACK_MODE_DEFAULT;
  }
  if (!DISPLAY_MODE_VALID(s_settings.display_mode)) {
    s_settings.display_mode = DISPLAY_MODE_DEFAULT;
  }
  select_visual_palette();
}

static void settings_save() {
  persist_write_data(PERSIST_SETTINGS, &s_settings, sizeof(s_settings));
}

static inline const VisualPalette* get_visual_palette() {
  return (s_settings.display_mode == DISPLAY_MODE_LIGHT) ?
      &c_light_palette : &c_dark_palette;
}

static inline void select_visual_palette() {
  s_palette = get_visual_palette();
}

static char* get_text_buffer(TextBufferId id) {
  switch (id) {
    case BUF_DATE:
    case BUF_TIME:
    case BUF_BPM:
    case BUF_STEPS:
    case BUF_BATTERY:
    case BUF_TEMP:
      return s_text_buffers[id];

    case BUF_CLEANUP: {
        for (size_t i = 0; i < BUF_TOTAL_COUNT; ++i) {
          s_text_buffers[i][0] = '\0';
        }
        return NULL;
    }

    default:
      return NULL;
  }
}

static void uppercase_date(char* buf) {
  if (!buf || strlen(buf) == 0) {
    return;
  }
  int distance = 'a' - 'A';
  for (char* p = buf;* p; ++p) {
    if (*p >= 'a' &&* p <= 'z') {
     * p = (char)(*p - distance);
    }
  }
}

static void format_time(char* buf, size_t buflen, const struct tm* t) {
  if (buf && buflen > 0) {
    if (s_settings.time_format == TIME_FMT_12) {
      strftime(buf, buflen, "%I:%M", t);
      if (buf[0] == '0') {
        memmove(buf, buf + 1, strlen(buf));
      }
    } else {
      strftime(buf, buflen, "%H:%M", t);
    }
  }
}

static void format_temp(char* buf, size_t buflen) {
  if (buf && buflen > 0) {
    int c_tenths = s_settings.temp_celsius_tenths;

    if (c_tenths == TEMP_INVALID) {
      // Temperature can be 3-digits, so we need to print 3 hyphens and the unit
      snprintf(buf, buflen, "%s%s", c_unavailable_text, s_settings.temp_unit == TEMP_UNIT_C ? "°C" : "°F");
      return;
    }

    if (s_settings.temp_unit == TEMP_UNIT_F) {
    // Convert Celsius to Fahrenheit, add half the divisor to round up
      int f_whole = ((c_tenths*  9 + 25) / 50) + 32;
      snprintf(buf, buflen, "%d°F", f_whole);
    } else {
      // Add half the divisor to round up, in this case 10
      int c_whole = (c_tenths >= 0) ? (c_tenths + 5) / 10 : (c_tenths - 5) / 10;
      snprintf(buf, buflen, "%d°C", c_whole);
    }
  }
}

// This is a callback function, so no need to mark the layer as dirty.
// This is being called because the OS knows the layer is dirty!
static void background_update_proc(Layer* layer, GContext* ctx) {
  (void)layer;
  graphics_context_set_stroke_color(ctx, s_palette->rule);
  graphics_context_set_stroke_width(ctx, 2);
  graphics_draw_line(ctx, GPoint(CONTENT_X, RULE_VERT), GPoint(RULE_RIGHT, RULE_VERT));
}

static GColor calculate_bpm_color(int bpm) {
  if (bpm <= 0) {
    // Not Available or Invalid
    return s_palette->unavailable_text;
  } else if (bpm > 120) {
    // Peak Cardiorespiratory Zone
    return GColorRed;
  } else if (bpm >= 100) {
    // Active Cardio/Fat Burn Zone
    return GColorMagenta;
  } else {
    // Healthy Resting Zone
    return GColorJaegerGreen;
  }
}

static bool set_bpm_color_callback(
    GDrawCommand *cmd,
    uint32_t index,
    void *context) {
  GColor *target_color = (GColor *)context;

  (void)index;
  gdraw_command_set_fill_color(cmd, *target_color);
  gdraw_command_set_stroke_color(cmd, *target_color);
  return true;
}

static inline int get_icon_draw_size(GRect bounds) {
  return bounds.size.w < bounds.size.h ? bounds.size.w : bounds.size.h;
}

static inline int scale_icon_coord(GRect bounds, int coord) {
  return (coord * get_icon_draw_size(bounds)) / 28;
}

static inline GPoint scale_icon_point(GRect bounds, int x, int y) {
  int size = get_icon_draw_size(bounds);
  int x_offset = (bounds.size.w - size) / 2;
  int y_offset = (bounds.size.h - size) / 2;

  return GPoint(x_offset + scale_icon_coord(bounds, x),
                y_offset + scale_icon_coord(bounds, y));
}

static void draw_fallback_bpm_icon(
    GContext* ctx,
    GColor color,
    GRect bounds) {
  int radius = scale_icon_coord(bounds, 5);

  if (!ctx) {
    return;
  }

  if (radius < 1) {
    radius = 1;
  }

  graphics_context_set_fill_color(ctx, color);
  graphics_fill_circle(ctx, scale_icon_point(bounds, 9, 10), radius);
  graphics_fill_circle(ctx, scale_icon_point(bounds, 18, 10), radius);

  if (s_bpm_icon_fallback_path) {
    gpath_draw_filled(ctx, s_bpm_icon_fallback_path);
  }
}

static void draw_bpm_icon_with_color(
    GContext* ctx,
    GColor color,
    GRect bounds) {
  bool use_fallback = false;

  if (!ctx) {
    return;
  }

  use_fallback = is_icon_fallback_enabled() || !s_bpm_icon_image;

  if (!use_fallback) {
    GDrawCommandList* list = gdraw_command_image_get_command_list(
        s_bpm_icon_image);
    if (!list) {
      use_fallback = true;
    } else {
      gdraw_command_list_iterate(list, set_bpm_color_callback, &color);
      gdraw_command_image_draw(ctx, s_bpm_icon_image, GPoint(0, 0));
    }
  }

  if (use_fallback) {
    draw_fallback_bpm_icon(ctx, color, bounds);
  }
}

static void unload_bpm_icon_assets() {
  if (s_bpm_icon_image) {
    gdraw_command_image_destroy(s_bpm_icon_image);
    s_bpm_icon_image = NULL;
  }
  if (s_bpm_icon_fallback_path) {
    gpath_destroy(s_bpm_icon_fallback_path);
    s_bpm_icon_fallback_path = NULL;
  }
}

static inline bool is_icon_fallback_enabled() {
  return s_settings.icon_fallback_mode == ICON_FALLBACK_MODE_ENABLED;
}

static void refresh_watchface_display() {
  if (!s_window) {
    APP_LOG(APP_LOG_LEVEL_ERROR,
            "Cannot refresh watchface display without a window");
    return;
  }

  window_set_background_color(s_window, s_palette->background);

  if (s_background_layer) {
    layer_mark_dirty(s_background_layer);
  }
  if (s_steps_icon_layer) {
    layer_mark_dirty(s_steps_icon_layer);
  }

  update_date();
  update_time();
  #if defined(PBL_HEALTH)
  update_steps();
  update_bpm();
  #endif
  s_battery_state = battery_state_service_peek();
  update_battery();
  update_temp();
}

static inline void create_bpm_fallback_icon(GRect bounds) {
  static GPoint s_bpm_icon_fallback_points[] = {
    GPoint(0, 0),
    GPoint(0, 0),
    GPoint(0, 0),
  };
  static GPathInfo s_bpm_icon_fallback_path_info = {
    .num_points = 3,
    .points = s_bpm_icon_fallback_points,
  };

  if (!s_bpm_icon_fallback_path) {
    s_bpm_icon_fallback_points[0] = scale_icon_point(bounds, 4, 13);
    s_bpm_icon_fallback_points[1] = scale_icon_point(bounds, 23, 13);
    s_bpm_icon_fallback_points[2] = scale_icon_point(bounds, 14, 24);
    s_bpm_icon_fallback_path = gpath_create(
        &s_bpm_icon_fallback_path_info);
  }
  if (!s_bpm_icon_fallback_path) {
    APP_LOG(APP_LOG_LEVEL_WARNING,
            "Fallback BPM icon path could not be created");
  }
}

static void load_bpm_icon_assets() {
  bool use_fallback = is_icon_fallback_enabled();
  GRect bounds = GRect(0, 0, 28, 28);

  if (s_bpm_icon_layer) {
    bounds = layer_get_bounds(s_bpm_icon_layer);
  }

  if (!use_fallback && !s_bpm_icon_image) {
    s_bpm_icon_image = gdraw_command_image_create_with_resource(
        RESOURCE_ID_ICON_BPM);

    if (!s_bpm_icon_image) {
      use_fallback = true;
    }
  }

  if (!use_fallback && s_bpm_icon_image) {
    GDrawCommandList* list = gdraw_command_image_get_command_list(
        s_bpm_icon_image);
    if (!list) {
      use_fallback = true;
    }
  }

  if (use_fallback) {
    create_bpm_fallback_icon(bounds);
  }
}

static void bpm_icon_update_proc(Layer* layer, GContext* ctx) {
  if (!layer || !ctx) {
    return;
  }

  draw_bpm_icon_with_color(ctx, s_bpm_color, layer_get_bounds(layer));
}

static void steps_icon_update_proc(Layer* layer, GContext* ctx) {
  (void)layer;
  graphics_context_set_fill_color(ctx, s_palette->steps_icon);
  // --- THE MAIN HEEL PAD (Centered and unified) ---
    // A clean, central oval-like base anchoring the bottom of the 24x24 frame
    graphics_fill_circle(ctx, GPoint(13, 18), 5); // Main pad center
    graphics_fill_circle(ctx, GPoint(10, 19), 4); // Left swell
    graphics_fill_circle(ctx, GPoint(16, 19), 4); // Right swell

    // --- THE 4 SMALL TOE PADS (Arcing cleanly above the heel) ---
    graphics_fill_circle(ctx, GPoint(6, 12), 2);   // Far Left Toe
    graphics_fill_circle(ctx, GPoint(10, 8), 2.5); // Center Left Toe
    graphics_fill_circle(ctx, GPoint(16, 8), 2.5); // Center Right Toe
    graphics_fill_circle(ctx, GPoint(20, 12), 2);  // Far Right Toe
}

static inline GColor get_battery_color_from_state() {
  // Battery health color profile based on percentage remaining
  // Green if charging, CobaltBlue for >50%, Yellow for 20-50%, Red otherwise
  // 1. Fetch live system battery metrics
  int percent = s_battery_state.charge_percent;
  return ((s_battery_state.is_charging) ? GColorJaegerGreen :
    ((percent > 50) ? GColorCobaltBlue : ((percent > 20) ? GColorYellow : GColorRed)));
}

static void battery_icon_update_proc(Layer* layer, GContext* ctx) {
  (void)layer;

  // 1. Fetch live system battery metrics
  int percent = s_battery_state.charge_percent;

  // 2. Determine battery health color profiles
  GColor draw_color = get_battery_color_from_state();

  // 3. Draw the outer AA Battery shell (Centered inside the 25x25 canvas)
  graphics_context_set_stroke_color(ctx, draw_color);
  graphics_context_set_stroke_width(ctx, 2);

  // Main cylinder body shell (Width: 14px, Height: 20px)
  graphics_draw_rect(ctx, GRect(4, 5, 14, 20));

  // The positive (+) contact terminal nub on the very top of the AA battery
  graphics_context_set_fill_color(ctx, draw_color);
  graphics_fill_rect(ctx, GRect(8, 2, 6, 3), 0, GCornerNone);

  // 4. Calculate the vertical fuel cell height from the percentage variable
  // Total interior cylinder height is 16 pixels (from Y = 6 to Y = 22)
  int fill_height = (int) (((percent * 16) / 100) + 1);
  fill_height = (fill_height > 16) ? 16 : fill_height; // Cap the height at 16px max

  // 5. Draw the inner fluid level (Fills upwards from the bottom base plate)
  int fill_y = 23 - fill_height;
  graphics_fill_rect(ctx, GRect(6, fill_y, 10, fill_height), 0, GCornerNone);
}

static void update_date() {
  char* buf = get_text_buffer(BUF_DATE);
  if (!buf || !s_date_layer) {
    return;
  }

  time_t now = time(NULL);
  struct tm* t = localtime(&now);
  strftime(buf, MAX_STR_LEN, "%a · %d %b", t);
  uppercase_date(buf);
  text_layer_set_text_color(s_date_layer, s_palette->date);
  text_layer_set_text(s_date_layer, buf);
}

static void update_time() {
  char* buf = get_text_buffer(BUF_TIME);
  if (!buf || !s_time_layer) {
    return;
  }

  time_t now = time(NULL);
  struct tm* t = localtime(&now);
  format_time(buf, MAX_STR_LEN, t);
  text_layer_set_text_color(s_time_layer, s_palette->time);
  text_layer_set_text(s_time_layer, buf);
}

static void update_bpm() {
  char* bpm_buf = get_text_buffer(BUF_BPM);

  if (!bpm_buf || !s_bpm_layer) {
    return;
  }

  time_t now = time(NULL);
  HealthServiceAccessibilityMask hr_mask = health_service_metric_accessible(
      HealthMetricHeartRateBPM,
      now,
      now);

  if (hr_mask & HealthServiceAccessibilityMaskAvailable) {
    s_bpm = (int) health_service_peek_current_value(HealthMetricHeartRateBPM);

    if (s_bpm > 0) {
      snprintf(bpm_buf, MAX_STR_LEN, "%d", s_bpm);
      s_bpm_color = calculate_bpm_color(s_bpm);
    } else {
      snprintf(bpm_buf, MAX_STR_LEN, "%s", c_unavailable_text);
      s_bpm_color = s_palette->unavailable_text;
    }
  } else {
    snprintf(bpm_buf, MAX_STR_LEN, "%s", c_unavailable_text);
    s_bpm_color = s_palette->unavailable_text;
  }

  text_layer_set_text_color(s_bpm_layer, s_bpm_color);
  text_layer_set_text(s_bpm_layer, bpm_buf);

  if (s_bpm_icon_layer) {
    layer_mark_dirty(s_bpm_icon_layer);
  } else {
    APP_LOG(APP_LOG_LEVEL_WARNING, "BPM icon layer is unavailable");
  }
}

static void update_steps() {
  char* steps_buf = get_text_buffer(BUF_STEPS);
  if (!steps_buf || !s_steps_layer) {
    return;
  }

  GColor text_color = s_palette->unavailable_text;

  HealthServiceAccessibilityMask steps_mask = health_service_metric_accessible(
      HealthMetricStepCount,
      time_start_of_today(),
      time(NULL));

  if (steps_mask & HealthServiceAccessibilityMaskAvailable) {
    HealthValue steps = health_service_sum_today(HealthMetricStepCount);
    if (steps > 0) {
      snprintf(steps_buf, MAX_STR_LEN, "%d", (int)steps);
      text_color = s_palette->primary_text;
    } else {
      snprintf(steps_buf, MAX_STR_LEN, "%s", c_unavailable_text);
    }
  } else {
    snprintf(steps_buf, MAX_STR_LEN, "%s", c_unavailable_text);
  }

  text_layer_set_text_color(s_steps_layer, text_color);
  text_layer_set_text(s_steps_layer, steps_buf);
}

static void update_battery() {
  char* buf = get_text_buffer(BUF_BATTERY);
  if (!buf || !s_battery_layer || !s_battery_icon_layer) {
    return;
  }
  snprintf(buf, MAX_STR_LEN, "%d%%", s_battery_state.charge_percent);

  // Battery text is recolored based on current battery state
  text_layer_set_text_color(s_battery_layer, get_battery_color_from_state());
  text_layer_set_text(s_battery_layer, buf);
  layer_mark_dirty(s_battery_icon_layer);
}

static void update_temp() {
  char* buf = get_text_buffer(BUF_TEMP);
  if (!buf || !s_temp_layer) {
    return;
  }
  format_temp(buf, MAX_STR_LEN);
  text_layer_set_text_color(s_temp_layer, s_palette->primary_text);
  text_layer_set_text(s_temp_layer, buf);
}

static void health_handler(HealthEventType event, void* context) {
  if (event == HealthEventSignificantUpdate || event == HealthEventMovementUpdate) {
    update_steps();
  }
  if (event == HealthEventSignificantUpdate || event == HealthEventHeartRateUpdate) {
    update_bpm();
  }
}

static void battery_handler(BatteryChargeState state) {
  s_battery_state = state;
  update_battery();
}

static void tick_handler(struct tm* tick_time, TimeUnits units_changed) {
  if (units_changed & MINUTE_UNIT) {
    update_time();
  }
  if (units_changed & DAY_UNIT) {
    update_date();
  }
}

static int tuple_to_int(Tuple* tuple) {
  if (!tuple) {
    return -1;
  }
  switch (tuple->type) {
    case TUPLE_INT:
      return (int)tuple->value->int32;
    case TUPLE_CSTRING:
      return atoi(tuple->value->cstring);
    default:
      return -1;
  }
}

static void inbox_received_callback(DictionaryIterator* iter, void* context) {
  // context is unused in this function
  if (!iter) {
    return;
  }
  bool changed = false;

  Tuple* tf = dict_find(iter, MESSAGE_KEY_TIME_FORMAT);
  int time_fmt = tuple_to_int(tf);
  if (TIME_FORMAT_VALID(time_fmt)) {
    s_settings.time_format = (uint8_t)time_fmt;
    changed = true;
    update_time();
  }

  Tuple* tu = dict_find(iter, MESSAGE_KEY_TEMP_UNIT);
  int temp_unit = tuple_to_int(tu);
  if (TEMP_UNIT_VALID(temp_unit)) {
    s_settings.temp_unit = (uint8_t)temp_unit;
    changed = true;
    update_temp();
  }

  Tuple* tt = dict_find(iter, MESSAGE_KEY_TEMPERATURE);
  if (tt && tt->type == TUPLE_INT) {
    s_settings.temp_celsius_tenths = (int16_t)tt->value->int32;
    update_temp();
    changed = true;
  }

  Tuple* th = dict_find(iter, MESSAGE_KEY_HR_SAMPLE_MINUTES);
  int hr_minutes = tuple_to_int(th);
  if (HR_SAMPLE_MINUTES_VALID(hr_minutes)) {
    s_settings.hr_sample_minutes = (uint8_t)hr_minutes;
    apply_hr_sample_period();
    changed = true;
  }

  Tuple* ti = dict_find(iter, MESSAGE_KEY_ICON_FALLBACK_MODE);
  int icon_fallback_mode = tuple_to_int(ti);
  if (ICON_FALLBACK_MODE_VALID(icon_fallback_mode) &&
      s_settings.icon_fallback_mode != (uint8_t)icon_fallback_mode) {
    s_settings.icon_fallback_mode = (uint8_t)icon_fallback_mode;
    if (s_bpm_icon_layer) {
      load_bpm_icon_assets();
      layer_mark_dirty(s_bpm_icon_layer);
    }
    changed = true;
  }

  Tuple* td = dict_find(iter, MESSAGE_KEY_DISPLAY_MODE);
  int display_mode = tuple_to_int(td);
  if (DISPLAY_MODE_VALID(display_mode) &&
      s_settings.display_mode != (uint8_t)display_mode) {
    s_settings.display_mode = (uint8_t)display_mode;
    select_visual_palette();
    refresh_watchface_display();
    changed = true;
  }

  if (changed) {
    settings_save();
  }
}

static void inbox_dropped_callback(AppMessageResult reason, void* context) {
  APP_LOG(APP_LOG_LEVEL_WARNING, "AppMessage dropped: %d", reason);
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  return;
}

static inline TextLayer* create_and_initialize_text_layer(
    Layer* parent,
    GRect frame,
    GColor text_color,
    GColor background_color,
    GFont font,
    GTextAlignment alignment) {
  TextLayer* layer = text_layer_create(frame);
  if (!layer) {
    return NULL;
  }

  text_layer_set_background_color(layer, background_color);
  text_layer_set_text_color(layer, text_color);
  text_layer_set_font(layer, font);
  text_layer_set_text_alignment(layer, alignment);
  layer_add_child(parent, text_layer_get_layer(layer));
  return layer;
}

static WatchfaceLayout get_watchface_layout(GRect bounds) {
  const int content_width = bounds.size.w - (2 * CONTENT_X);

  const int date_top = 10;
  const int date_height = 36;

  const int time_layer_top = 46;
  const int time_layer_height = 50;

  const int metrics_row_top = RULE_VERT + 8;
  const int bottom_row_top = 184;

  const int left_icon_x = CONTENT_X + 8;
  const int left_value_x = 44 + 8;
  const int right_icon_x = 86 + 8;
  const int right_value_x = 118 + 8;

  const int icon_size = 28;
  const int metrics_text_height = 36;
  const int bottom_text_height = 30;

  WatchfaceLayout layout;
  layout.background_frame = GRect(0, 0, 200, 228);
  layout.date_frame = GRect(CONTENT_X,
                            date_top,
                            content_width,
                            date_height);
  layout.time_frame = GRect(CONTENT_X,
                            time_layer_top,
                            content_width,
                            time_layer_height);
  layout.bpm_icon_frame = GRect(left_icon_x,
                                metrics_row_top,
                                icon_size,
                                icon_size);
  layout.bpm_text_frame = GRect(left_value_x,
                                metrics_row_top,
                                58,
                                metrics_text_height);
  layout.steps_icon_frame = GRect(right_icon_x,
                                  metrics_row_top,
                                  icon_size,
                                  icon_size);
  layout.steps_text_frame = GRect(right_value_x,
                                  metrics_row_top,
                                  54,
                                  metrics_text_height);
  layout.temp_text_frame = GRect(44,
                                 bottom_row_top,
                                 64,
                                 bottom_text_height);
  layout.battery_icon_frame = GRect(86,
                                    bottom_row_top,
                                    icon_size,
                                    icon_size);
  layout.battery_text_frame = GRect(118,
                                    bottom_row_top,
                                    54,
                                    bottom_text_height);
  return layout;
}

static inline void init_background_layer(Layer* root, GRect frame) {
  s_background_layer = layer_create(frame);
  if (s_background_layer) {
    layer_set_update_proc(s_background_layer, background_update_proc);
    layer_add_child(root, s_background_layer);
  }
}

static inline void init_top_layers(
    Layer* root,
    const WatchfaceLayout* layout) {
  s_date_layer = create_and_initialize_text_layer(
      root,
      layout->date_frame,
      s_palette->date,
      GColorClear,
      s_font_gothic_24_bold,
      GTextAlignmentLeft);

  s_time_layer = create_and_initialize_text_layer(
      root,
      layout->time_frame,
      s_palette->time,
      GColorClear,
      s_font_time,
      GTextAlignmentLeft);
}

static inline void init_bpm_column(
    Layer* root,
    const WatchfaceLayout* layout) {
  // Initialize BPM releated variables
  s_bpm = 0;
  s_bpm_color = calculate_bpm_color(s_bpm);

  // Create the BPM Icon Layer
  s_bpm_icon_layer = layer_create(layout->bpm_icon_frame);
  if (s_bpm_icon_layer) {
    load_bpm_icon_assets();
    layer_set_update_proc(s_bpm_icon_layer, bpm_icon_update_proc);
    layer_add_child(root, s_bpm_icon_layer);
  }

  s_bpm_layer = create_and_initialize_text_layer(
      root,
      layout->bpm_text_frame,
      s_palette->unavailable_text,
      GColorClear,
      s_font_gothic_28,
      GTextAlignmentLeft);
}

static inline void init_steps_column(
    Layer* root,
    const WatchfaceLayout* layout) {
  s_steps_icon_layer = layer_create(layout->steps_icon_frame);
  if (s_steps_icon_layer) {
    layer_set_update_proc(s_steps_icon_layer, steps_icon_update_proc);
    layer_add_child(root, s_steps_icon_layer);
  }

  s_steps_layer = create_and_initialize_text_layer(
      root,
      layout->steps_text_frame,
      s_palette->unavailable_text,
      GColorClear,
      s_font_gothic_28,
      GTextAlignmentLeft);
}

static inline void init_temp_column(
    Layer* root,
    const WatchfaceLayout* layout) {
  s_temp_layer = create_and_initialize_text_layer(
      root,
      layout->temp_text_frame,
      s_palette->unavailable_text,
      GColorClear,
      s_font_gothic_24_bold,
      GTextAlignmentLeft);
}

static inline void init_battery_column(
    Layer* root,
    const WatchfaceLayout* layout) {
  s_battery_icon_layer = layer_create(layout->battery_icon_frame);
  if (s_battery_icon_layer) {
    layer_set_update_proc(s_battery_icon_layer, battery_icon_update_proc);
    layer_add_child(root, s_battery_icon_layer);
  }

  s_battery_layer = create_and_initialize_text_layer(
      root,
      layout->battery_text_frame,
      s_palette->unavailable_text,
      GColorClear,
      s_font_gothic_24_bold,
      GTextAlignmentLeft);
}

static void main_window_load(Window* window) {
  if (!window) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Main window load received NULL window");
    return;
  }

  Layer* root = window_get_root_layer(window);
  if (!root) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Main window root layer is NULL");
    return;
  }

  GRect bounds = layer_get_bounds(root);
  WatchfaceLayout layout = get_watchface_layout(bounds);

  // Background rule spanning across the content region.
  init_background_layer(root, layout.background_frame);

  // Top row: date and hero time.
  init_top_layers(root, &layout);

  // Middle row: health metrics (BPM and steps).
  init_bpm_column(root, &layout);
  init_steps_column(root, &layout);

  // Bottom row: environment metrics (temperature and battery).
  init_temp_column(root, &layout);
  init_battery_column(root, &layout);

  refresh_watchface_display();
}

static void main_window_unload(Window* window) {
  (void)window;

  if (s_background_layer) {
    layer_destroy(s_background_layer);
    s_background_layer = NULL;
  }

  if (s_date_layer) {
    text_layer_destroy(s_date_layer);
    s_date_layer = NULL;
  }

  if (s_time_layer) {
    text_layer_destroy(s_time_layer);
    s_time_layer = NULL;
  }

  unload_bpm_icon_assets();

  if (s_bpm_icon_layer) {
    layer_destroy(s_bpm_icon_layer);
    s_bpm_icon_layer = NULL;
  }
  if (s_bpm_layer) {
    text_layer_destroy(s_bpm_layer);
    s_bpm_layer = NULL;
  }

  if (s_steps_layer) {
    text_layer_destroy(s_steps_layer);
    s_steps_layer = NULL;
  }
  if (s_steps_icon_layer) {
    layer_destroy(s_steps_icon_layer);
    s_steps_icon_layer = NULL;
  }

  if (s_temp_layer) {
    text_layer_destroy(s_temp_layer);
    s_temp_layer = NULL;
  }

  if (s_battery_icon_layer) {
    layer_destroy(s_battery_icon_layer);
    s_battery_icon_layer = NULL;
  }

  if (s_battery_layer) {
    text_layer_destroy(s_battery_layer);
    s_battery_layer = NULL;
  }

  get_text_buffer(BUF_CLEANUP);
}

static void init(void) {
  // Lifecycle rule: do not initialize app state/services before window creation.
  s_window = window_create();
  if (!s_window) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to create main window");
    return;
  }
  settings_load();

  s_font_gothic_28 = fonts_get_system_font(FONT_KEY_GOTHIC_28);
  s_font_time = fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49);
  s_font_gothic_24_bold = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);

  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  window_stack_push(s_window, true);

  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());

  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  battery_state_service_subscribe(battery_handler);
  #if defined(PBL_HEALTH)
    bool health_available = health_service_events_subscribe(health_handler, NULL);
    if (!health_available) {
      APP_LOG(APP_LOG_LEVEL_ERROR, "Health not available!");
    } else {
      apply_hr_sample_period();
    }
  #else
    APP_LOG(APP_LOG_LEVEL_ERROR, "Health not available!");
  #endif
}

static void deinit(void) {
  if (!s_window) {
    return;
  }
  tick_timer_service_unsubscribe();
  #if defined(PBL_HEALTH)
    health_service_events_unsubscribe();
  #endif
  battery_state_service_unsubscribe();
  settings_save();
  window_destroy(s_window);
  s_window = NULL;
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
