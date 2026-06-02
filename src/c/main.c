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
  uint8_t fallback_mode;
  int16_t temp_celsius_tenths;
} s_settings;

static const GColor c_color_time = GColorSunsetOrange;
static const GColor c_color_date = GColorRichBrilliantLavender;
static const GColor c_data_unavailable_color = GColorWindsorTan;
static const GColor c_ataglance_text_color = GColorLightGray;
static const char c_unavailable_text[] = "---";

static inline bool is_fallback_mode_enabled();
static void load_bpm_icon_assets();
static void unload_bpm_icon_assets();
static void apply_mode_visual_cue();

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
  s_settings.fallback_mode = FALLBACK_MODE_DEFAULT;
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
  if (!FALLBACK_MODE_VALID(s_settings.fallback_mode)) {
    s_settings.fallback_mode = FALLBACK_MODE_DEFAULT;
  }
}

static void settings_save() {
  persist_write_data(PERSIST_SETTINGS, &s_settings, sizeof(s_settings));
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
  graphics_context_set_stroke_color(ctx, GColorLightGray);
  graphics_context_set_stroke_width(ctx, 2);
  graphics_draw_line(ctx, GPoint(CONTENT_X, RULE_VERT), GPoint(RULE_RIGHT, RULE_VERT));
}

