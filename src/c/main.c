#include "ataglance.h"
#include "../modules/weather.h"

static char s_text_buffers[BUF_TOTAL_COUNT][MAX_STR_LEN];

static GFont s_font_date;
static GFont s_font_value;
static GFont s_font_time;

static const int16_t c_temp_invalid = INT16_MIN;
static const int16_t c_icon_size_as_drawn = 28;

static Window* s_window;
static Layer* s_background_layer;

static TextLayer* s_date_layer;
static TextLayer* s_time_layer;

static Layer* s_bpm_icon_layer;
static GDrawCommandImage* s_bpm_icon_image;
static GPath* s_bpm_icon_fallback_path;
static GPoint s_bpm_icon_fallback_points[] = {
  GPoint(0, 0),
  GPoint(0, 0),
  GPoint(0, 0),
};
static GPathInfo s_bpm_icon_fallback_path_info = {
  .num_points = 3,
  .points = s_bpm_icon_fallback_points,
};

static TextLayer* s_bpm_layer;
static int s_bpm;
static GColor s_bpm_color;
static GColor s_bpm_icon_color;
static GColor s_bpm_icon_background_color;

static Layer* s_steps_icon_layer;
static TextLayer* s_steps_layer;
static GColor s_steps_icon_color;
static GColor s_steps_icon_background_color;

static TextLayer* s_temp_layer;
static Layer* s_battery_icon_layer;
static TextLayer* s_battery_layer;
static BatteryChargeState s_battery_state;

static WatchfaceSettings s_settings;
static WatchfaceLayout s_layout;

static const VisualPalette c_dark_palette = {
  .background = GColorBlack,
  .primary_text = PBL_IF_COLOR_ELSE(GColorLightGray, GColorWhite),
  .available_text_background = GColorClear,
  .unavailable_text = PBL_IF_COLOR_ELSE(GColorWindsorTan, GColorBlack),
  .unavailable_text_background = PBL_IF_COLOR_ELSE(
      GColorClear,
      GColorWhite),
  .date = PBL_IF_COLOR_ELSE(GColorRichBrilliantLavender, GColorWhite),
  .time = PBL_IF_COLOR_ELSE(GColorSunsetOrange, GColorWhite),
  .rule = PBL_IF_COLOR_ELSE(GColorLightGray, GColorWhite),
  .steps_icon = PBL_IF_COLOR_ELSE(GColorChromeYellow, GColorWhite),
};

static const VisualPalette c_light_palette = {
  .background = GColorWhite,
  .primary_text = GColorBlack,
  .available_text_background = GColorClear,
  .unavailable_text = PBL_IF_COLOR_ELSE(GColorLightGray, GColorWhite),
  .unavailable_text_background = PBL_IF_COLOR_ELSE(
      GColorClear,
      GColorBlack),
  .date = PBL_IF_COLOR_ELSE(GColorImperialPurple, GColorBlack),
  .time = PBL_IF_COLOR_ELSE(GColorSunsetOrange, GColorBlack),
  .rule = PBL_IF_COLOR_ELSE(GColorLightGray, GColorBlack),
  .steps_icon = PBL_IF_COLOR_ELSE(GColorChromeYellow, GColorBlack),
};

static const VisualPalette* s_palette = &c_dark_palette;
static const char c_unavailable_text[] = "---";

static inline uint8_t get_hr_sample_minutes(uint8_t hr_sample_minutes);
#if defined(PBL_HEALTH)
static void apply_hr_sample_period();
#endif
static void settings_load();
static void settings_save();
static inline void apply_visual_mode_cue();
static inline void update_text_layer_display(
    TextLayer* layer,
    const char* text,
    GColor text_color,
    GColor background_color);
static char* get_text_buffer(TextBufferId id);
static inline void uppercase_date(char* buf);
static void format_time(char* buf, size_t buflen, const struct tm* t);
static bool format_temp(char* buf, size_t buflen);
static void background_update_proc(Layer* layer, GContext* ctx);
static inline GColor calculate_bpm_color(int bpm);
static inline GColor calculate_bpm_icon_color(int bpm);
static bool set_bpm_color_callback(
    GDrawCommand *cmd,
    uint32_t index,
    void *context);
