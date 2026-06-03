#include "ataglance.h"
#include "../modules/battery.h"
#include "../modules/display.h"
#if defined(PBL_HEALTH)
#include "../modules/health.h"
#endif
#include "../modules/layout.h"
#include "../modules/settings.h"
#include "../modules/weather.h"

static char s_text_buffers[BUF_TOTAL_COUNT][MAX_STR_LEN];

static GFont s_font_date;
static GFont s_font_secondary_value;
static GFont s_font_battery_value;
static GFont s_font_time;

static Window* s_window;
static Layer* s_background_layer;

static TextLayer* s_date_layer;
static TextLayer* s_time_layer;

static TextLayer* s_temp_layer;

static WatchfaceSettings s_settings;
static WatchfaceLayout s_layout;

static const VisualPalette* s_palette;

#if defined(PBL_HEALTH)
static void apply_hr_sample_period();
#endif
static char* get_text_buffer(TextBufferId id);
static inline void uppercase_date(char* buf);
static void format_time(char* buf, size_t buflen, const struct tm* t);
static bool format_temp(char* buf, size_t buflen);
static void background_update_proc(Layer* layer, GContext* ctx);
static void refresh_watchface_display();
static void update_date();
static void update_time();
static void update_temp();
#if defined(PBL_HEALTH)
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
static inline void init_background_layer(
    Layer* root,
    const GRect* frame);
static inline void init_top_layers(
    Layer* root,
    const WatchfaceLayout* layout);
static inline void init_temp_column(
    Layer* root,
    const WatchfaceLayout* layout);
static void main_window_load(Window* window);
static void main_window_unload(Window* window);
static void init(void);
static void deinit(void);

#if defined(PBL_HEALTH)
static void apply_hr_sample_period() {
  uint8_t minutes = settings_get_hr_sample_minutes(
      s_settings.hr_sample_minutes);
  uint16_t interval_sec = (uint16_t)minutes * 60;
  health_service_set_heart_rate_sample_period(interval_sec);
}
#endif

static char* get_text_buffer(TextBufferId id) {
  switch (id) {
    case BUF_DATE:
    case BUF_TIME:
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

  if (c_tenths == SETTINGS_TEMP_INVALID) {
    // Temperature can be 3-digits, so print 3 hyphens and the unit
    snprintf(buf, buflen, "%s%s", display_unavailable_text(),
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

static void refresh_watchface_display() {
  if (!s_window) {
    APP_LOG(APP_LOG_LEVEL_ERROR,
            "Cannot refresh watchface display without a window");
    return;
  }

  s_palette = display_get_palette(s_settings.display_mode);
  window_set_background_color(s_window, s_palette->background);

  if (s_background_layer) {
    layer_mark_dirty(s_background_layer);
  }
  #if defined(PBL_HEALTH)
  health_module_refresh(s_palette);
  #endif
  weather_icon_mark_dirty();

  update_date();
  update_time();
  battery_module_refresh(s_palette);
  update_temp();
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
  display_update_text_layer(
      s_date_layer,
      buf,
      s_palette->date);
}

static void update_time() {
  char* buf = get_text_buffer(BUF_TIME);
  if (!buf || !s_time_layer) {
    return;
  }

  time_t now = time(NULL);
  struct tm* t = localtime(&now);
  format_time(buf, MAX_STR_LEN, t);
  display_update_text_layer(
      s_time_layer,
      buf,
      s_palette->time);
}

static void update_temp() {
  char* buf = get_text_buffer(BUF_TEMP);
  if (!buf || !s_temp_layer) {
    return;
  }
  bool is_temp_available = format_temp(buf, MAX_STR_LEN);
  GColor text_color = is_temp_available ?
      s_palette->primary_text : s_palette->unavailable_text;

  display_update_text_layer(
      s_temp_layer,
      buf,
      text_color);
  weather_icon_update_display(is_temp_available, s_palette);
}

#if defined(PBL_HEALTH)
static void health_handler(HealthEventType event, void* context) {
  (void)context;
  health_module_handle_event(event);
}
#endif

static void battery_handler(BatteryChargeState state) {
  battery_module_set_state(&state);
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
        s_settings.temp_celsius_tenths != SETTINGS_TEMP_INVALID,
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

  Tuple* td = dict_find(iter, MESSAGE_KEY_DISPLAY_MODE);
  int display_mode = tuple_to_int(td);
  if (DISPLAY_MODE_VALID(display_mode) &&
      s_settings.display_mode != (uint8_t)display_mode) {
    s_settings.display_mode = (uint8_t)display_mode;
    refresh_watchface_display();
    changed = true;
  }

  if (changed) {
    settings_save(&s_settings);
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

static inline void init_temp_column(
    Layer* root,
    const WatchfaceLayout* layout) {
  s_temp_layer = create_and_initialize_text_layer(
      root,
      &layout->temp_text_frame,
      GColorClear,
      s_font_secondary_value,
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
  layout_calculate(bounds.size.w, bounds.size.h, &s_layout);

  // Background rule spanning across the content region.
  init_background_layer(
      root,
      &s_layout.background_frame);

  // Top row: date and hero time.
  init_top_layers(root, &s_layout);

  // Middle row: health metrics (BPM and steps).
  #if defined(PBL_HEALTH)
  health_module_create(
      root,
      &s_layout,
      s_font_secondary_value,
      s_palette);
  #endif

  // Bottom row: temperature and battery.
  weather_icon_create(root, &s_layout.weather_icon_frame, s_palette);
  init_temp_column(root, &s_layout);
  battery_module_create(
      root,
      &s_layout,
      s_font_battery_value,
      s_palette);

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

  #if defined(PBL_HEALTH)
  health_module_destroy();
  #endif

  if (s_temp_layer) {
    text_layer_destroy(s_temp_layer);
    s_temp_layer = NULL;
  }

  weather_icon_destroy();

  battery_module_destroy();

  get_text_buffer(BUF_CLEANUP);
}

static void init(void) {
  // Create the window before initializing app state and services.
  s_window = window_create();
  if (!s_window) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to create main window");
    return;
  }
  settings_load(&s_settings);
  s_palette = display_get_palette(s_settings.display_mode);
  weather_icon_init();

  s_font_date = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  s_font_secondary_value = fonts_get_system_font(FONT_KEY_GOTHIC_18);
  s_font_battery_value = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
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
  settings_save(&s_settings);
  window_destroy(s_window);
  s_window = NULL;
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
