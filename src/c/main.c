#include "main.h"
#include "../modules/battery.h"
#include "../modules/date.h"
#include "../modules/helper.h"
#if defined(PBL_HEALTH)
#include "../modules/health.h"
#endif
#include "../modules/time_display.h"
#include "../modules/watchface_composer.h"
#include "../modules/weather.h"

static WatchfaceLayerState s_layers;
static WatchfaceRuntimeState s_runtime = {0};

#if defined(PBL_HEALTH)
static void apply_hr_sample_period();
#endif
static void refresh_watchface_display();
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
    DictionaryIterator* iterator,
    AppMessageResult reason,
    void* context);
static void outbox_sent_callback(
    DictionaryIterator* iterator,
    void* context);
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

static void refresh_watchface_display() {
  if (!s_layers.window) {
    APP_LOG(APP_LOG_LEVEL_ERROR,
            "Cannot refresh watchface display without a window");
    return;
  }

  s_runtime.palette = display_get_palette(
      s_runtime.settings.display_mode);
  watchface_composer_refresh(
      s_layers.window,
      &s_runtime.settings,
      s_runtime.palette);
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
    weather_module_refresh(
        s_runtime.settings.temp_unit,
        s_runtime.palette);
  } else if (tu) {
    APP_LOG(APP_LOG_LEVEL_WARNING,
            "AppMessage inbox invalid TEMP_UNIT");
  }

  Tuple* tt = dict_find(iter, MESSAGE_KEY_TEMPERATURE);
  if (tt && tt->type == TUPLE_INT) {
    weather_module_set_temperature(
        (int16_t)tt->value->int32,
        s_runtime.settings.temp_unit,
        s_runtime.palette);
  } else if (tt) {
    APP_LOG(APP_LOG_LEVEL_WARNING,
            "AppMessage inbox invalid TEMPERATURE");
  }

  Tuple* tw = dict_find(iter, MESSAGE_KEY_WEATHER_CONDITION);
  if (tw && tw->type == TUPLE_INT) {
    weather_module_set_condition(
        (int16_t)tw->value->int32,
        s_runtime.settings.temp_unit,
        s_runtime.palette);
  } else if (tw) {
    APP_LOG(APP_LOG_LEVEL_WARNING,
            "AppMessage inbox invalid WEATHER_CONDITION");
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

static void main_window_load(Window* window) {
  if (!window) {
    APP_LOG(APP_LOG_LEVEL_ERROR,
            "Main window load received NULL window");
    return;
  }

  if (watchface_composer_create(
      window,
      &s_runtime.settings,
      s_runtime.palette)) {
    refresh_watchface_display();
  }
}

static void main_window_unload(Window* window) {
  (void)window;

  watchface_composer_destroy();
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
  s_runtime.palette = display_get_palette(
      s_runtime.settings.display_mode);
  watchface_composer_init();

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