static inline int16_t get_icon_draw_size(const GSize* bounds_size);
static inline int16_t scale_icon_coord(
    const GSize* bounds_size,
    int16_t coord);
static inline int16_t scale_icon_x(
    const GSize* bounds_size,
    int16_t coord);
static inline int16_t scale_icon_y(
    const GSize* bounds_size,
    int16_t coord);
static inline void set_scaled_icon_point_full(
    GPoint* point,
    const GSize* bounds_size,
    int16_t x,
    int16_t y);
static inline void update_bpm_fallback_points(
    const GSize* bounds_size);
static void draw_fallback_bpm_icon(
    GContext* ctx,
    GColor color,
    const GSize* bounds_size);
static void draw_bpm_icon_with_color(
    GContext* ctx,
    GColor color,
    const GSize* bounds_size);
static void unload_bpm_icon_assets();
static inline bool is_icon_fallback_enabled();
static void refresh_watchface_display();
static inline void create_bpm_fallback_icon(
    const GSize* bounds_size);
static void load_bpm_icon_assets();
static void bpm_icon_update_proc(Layer* layer, GContext* ctx);
static void steps_icon_update_proc(Layer* layer, GContext* ctx);
static inline GColor get_battery_color_from_state();
static void battery_icon_update_proc(Layer* layer, GContext* ctx);
static void update_date();
static void update_time();
static void update_battery();
static void update_temp();
#if defined(PBL_HEALTH)
static void update_bpm();
static void update_steps();
static void health_handler(HealthEventType event, void* context);
#endif
static void battery_handler(BatteryChargeState state);
static void tick_handler(
    struct tm* tick_time,
    TimeUnits units_changed);
static int tuple_to_int(Tuple* tuple);
static void inbox_received_callback(
    DictionaryIterator* iter,
    void* context);
static void inbox_dropped_callback(
    AppMessageResult reason,
    void* context);
static void outbox_failed_callback(
    DictionaryIterator *iterator,
    AppMessageResult reason,
    void *context);
static void outbox_sent_callback(
    DictionaryIterator *iterator,
    void *context);
static inline TextLayer* create_and_initialize_text_layer(
    Layer* parent,
    const GRect* frame,
    GColor background_color,
    GFont font,
    GTextAlignment alignment);
static void calculate_watchface_layout(
    int16_t face_width,
    int16_t face_height,
    WatchfaceLayout* layout);
static inline void init_background_layer(
    Layer* root,
    const GRect* frame);
static inline void init_top_layers(
    Layer* root,
    const WatchfaceLayout* layout);
static inline void init_bpm_column(
    Layer* root,
    const WatchfaceLayout* layout);
static inline void init_steps_column(
    Layer* root,
    const WatchfaceLayout* layout);
static inline void init_temp_column(
    Layer* root,
    const WatchfaceLayout* layout);
static inline void init_battery_column(
    Layer* root,
    const WatchfaceLayout* layout);
static void main_window_load(Window* window);
static void main_window_unload(Window* window);
static void init(void);
static void deinit(void);

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

#if defined(PBL_HEALTH)
static void apply_hr_sample_period() {
  uint8_t minutes = get_hr_sample_minutes(s_settings.hr_sample_minutes);
  uint16_t interval_sec = (uint16_t)minutes * 60;
  health_service_set_heart_rate_sample_period(interval_sec);
}
#endif

