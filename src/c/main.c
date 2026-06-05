#include "main.h"
#include "../modules/helper.h"

static WatchfaceTextState s_text;
static WatchfaceFontState s_fonts;
static WatchfaceLayerState s_layers;
static WatchfaceRuntimeState s_runtime = {0};

#if defined(PBL_HEALTH)
static void apply_hr_sample_period();
#endif
static char* get_text_buffer(TextBufferId id);
static bool format_temp(char* buf, size_t buflen);
static void refresh_watchface_display();
static void update_temp();
#if defined(PBL_HEALTH)
static void health_handler(HealthEventType event, void* context);
#endif
static void battery_handler(BatteryChargeState state);
static void tick_handler(
    struct tm* tick_time,
    TimeUnits units_changed);
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
      s_runtime.settings.hr_sample_minutes);
  uint16_t interval_sec = (uint16_t)minutes * 60;
  health_service_set_heart_rate_sample_period(interval_sec);
}
#endif

static char* get_text_buffer(TextBufferId id) {
  switch (id) {
    case BUF_TEMP:
      return s_text.buffers[id];

    case BUF_CLEANUP: {
        for (size_t i = 0; i < BUF_TOTAL_COUNT; ++i) {
          s_text.buffers[i][0] = '\0';
        }
        return NULL;
    }

    default:
      return NULL;
  }
}