static GColor calculate_bpm_color(int bpm) {
  if (bpm <= 0) {
    // Not Available or Invalid
    return c_data_unavailable_color;
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

static bool set_bpm_color_callback(GDrawCommand *cmd, uint32_t index, void *context) {
  GColor *target_color = (GColor *)context;

  gdraw_command_set_fill_color(cmd, *target_color);
  gdraw_command_set_stroke_color(cmd, *target_color);
  return true;
}

static void draw_fallback_bpm_icon(GContext* ctx, GColor color) {
  if (!ctx) {
    return;
  }

  graphics_context_set_fill_color(ctx, color);
  graphics_fill_circle(ctx, GPoint(9, 10), 5);
  graphics_fill_circle(ctx, GPoint(18, 10), 5);

  if (s_bpm_icon_fallback_path) {
    gpath_draw_filled(ctx, s_bpm_icon_fallback_path);
  }
}

static void draw_bpm_icon_with_color(GContext* ctx, GColor color) {
  static GColor s_prev_bpm_icon_color = GColorBlack;
  static bool s_logged_fallback_draw = false;
  bool use_fallback = false;
  bool bpm_color_updated = false;

  if (!ctx) {
    return;
  }

  use_fallback = is_fallback_mode_enabled() || !s_bpm_icon_image;

  if ((bpm_color_updated = (s_prev_bpm_icon_color.argb != color.argb))) {
    s_prev_bpm_icon_color = color;
  }

  if (!use_fallback) {
    // Use Fallback can only be true if s_bpm_icon_image is NULL
    // So, if we enter here, s_bpm_icon_image is valid
    GDrawCommandList* list = gdraw_command_image_get_command_list(s_bpm_icon_image);
    if (!list) {
      use_fallback = true;
    } else {
      // Draw the icon from the image
      gdraw_command_list_iterate(list, set_bpm_color_callback, &color);
      gdraw_command_image_draw(ctx, s_bpm_icon_image, GPoint(0, 0));
      s_logged_fallback_draw = false;
    }
  }

  // if we get here, use_fallback has to be TRUE
  if (use_fallback) {
    // Log the use of fallback icon
    if (!s_logged_fallback_draw) {
    APP_LOG(APP_LOG_LEVEL_INFO,
            "Fallback icon draw is active (mode=%d, image=%d)",
            (int)s_settings.fallback_mode,
            s_bpm_icon_image ? 1 : 0);
      s_logged_fallback_draw = true;
    }
    // Draw the fallback icon
    draw_fallback_bpm_icon(ctx, color);
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

static inline bool is_fallback_mode_enabled() {
  return s_settings.fallback_mode == FALLBACK_MODE_ENABLED;
}

static void apply_mode_visual_cue() {
  if (!s_window) {
    return;
  }
  if (is_fallback_mode_enabled()) {
    window_set_background_color(s_window, GColorBabyBlueEyes);
  } else {
    window_set_background_color(s_window, GColorDarkGray);
  }
}

static inline void create_bpm_fallback_icon() {
  static GPoint s_bpm_icon_fallback_points[] = {
    GPoint(4, 13),
    GPoint(23, 13),
    GPoint(14, 24),
  };
  static GPathInfo s_bpm_icon_fallback_path_info = {
    .num_points = 3,
    .points = s_bpm_icon_fallback_points,
  };

  if (!s_bpm_icon_fallback_path) {
    s_bpm_icon_fallback_path = gpath_create(&s_bpm_icon_fallback_path_info);
  }
  if (!s_bpm_icon_fallback_path) {
    APP_LOG(APP_LOG_LEVEL_WARNING,
            "Fallback BPM icon path could not be created");
  } else {
    APP_LOG(APP_LOG_LEVEL_INFO,
            "Fallback BPM icon path created or exists");
  }
}

static void load_bpm_icon_assets() {
  bool use_fallback = is_fallback_mode_enabled();

  if (!use_fallback && !s_bpm_icon_image) {
    s_bpm_icon_image = gdraw_command_image_create_with_resource(
        RESOURCE_ID_ICON_BPM);

    if (!s_bpm_icon_image) {
      use_fallback = true;
    }
  }

  if (use_fallback) {
    create_bpm_fallback_icon();
  }
}

// Called whenever this layer is updated. This will be called multiple times
// per second, so try not to do any heavy processing here (like drawing).
static void bpm_icon_update_proc(Layer* layer, GContext* ctx) {
  (void)layer;

  if (!ctx) {
    return;
  }

  draw_bpm_icon_with_color(ctx, s_bpm_color);
}

static void steps_icon_update_proc(Layer* layer, GContext* ctx) {
  (void)layer;
  graphics_context_set_fill_color(ctx, c_color_date);
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
      s_bpm_color = c_data_unavailable_color;
    }
  } else {
    snprintf(bpm_buf, MAX_STR_LEN, "%s", c_unavailable_text);
    s_bpm_color = c_data_unavailable_color;
  }

  text_layer_set_text_color(s_bpm_layer, s_bpm_color);
  text_layer_set_text(s_bpm_layer, bpm_buf);

  if (s_bpm_icon_layer) {
    layer_mark_dirty(s_bpm_icon_layer);
  } else {
    APP_LOG(APP_LOG_LEVEL_WARNING, "BPM icon layer is unavailable");
  }
}

static void update_step_count(){
  char* steps_buf = get_text_buffer(BUF_STEPS);
   if (!steps_buf || !s_steps_layer) {
    return;
   }

  GColor textColor = c_data_unavailable_color;

  HealthServiceAccessibilityMask steps_mask = health_service_metric_accessible(
    HealthMetricStepCount,
    time_start_of_today(),
    time(NULL));

  if (steps_mask & HealthServiceAccessibilityMaskAvailable) {
    HealthValue steps = health_service_sum_today(HealthMetricStepCount);
    if (steps > 0) {
     snprintf(steps_buf, MAX_STR_LEN, "%d", (int)steps);
     textColor = c_ataglance_text_color;
    } else {
     snprintf(steps_buf, MAX_STR_LEN, "%s", c_unavailable_text);
    }
  } else {
    snprintf(steps_buf, MAX_STR_LEN, "%s", c_unavailable_text);
  }

  text_layer_set_text_color(s_steps_layer, textColor);
  text_layer_set_text(s_steps_layer, steps_buf);
}

static void update_battery() {
  char* buf = get_text_buffer(BUF_BATTERY);
  if (!buf || !s_battery_layer || !s_battery_icon_layer) {
    return;
  }
  s_battery_state = battery_state_service_peek();
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
  text_layer_set_text(s_temp_layer, buf);
}

static void update_all() {
  update_date();
  update_time();
  #if defined(PBL_HEALTH)
  update_step_count();
  update_bpm();
  #endif
  s_battery_state = battery_state_service_peek();
  update_battery();
  update_temp();
}

static void health_handler(HealthEventType event, void* context) {
  if (event == HealthEventSignificantUpdate || event == HealthEventMovementUpdate) {
    update_step_count();
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
  APP_LOG(APP_LOG_LEVEL_INFO, "Handler: Update Time / Date");
  if (units_changed & MINUTE_UNIT) {
    update_date();
    update_time();
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

  Tuple* tb = dict_find(iter, MESSAGE_KEY_BPM_ICON_MODE);
  int fallback_mode = tuple_to_int(tb);
  if (FALLBACK_MODE_VALID(fallback_mode) &&
      s_settings.fallback_mode != (uint8_t)fallback_mode) {
    s_settings.fallback_mode = (uint8_t)fallback_mode;
    apply_mode_visual_cue();
    if (s_bpm_icon_layer) {
      load_bpm_icon_assets();
      layer_mark_dirty(s_bpm_icon_layer);
    }
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
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
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

static inline void init_background_layer(Layer* root) {
  s_background_layer = layer_create(GRect(0, 0, 200, 228));
  if (s_background_layer) {
    layer_set_update_proc(s_background_layer, background_update_proc);
    layer_add_child(root, s_background_layer);
  }
}

static inline void init_top_layers(
    Layer* root,
    int content_width,
    int date_top,
    int date_height,
    int time_layer_top,
    int time_layer_height) {
  s_date_layer = create_and_initialize_text_layer(
      root,
      GRect(CONTENT_X, date_top, content_width, date_height),
      c_color_date,
      GColorClear,
      s_font_gothic_24_bold,
      GTextAlignmentLeft);

  s_time_layer = create_and_initialize_text_layer(
      root,
      GRect(CONTENT_X, time_layer_top, content_width, time_layer_height),
      c_color_time,
      GColorClear,
      s_font_time,
      GTextAlignmentLeft);
}

static inline void init_bpm_column(
    Layer* root,
    int left_icon_x,
    int left_value_x,
    int metrics_row_top,
    int icon_size,
    int metrics_text_height) {
  // Initialize BPM releated variables
  s_bpm = 0;
  s_bpm_color = calculate_bpm_color(s_bpm);

  // Create the BPM Icon Layer
  s_bpm_icon_layer = layer_create(
      GRect(left_icon_x, metrics_row_top, icon_size, icon_size));
  if (s_bpm_icon_layer) {
    load_bpm_icon_assets();
    layer_set_update_proc(s_bpm_icon_layer, bpm_icon_update_proc);
    layer_add_child(root, s_bpm_icon_layer);
  }

  s_bpm_layer = create_and_initialize_text_layer(
      root,
      GRect(left_value_x, metrics_row_top, 58, metrics_text_height),
      c_data_unavailable_color,
      GColorClear,
      s_font_gothic_28,
      GTextAlignmentLeft);
}

static inline void init_steps_column(
    Layer* root,
    int right_icon_x,
    int right_value_x,
    int metrics_row_top,
    int icon_size,
    int metrics_text_height) {
  s_steps_icon_layer = layer_create(
      GRect(right_icon_x, metrics_row_top, icon_size, icon_size));
  if (s_steps_icon_layer) {
    layer_set_update_proc(s_steps_icon_layer, steps_icon_update_proc);
    layer_add_child(root, s_steps_icon_layer);
  }

  s_steps_layer = create_and_initialize_text_layer(
      root,
      GRect(right_value_x, metrics_row_top, 54, metrics_text_height),
      c_data_unavailable_color,
      GColorClear,
      s_font_gothic_28,
      GTextAlignmentLeft);
}

static inline void init_temp_layer(
    Layer* root, int bottom_row_top, int bottom_text_height) {
  s_temp_layer = create_and_initialize_text_layer(
      root,
      GRect(44, bottom_row_top, 64, bottom_text_height),
      c_ataglance_text_color,
      GColorClear,
      s_font_gothic_24_bold,
      GTextAlignmentLeft);
}

static inline void init_battery_column(
    Layer* root,
    int bottom_row_top,
    int icon_size,
    int bottom_text_height) {
  s_battery_icon_layer = layer_create(
      GRect(86, bottom_row_top, icon_size, icon_size));
  if (s_battery_icon_layer) {
    layer_set_update_proc(s_battery_icon_layer, battery_icon_update_proc);
    layer_add_child(root, s_battery_icon_layer);
  }

  s_battery_layer = create_and_initialize_text_layer(
      root,
      GRect(118, bottom_row_top, 54, bottom_text_height),
      c_data_unavailable_color,
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

  apply_mode_visual_cue();

  // Background rule spanning across the content region.
  init_background_layer(root);

  // Top row: date and hero time.
  init_top_layers(root,
                  content_width,
                  date_top,
                  date_height,
                  time_layer_top,
                  time_layer_height);

  // Middle row: health metrics (BPM and steps).
  init_bpm_column(root,
                  left_icon_x,
                  left_value_x,
                  metrics_row_top,
                  icon_size,
                  metrics_text_height);
  init_steps_column(root,
                    right_icon_x,
                    right_value_x,
                    metrics_row_top,
                    icon_size,
                    metrics_text_height);

  // Bottom row: environment metrics (temperature and battery).
  init_temp_layer(root, bottom_row_top, bottom_text_height);
  init_battery_column(root, bottom_row_top, icon_size, bottom_text_height);

  update_all();
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