static void settings_load() {
  s_settings.temp_unit = TEMP_UNIT_DEFAULT;
  s_settings.time_format = TIME_FMT_DEFAULT;
  s_settings.hr_sample_minutes = HR_SAMPLE_MINUTES_DEFAULT;
  s_settings.icon_fallback_mode = ICON_FALLBACK_MODE_DEFAULT;
  s_settings.display_mode = DISPLAY_MODE_DEFAULT;
  s_settings.temp_celsius_tenths = c_temp_invalid;

  if (persist_exists(PERSIST_SETTINGS)) {
    persist_read_data(
        PERSIST_SETTINGS,
        &s_settings,
        sizeof(s_settings));
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
}

static void settings_save() {
  persist_write_data(PERSIST_SETTINGS, &s_settings, sizeof(s_settings));
}

static inline void apply_visual_mode_cue() {
  if (s_settings.display_mode == DISPLAY_MODE_LIGHT) {
    s_palette = &c_light_palette;
  } else {
    s_palette = &c_dark_palette;
  }
}

static inline void update_text_layer_display(
    TextLayer* layer,
    const char* text,
    GColor text_color,
    GColor background_color) {
  text_layer_set_background_color(layer, background_color);
  text_layer_set_text_color(layer, text_color);
  text_layer_set_text(layer, text);
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

static inline void uppercase_date(char* buf) {
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

static bool format_temp(char* buf, size_t buflen) {
  if (!buf || buflen == 0) {
    return false;
  }

  int c_tenths = s_settings.temp_celsius_tenths;

  if (c_tenths == c_temp_invalid) {
    // Temperature can be 3-digits, so print 3 hyphens and the unit
    snprintf(buf, buflen, "%s%s", c_unavailable_text,
             s_settings.temp_unit == TEMP_UNIT_C ? "°C" : "°F");
    return false;
  }

  if (s_settings.temp_unit == TEMP_UNIT_F) {
    // Convert Celsius to Fahrenheit, add half the divisor to round up
    int f_whole = ((c_tenths * 9 + 25) / 50) + 32;
    snprintf(buf, buflen, "%d°F", f_whole);
  } else {
    // Add half the divisor to round up, in this case 10
    int c_whole = (c_tenths >= 0) ?
        (c_tenths + 5) / 10 : (c_tenths - 5) / 10;
    snprintf(buf, buflen, "%d°C", c_whole);
  }

  return true;
}

// This is a callback function, so no need to mark the layer as dirty.
// This is being called because the OS knows the layer is dirty!
static void background_update_proc(Layer* layer, GContext* ctx) {
  (void)layer;
  graphics_context_set_stroke_color(ctx, s_palette->rule);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_line(ctx,
                     GPoint(s_layout.rule_left, s_layout.rule_y),
                     GPoint(s_layout.rule_right, s_layout.rule_y));
}

static inline GColor calculate_bpm_color(int bpm) {
  if (bpm <= 0) {
    // Not Available or Invalid
    return s_palette->unavailable_text;
  } else if (bpm > 120) {
    // Peak Cardiorespiratory Zone
    return PBL_IF_COLOR_ELSE(GColorRed, s_palette->primary_text);
  } else if (bpm >= 100) {
    // Active Cardio/Fat Burn Zone
    return PBL_IF_COLOR_ELSE(GColorMagenta, s_palette->primary_text);
  } else {
    // Healthy Resting Zone
    return PBL_IF_COLOR_ELSE(
        GColorJaegerGreen,
        s_palette->primary_text);
  }
}

static inline GColor calculate_bpm_icon_color(int bpm) {
  if (bpm <= 0) {
    return s_palette->unavailable_text;
  }

  return calculate_bpm_color(bpm);
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

static inline int16_t get_icon_draw_size(const GSize* bounds_size) {
  if (!bounds_size) {
    return 0;
  }

  return bounds_size->w < bounds_size->h ?
      bounds_size->w : bounds_size->h;
}

static inline int16_t scale_icon_coord(
    const GSize* bounds_size,
    int16_t coord) {
  int16_t draw_size = get_icon_draw_size(bounds_size);
  return (coord * draw_size) / c_icon_size_as_drawn;
}

static inline int16_t scale_icon_x(
    const GSize* bounds_size,
    int16_t coord) {
  if (!bounds_size) {
    return 0;
  }

  return (coord * bounds_size->w) / c_icon_size_as_drawn;
}

static inline int16_t scale_icon_y(
    const GSize* bounds_size,
    int16_t coord) {
  if (!bounds_size) {
    return 0;
  }

  return (coord * bounds_size->h) / c_icon_size_as_drawn;
}

static inline void set_scaled_icon_point_full(
    GPoint* point,
    const GSize* bounds_size,
    int16_t x,
    int16_t y) {
  if (!point || !bounds_size) {
    return;
  }

  point->x = scale_icon_x(bounds_size, x);
  point->y = scale_icon_y(bounds_size, y);
}

static inline void update_bpm_fallback_points(
    const GSize* bounds_size) {
  set_scaled_icon_point_full(
      &s_bpm_icon_fallback_points[0],
      bounds_size,
      4,
      13);
  set_scaled_icon_point_full(
      &s_bpm_icon_fallback_points[1],
      bounds_size,
      23,
      13);
  set_scaled_icon_point_full(
      &s_bpm_icon_fallback_points[2],
      bounds_size,
      14,
      24);
}

static void draw_fallback_bpm_icon(
    GContext* ctx,
    GColor color,
    const GSize* bounds_size) {
  int16_t radius = scale_icon_coord(bounds_size, 5);
  GPoint left_lobe;
  GPoint right_lobe;

  if (!ctx || !bounds_size) {
    return;
  }

  set_scaled_icon_point_full(&left_lobe, bounds_size, 9, 10);
  set_scaled_icon_point_full(&right_lobe, bounds_size, 18, 10);

  if (radius < 1) {
    radius = 1;
  }

  graphics_context_set_fill_color(ctx, color);
  graphics_fill_circle(ctx, left_lobe, radius);
  graphics_fill_circle(ctx, right_lobe, radius);

  if (s_bpm_icon_fallback_path) {
    update_bpm_fallback_points(bounds_size);
    gpath_draw_filled(ctx, s_bpm_icon_fallback_path);
  }
}

static void draw_bpm_icon_with_color(
    GContext* ctx,
    GColor color,
    const GSize* bounds_size) {
  bool use_fallback = false;

  if (!ctx || !bounds_size) {
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
    draw_fallback_bpm_icon(ctx, color, bounds_size);
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

  apply_visual_mode_cue();
  window_set_background_color(s_window, s_palette->background);

  if (s_background_layer) {
    layer_mark_dirty(s_background_layer);
  }
  if (s_steps_icon_layer) {
    layer_mark_dirty(s_steps_icon_layer);
  }
  weather_icon_mark_dirty();

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

static inline void create_bpm_fallback_icon(
    const GSize* bounds_size) {
  if (!s_bpm_icon_fallback_path) {
    update_bpm_fallback_points(bounds_size);
    s_bpm_icon_fallback_path = gpath_create(
        &s_bpm_icon_fallback_path_info);
  }
  if (!s_bpm_icon_fallback_path) {
    APP_LOG(APP_LOG_LEVEL_WARNING,
            "Fallback BPM icon path could not be created");
  }
}

static void load_bpm_icon_assets() {
  bool use_fallback = PBL_IF_COLOR_ELSE(
      is_icon_fallback_enabled(),
      true);
  GSize bounds_size = GSize(
      c_icon_size_as_drawn,
      c_icon_size_as_drawn);

  if (s_bpm_icon_layer) {
    GRect bounds = layer_get_bounds(s_bpm_icon_layer);
    bounds_size = bounds.size;
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
    create_bpm_fallback_icon(&bounds_size);
  }
}

static void bpm_icon_update_proc(Layer* layer, GContext* ctx) {
  if (!layer || !ctx) {
    return;
  }

  GRect bounds = layer_get_bounds(layer);

  graphics_context_set_fill_color(ctx, s_bpm_icon_background_color);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  draw_bpm_icon_with_color(
      ctx,
      s_bpm_icon_color,
      &bounds.size);
}

static void steps_icon_update_proc(Layer* layer, GContext* ctx) {
  if (!layer || !ctx) {
    return;
  }

  GRect bounds = layer_get_bounds(layer);

  graphics_context_set_fill_color(ctx, s_steps_icon_background_color);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  graphics_context_set_fill_color(ctx, s_steps_icon_color);

  graphics_fill_circle(ctx, GPoint(13, 18), 5);
  graphics_fill_circle(ctx, GPoint(10, 19), 4);
  graphics_fill_circle(ctx, GPoint(16, 19), 4);

  graphics_fill_circle(ctx, GPoint(6, 12), 2);
  graphics_fill_circle(ctx, GPoint(10, 8), 2.5);
  graphics_fill_circle(ctx, GPoint(16, 8), 2.5);
  graphics_fill_circle(ctx, GPoint(20, 12), 2);
}

static inline GColor get_battery_color_from_state() {
  int percent = s_battery_state.charge_percent;

  if (s_battery_state.is_charging) {
    return PBL_IF_COLOR_ELSE(
        GColorJaegerGreen,
        s_palette->primary_text);
  }

  if (percent > 50) {
    return PBL_IF_COLOR_ELSE(GColorCobaltBlue, s_palette->primary_text);
  }
  if (percent > 20) {
    return PBL_IF_COLOR_ELSE(GColorYellow, s_palette->primary_text);
  }
  return PBL_IF_COLOR_ELSE(GColorRed, s_palette->primary_text);
}

static void battery_icon_update_proc(Layer* layer, GContext* ctx) {
  if (!layer || !ctx) {
    return;
  }

  GRect bounds = layer_get_bounds(layer);
  const int stroke_width = 2;
  int body_w = scale_icon_x(&bounds.size, 28);
  int body_h = scale_icon_y(&bounds.size, 20);
  int body_x = (bounds.size.w - body_w) / 2;
  int body_y = bounds.size.h - body_h - stroke_width;
  int fill_inset_x = scale_icon_x(&bounds.size, 2);
  int fill_inset_y = scale_icon_y(&bounds.size, 2);
  int fill_x = body_x + fill_inset_x;
  int fill_y = body_y + fill_inset_y;
  int max_fill_w = body_w - (2 * fill_inset_x);
  int fill_h = body_h - (2 * fill_inset_y);

  // 1. Fetch live system battery metrics
  int percent = s_battery_state.charge_percent;

  // 2. Determine battery health color profiles
  GColor draw_color = get_battery_color_from_state();

  graphics_context_set_stroke_color(ctx, draw_color);
  graphics_context_set_stroke_width(ctx, stroke_width);

  graphics_draw_rect(ctx, GRect(body_x, body_y, body_w, body_h));

  int fill_w = ((percent * max_fill_w) / 100) + 1;
  if (fill_w > max_fill_w) {
    fill_w = max_fill_w;
  }

  graphics_context_set_fill_color(ctx, draw_color);
  graphics_fill_rect(ctx, GRect(fill_x, fill_y, fill_w, fill_h),
                     0, GCornerNone);
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
  update_text_layer_display(
      s_date_layer,
      buf,
      s_palette->date,
      s_palette->available_text_background);
}

static void update_time() {
  char* buf = get_text_buffer(BUF_TIME);
  if (!buf || !s_time_layer) {
    return;
  }

  time_t now = time(NULL);
  struct tm* t = localtime(&now);
  format_time(buf, MAX_STR_LEN, t);
  update_text_layer_display(
      s_time_layer,
      buf,
      s_palette->time,
      s_palette->available_text_background);
}

#if defined(PBL_HEALTH)
static void update_bpm() {
  char* bpm_buf = get_text_buffer(BUF_BPM);

  if (!bpm_buf || !s_bpm_layer) {
    return;
  }

  time_t now = time(NULL);
  HealthServiceAccessibilityMask hr_mask =
      health_service_metric_accessible(
          HealthMetricHeartRateBPM,
          now,
          now);

  GColor text_background_color = s_palette->unavailable_text_background;
  s_bpm_color = s_palette->unavailable_text;
  s_bpm_icon_color = s_palette->unavailable_text;
  s_bpm_icon_background_color =
      s_palette->unavailable_text_background;

  if (hr_mask & HealthServiceAccessibilityMaskAvailable) {
    s_bpm = (int) health_service_peek_current_value(
        HealthMetricHeartRateBPM);

    if (s_bpm > 0) {
      snprintf(bpm_buf, MAX_STR_LEN, "%d", s_bpm);
      s_bpm_color = calculate_bpm_color(s_bpm);
      s_bpm_icon_color = calculate_bpm_icon_color(s_bpm);
      s_bpm_icon_background_color = s_palette->background;
      text_background_color = s_palette->available_text_background;
    } else {
      snprintf(bpm_buf, MAX_STR_LEN, "%s", c_unavailable_text);
    }
  } else {
    snprintf(bpm_buf, MAX_STR_LEN, "%s", c_unavailable_text);
    s_bpm_color = s_palette->unavailable_text;
    s_bpm_icon_color = calculate_bpm_icon_color(0);
  }

  update_text_layer_display(
      s_bpm_layer,
      bpm_buf,
      s_bpm_color,
      text_background_color);

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
  GColor background_color = s_palette->unavailable_text_background;
  s_steps_icon_color = s_palette->unavailable_text;
  s_steps_icon_background_color =
      s_palette->unavailable_text_background;

  HealthServiceAccessibilityMask steps_mask =
      health_service_metric_accessible(
          HealthMetricStepCount,
          time_start_of_today(),
          time(NULL));

  if (steps_mask & HealthServiceAccessibilityMaskAvailable) {
    HealthValue steps = health_service_sum_today(HealthMetricStepCount);
    if (steps > 0) {
      snprintf(steps_buf, MAX_STR_LEN, "%d", (int)steps);
      text_color = s_palette->primary_text;
      background_color = s_palette->available_text_background;
      s_steps_icon_color = s_palette->steps_icon;
      s_steps_icon_background_color = s_palette->background;
    } else {
      snprintf(steps_buf, MAX_STR_LEN, "%s", c_unavailable_text);
    }
  } else {
    snprintf(steps_buf, MAX_STR_LEN, "%s", c_unavailable_text);
  }

  update_text_layer_display(
      s_steps_layer,
      steps_buf,
      text_color,
      background_color);

  if (s_steps_icon_layer) {
    layer_mark_dirty(s_steps_icon_layer);
  } else {
    APP_LOG(APP_LOG_LEVEL_WARNING, "Steps icon layer is unavailable");
  }
}
#endif

static void update_battery() {
  char* buf = get_text_buffer(BUF_BATTERY);
  if (!buf || !s_battery_layer || !s_battery_icon_layer) {
    return;
  }
  snprintf(buf, MAX_STR_LEN, "%d%%", s_battery_state.charge_percent);

  // Battery text is recolored based on current battery state
  update_text_layer_display(
      s_battery_layer,
      buf,
      get_battery_color_from_state(),
      s_palette->available_text_background);
  layer_mark_dirty(s_battery_icon_layer);
}

static void update_temp() {
  char* buf = get_text_buffer(BUF_TEMP);
  if (!buf || !s_temp_layer) {
    return;
  }
  bool is_temp_available = format_temp(buf, MAX_STR_LEN);
  GColor text_color = is_temp_available ?
      s_palette->primary_text : s_palette->unavailable_text;
  GColor background_color = is_temp_available ?
      s_palette->available_text_background :
      s_palette->unavailable_text_background;

  update_text_layer_display(
      s_temp_layer,
      buf,
      text_color,
      background_color);
  weather_icon_update_display(is_temp_available, s_palette);
}

#if defined(PBL_HEALTH)
static void health_handler(HealthEventType event, void* context) {
  bool significant_update = event == HealthEventSignificantUpdate;

  if (significant_update || event == HealthEventMovementUpdate) {
    update_steps();
  }
  if (significant_update || event == HealthEventHeartRateUpdate) {
    update_bpm();
  }
}
#endif

static void battery_handler(BatteryChargeState state) {
  s_battery_state = state;
  update_battery();
}

static void tick_handler(
    struct tm* tick_time,
    TimeUnits units_changed) {
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

static void inbox_received_callback(
    DictionaryIterator* iter,
    void* context) {
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

  Tuple* tw = dict_find(iter, MESSAGE_KEY_WEATHER_CONDITION);
  if (tw && tw->type == TUPLE_INT) {
    weather_icon_set_condition((int16_t)tw->value->int32);
    weather_icon_update_display(
        s_settings.temp_celsius_tenths != c_temp_invalid,
        s_palette);
  }

  Tuple* th = dict_find(iter, MESSAGE_KEY_HR_SAMPLE_MINUTES);
  int hr_minutes = tuple_to_int(th);
  if (HR_SAMPLE_MINUTES_VALID(hr_minutes)) {
    s_settings.hr_sample_minutes = (uint8_t)hr_minutes;
    #if defined(PBL_HEALTH)
    apply_hr_sample_period();
    #endif
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
    refresh_watchface_display();
    changed = true;
  }

  if (changed) {
    settings_save();
  }
}

static void inbox_dropped_callback(
    AppMessageResult reason,
    void* context) {
  APP_LOG(APP_LOG_LEVEL_WARNING, "AppMessage dropped: %d", reason);
}

static void outbox_failed_callback(
    DictionaryIterator *iterator,
    AppMessageResult reason,
    void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(
    DictionaryIterator *iterator,
    void *context) {
  return;
}

static inline TextLayer* create_and_initialize_text_layer(
    Layer* parent,
    const GRect* frame,
    GColor background_color,
    GFont font,
    GTextAlignment alignment) {
  if (!frame) {
    return NULL;
  }

  TextLayer* layer = text_layer_create(*frame);
  if (!layer) {
    return NULL;
  }

  text_layer_set_background_color(layer, background_color);
  text_layer_set_font(layer, font);
  text_layer_set_text_alignment(layer, alignment);
  layer_add_child(parent, text_layer_get_layer(layer));
  return layer;
}

static void calculate_watchface_layout(
    int16_t face_width,
    int16_t face_height,
    WatchfaceLayout* layout) {
  #if defined(PBL_RECT)
  const int layout_spacing = PBL_DISPLAY_HEIGHT / 28;
  #else
  const int layout_spacing = 8;
  #endif
  const int content_x = layout_spacing;
  const int row_gap = layout_spacing;
  const int column_gap = layout_spacing;
  const int icon_text_gap = 2;
  const int content_width = face_width - (2 * content_x);
  const int rule_y = face_height / 2;
  const int rule_left = content_x;
  const int rule_right = face_width - content_x;

  const int date_height = 20;

  const int time_layer_height = 48;
  const int time_layer_top = rule_y - row_gap - time_layer_height;

  const int date_top = row_gap;
  const int metrics_row_top = rule_y + row_gap;
  const int bottom_text_height = 20;
  const int bottom_row_top =
      face_height - row_gap - bottom_text_height;
  const int icon_size = c_icon_size_as_drawn;
  const int metrics_text_height = 20;
  const int metrics_left_text_width = 28;
  const int metrics_right_text_width = 40;
  const int bottom_row_left_text_width = 34;
  const int bottom_row_right_text_width = 34;
  const int metrics_icon_top =
      metrics_row_top + ((metrics_text_height - icon_size) / 2);
  const int bottom_icon_top =
      bottom_row_top + ((bottom_text_height - icon_size) / 2);

  const int left_icon_x = content_x;
  const int left_value_x = left_icon_x + icon_size + icon_text_gap;
  const int right_value_x =
      face_width - content_x - metrics_right_text_width;
  const int right_icon_x = right_value_x - icon_text_gap - icon_size;
  const int weather_icon_x = content_x;
  const int temperature_value_x =
      weather_icon_x + icon_size + icon_text_gap;
  const int bottom_row_right_value_x =
      face_width - content_x - bottom_row_right_text_width;
  const int bottom_row_right_icon_x =
      bottom_row_right_value_x - icon_text_gap - icon_size;

  layout->background_frame = GRect(0, 0, face_width, face_height);
  layout->rule_left = rule_left;
  layout->rule_y = rule_y;
  layout->rule_right = rule_right;
  layout->content_x = content_x;
  layout->row_gap = row_gap;
  layout->column_gap = column_gap;
  layout->content_width_date = content_width;
  layout->content_width_time = content_width;
  layout->content_width_health = content_width;
  layout->content_width_environment = content_width;
  layout->date_frame = GRect(content_x,
                            date_top,
                            layout->content_width_date,
                            date_height);
  layout->time_frame = GRect(content_x,
                            time_layer_top,
                            layout->content_width_time,
                            time_layer_height);
  layout->bpm_icon_frame = GRect(left_icon_x,
                                metrics_icon_top,
                                icon_size,
                                icon_size);
  layout->bpm_text_frame = GRect(left_value_x,
                                metrics_row_top,
                                metrics_left_text_width,
                                metrics_text_height);
  layout->steps_icon_frame = GRect(right_icon_x,
                                  metrics_icon_top,
                                  icon_size,
                                  icon_size);
  layout->steps_text_frame = GRect(right_value_x,
                                  metrics_row_top,
                                  metrics_right_text_width,
                                  metrics_text_height);
  layout->weather_icon_frame = GRect(weather_icon_x,
                                     bottom_icon_top,
                                     icon_size,
                                     icon_size);
  layout->temp_text_frame = GRect(temperature_value_x,
                                 bottom_row_top,
                                 bottom_row_left_text_width,
                                 bottom_text_height);
  layout->battery_icon_frame = GRect(bottom_row_right_icon_x,
                                    bottom_icon_top,
                                    icon_size,
                                    icon_size);
  layout->battery_text_frame = GRect(bottom_row_right_value_x,
                                    bottom_row_top,
                                    bottom_row_right_text_width,
                                    bottom_text_height);
}

static inline void init_background_layer(
    Layer* root,
    const GRect* frame) {
  if (!frame) {
    return;
  }

  s_background_layer = layer_create(*frame);
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
      &layout->date_frame,
      GColorClear,
      s_font_date,
      GTextAlignmentLeft);

  s_time_layer = create_and_initialize_text_layer(
      root,
      &layout->time_frame,
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
  s_bpm_icon_color = calculate_bpm_icon_color(s_bpm);
  s_bpm_icon_background_color = s_palette->unavailable_text_background;

  // Create the BPM Icon Layer
  s_bpm_icon_layer = layer_create(layout->bpm_icon_frame);
  if (s_bpm_icon_layer) {
    load_bpm_icon_assets();
    layer_set_update_proc(s_bpm_icon_layer, bpm_icon_update_proc);
    layer_add_child(root, s_bpm_icon_layer);
  }

  s_bpm_layer = create_and_initialize_text_layer(
      root,
      &layout->bpm_text_frame,
      GColorClear,
      s_font_value,
      GTextAlignmentLeft);
}

static inline void init_steps_column(
    Layer* root,
    const WatchfaceLayout* layout) {
  s_steps_icon_color = s_palette->unavailable_text;
  s_steps_icon_background_color =
      s_palette->unavailable_text_background;

  s_steps_icon_layer = layer_create(layout->steps_icon_frame);
  if (s_steps_icon_layer) {
    layer_set_update_proc(s_steps_icon_layer, steps_icon_update_proc);
    layer_add_child(root, s_steps_icon_layer);
  }

  s_steps_layer = create_and_initialize_text_layer(
      root,
      &layout->steps_text_frame,
      GColorClear,
      s_font_value,
      GTextAlignmentLeft);
}

static inline void init_temp_column(
    Layer* root,
    const WatchfaceLayout* layout) {
  s_temp_layer = create_and_initialize_text_layer(
      root,
      &layout->temp_text_frame,
      GColorClear,
      s_font_value,
      GTextAlignmentLeft);
}

static inline void init_battery_column(
    Layer* root,
    const WatchfaceLayout* layout) {
  s_battery_icon_layer = layer_create(layout->battery_icon_frame);
  if (s_battery_icon_layer) {
    layer_set_update_proc(
        s_battery_icon_layer,
        battery_icon_update_proc);
    layer_add_child(root, s_battery_icon_layer);
  }

  s_battery_layer = create_and_initialize_text_layer(
      root,
      &layout->battery_text_frame,
      GColorClear,
      s_font_value,
      GTextAlignmentLeft);
}

static void main_window_load(Window* window) {
  if (!window) {
    APP_LOG(APP_LOG_LEVEL_ERROR,
            "Main window load received NULL window");
    return;
  }

  Layer* root = window_get_root_layer(window);
  if (!root) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Main window root layer is NULL");
    return;
  }

  GRect bounds = layer_get_bounds(root);
  calculate_watchface_layout(bounds.size.w, bounds.size.h, &s_layout);

  // Background rule spanning across the content region.
  init_background_layer(
      root,
      &s_layout.background_frame);

  // Top row: date and hero time.
  init_top_layers(root, &s_layout);

  // Middle row: health metrics (BPM and steps).
  init_bpm_column(root, &s_layout);
  init_steps_column(root, &s_layout);

  // Bottom row: environment metrics (temperature and battery).
  weather_icon_create(root, &s_layout.weather_icon_frame, s_palette);
  init_temp_column(root, &s_layout);
  init_battery_column(root, &s_layout);

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

  weather_icon_destroy();

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
  // Create the window before initializing app state and services.
  s_window = window_create();
  if (!s_window) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to create main window");
    return;
  }
  settings_load();
  weather_icon_init();

  s_font_date = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  s_font_value = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  s_font_time = fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD);

  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  window_stack_push(s_window, true);

  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  app_message_open(
      app_message_inbox_size_maximum(),
      app_message_outbox_size_maximum());

  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  battery_state_service_subscribe(battery_handler);
  #if defined(PBL_HEALTH)
    bool health_available = health_service_events_subscribe(
        health_handler,
        NULL);
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