static bool format_temp(char* buf, size_t buflen) {
  if (!buf || buflen == 0) {
    return false;
  }

  int c_tenths = s_runtime.temp_celsius_tenths;

  if (c_tenths == WEATHER_TEMP_INVALID) {
    // Temperature can be 3-digits, so print 3 hyphens and the unit
    const char* temp_unit =
        s_runtime.settings.temp_unit == TEMP_UNIT_C ? "°C" : "°F";
    snprintf(
        buf,
        buflen,
        "%s%s",
        DISPLAY_UNAVAILABLE_TEXT,
        temp_unit
    );
    return false;
  }

  if (s_runtime.settings.temp_unit == TEMP_UNIT_F) {
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

static void refresh_watchface_display() {
  if (!s_layers.window) {
    APP_LOG(APP_LOG_LEVEL_ERROR,
            "Cannot refresh watchface display without a window");
    return;
  }

  s_runtime.palette = display_get_palette(
      s_runtime.settings.display_mode);
  window_set_background_color(
      s_layers.window,
      s_runtime.palette->background);

  #if defined(PBL_HEALTH)
  health_module_refresh(s_runtime.palette);
  #endif
  weather_icon_mark_dirty();

  date_module_refresh(s_runtime.palette);
  time_display_module_refresh(
      s_runtime.settings.time_format,
      s_runtime.palette);
  battery_module_refresh(s_runtime.palette);
  update_temp();
}

static void update_temp() {
  char* buf = get_text_buffer(BUF_TEMP);
  if (!buf || !s_layers.temp_layer) {
    return;
  }
  bool is_temp_available = format_temp(buf, ATAGLANCE_MAX_STR_LEN);
  GColor text_color = is_temp_available ?
      s_runtime.palette->primary_text :
      s_runtime.palette->unavailable_text;

  display_update_text_layer(
      s_layers.temp_layer,
      buf,
      text_color);
  weather_icon_update_display(is_temp_available, s_runtime.palette);
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
  (void)tick_time;

  if (units_changed & MINUTE_UNIT) {
    time_display_module_refresh(
        s_runtime.settings.time_format,
        s_runtime.palette);
  }
  if (units_changed & DAY_UNIT) {
    date_module_refresh(s_runtime.palette);
  }
}

static void inbox_received_callback(
    DictionaryIterator* iter,
    void* context) {
  (void)context;

  if (!iter) {
    APP_LOG(APP_LOG_LEVEL_WARNING,
            "AppMessage inbox received NULL iterator");
    return;
  }
  bool changed = false;

  Tuple* tf = dict_find(iter, MESSAGE_KEY_TIME_FORMAT);
  int time_fmt = 0;
  if (tf &&
      helper_tuple_to_int(tf, &time_fmt) &&
      TIME_FORMAT_VALID(time_fmt)) {
    s_runtime.settings.time_format = (uint8_t)time_fmt;
    changed = true;
    time_display_module_refresh(
        s_runtime.settings.time_format,
        s_runtime.palette);
  } else if (tf) {
    APP_LOG(APP_LOG_LEVEL_WARNING,
            "AppMessage inbox invalid TIME_FORMAT");
  }

  Tuple* tu = dict_find(iter, MESSAGE_KEY_TEMP_UNIT);
  int temp_unit = 0;
  if (tu &&
      helper_tuple_to_int(tu, &temp_unit) &&
      TEMP_UNIT_VALID(temp_unit)) {
    s_runtime.settings.temp_unit = (uint8_t)temp_unit;
    changed = true;
    update_temp();
  } else if (tu) {
    APP_LOG(APP_LOG_LEVEL_WARNING,
            "AppMessage inbox invalid TEMP_UNIT");
  }

  Tuple* tt = dict_find(iter, MESSAGE_KEY_TEMPERATURE);
  bool weather_changed = false;
  if (tt && tt->type == TUPLE_INT) {
    s_runtime.temp_celsius_tenths = (int16_t)tt->value->int32;
    weather_changed = true;
  } else if (tt) {
    APP_LOG(APP_LOG_LEVEL_WARNING,
            "AppMessage inbox invalid TEMPERATURE");
  }

  Tuple* tw = dict_find(iter, MESSAGE_KEY_WEATHER_CONDITION);
  if (tw && tw->type == TUPLE_INT) {
    weather_icon_set_condition((int16_t)tw->value->int32);
    weather_changed = true;
  } else if (tw) {
    APP_LOG(APP_LOG_LEVEL_WARNING,
            "AppMessage inbox invalid WEATHER_CONDITION");
  }

  if (weather_changed) {
    update_temp();
  }

  Tuple* th = dict_find(iter, MESSAGE_KEY_HR_SAMPLE_MINUTES);
  int hr_minutes = 0;
  if (th &&
      helper_tuple_to_int(th, &hr_minutes) &&
      HR_SAMPLE_MINUTES_VALID(hr_minutes)) {
    s_runtime.settings.hr_sample_minutes = (uint8_t)hr_minutes;
    #if defined(PBL_HEALTH)
    apply_hr_sample_period();
    #endif
    changed = true;
  } else if (th) {
    APP_LOG(APP_LOG_LEVEL_WARNING,
            "AppMessage inbox invalid HR_SAMPLE_MINUTES");
  }

  Tuple* td = dict_find(iter, MESSAGE_KEY_DISPLAY_MODE);
  int display_mode = 0;
  if (td) {
    bool display_mode_is_valid =
        helper_tuple_to_int(td, &display_mode) &&
        DISPLAY_MODE_VALID(display_mode);
    if (display_mode_is_valid &&
        s_runtime.settings.display_mode != (uint8_t)display_mode) {
      s_runtime.settings.display_mode = (uint8_t)display_mode;
      refresh_watchface_display();
      changed = true;
    } else if (!display_mode_is_valid) {
      APP_LOG(APP_LOG_LEVEL_WARNING,
              "AppMessage inbox invalid DISPLAY_MODE");
    }
  }

  if (changed) {
    settings_save(&s_runtime.settings);
  }
}

static void inbox_dropped_callback(
    AppMessageResult reason,
    void* context) {
  (void)context;
  APP_LOG(APP_LOG_LEVEL_WARNING,
          "AppMessage inbox dropped: reason=%d",
          reason);
}

static void outbox_failed_callback(
    DictionaryIterator* iterator,
    AppMessageResult reason,
    void* context) {
  (void)iterator;
  (void)context;
  APP_LOG(APP_LOG_LEVEL_ERROR,
          "AppMessage outbox failed: reason=%d",
          reason);
}

static void outbox_sent_callback(
    DictionaryIterator* iterator,
    void* context) {
  (void)iterator;
  (void)context;
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

static inline void init_temp_column(
    Layer* root,
    const WatchfaceLayout* layout) {
  s_layers.temp_layer = create_and_initialize_text_layer(
      root,
      &layout->temp_text_frame,
      GColorClear,
      s_fonts.secondary_value,
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
  layout_calculate(bounds.size.w, bounds.size.h, &s_runtime.layout);

  // Top row: date and hero time.
  date_module_create(
      root,
      &s_runtime.layout.date_frame,
      s_runtime.palette);
  time_display_module_create(
      root,
      &s_runtime.layout.time_frame,
      s_runtime.palette);

  // Middle row: health metrics (BPM and steps).
  #if defined(PBL_HEALTH)
  health_module_create(
      root,
      &s_runtime.layout,
      s_fonts.secondary_value,
      s_runtime.palette);
  #endif

  // Bottom row: temperature and battery.
  weather_icon_create(
      root,
      &s_runtime.layout.weather_icon_frame,
      s_runtime.palette);
  init_temp_column(root, &s_runtime.layout);
  battery_module_create(
      root,
      &s_runtime.layout,
      s_fonts.battery_value,
      s_runtime.palette);

  refresh_watchface_display();
}

static void main_window_unload(Window* window) {
  (void)window;

  date_module_destroy();
  time_display_module_destroy();

  #if defined(PBL_HEALTH)
  health_module_destroy();
  #endif

  if (s_layers.temp_layer) {
    text_layer_destroy(s_layers.temp_layer);
    s_layers.temp_layer = NULL;
  }

  weather_icon_destroy();

  battery_module_destroy();

  get_text_buffer(BUF_CLEANUP);
}

static void init(void) {
  // Create the window before initializing app state and services.
  s_layers.window = window_create();
  if (!s_layers.window) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to create main window");
    return;
  }

  if (ATAGLANCE_USE_AND_PERSIST_SETTINGS) {
    settings_load(&s_runtime.settings);
  } else {
    settings_apply_defaults(&s_runtime.settings);
  }
  s_runtime.temp_celsius_tenths = WEATHER_TEMP_INVALID;
  s_runtime.palette = display_get_palette(
      s_runtime.settings.display_mode);
  weather_icon_init();

  s_fonts.secondary_value = fonts_get_system_font(FONT_KEY_GOTHIC_18);
  s_fonts.battery_value = fonts_get_system_font(
      FONT_KEY_GOTHIC_18_BOLD);

  window_set_window_handlers(s_layers.window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  window_stack_push(s_layers.window, true);

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
      APP_LOG(APP_LOG_LEVEL_WARNING,
              "Health service subscription failed");
    } else {
      apply_hr_sample_period();
    }
  #endif
}

static void deinit(void) {
  if (!s_layers.window) {
    return;
  }
  tick_timer_service_unsubscribe();
  #if defined(PBL_HEALTH)
    health_service_events_unsubscribe();
  #endif
  battery_state_service_unsubscribe();
  if (ATAGLANCE_USE_AND_PERSIST_SETTINGS) {
    settings_save(&s_runtime.settings);
  }
  window_destroy(s_layers.window);
  s_layers.window = NULL;
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
