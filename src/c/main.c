#include <pebble.h>

#include "ataglance.h"
#include "../modules/helper.h"
#include "../modules/watchface_composer.h"

#define APP_MESSAGE_CONFIG_VALUE_SIZE 2
#define APP_MESSAGE_OUTBOX_SIZE 64

static Window* s_window;
static WatchfaceSettings s_settings = {0};

#if defined(PBL_HEALTH)
static bool s_health_events_subscribed = false;
static void apply_hr_sample_period();
static void health_handler(HealthEventType event, void* context);
#endif

static void refresh_watchface_display();
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
static uint32_t app_message_inbox_size(void);
static AppMessageResult open_app_message(void);
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

static void refresh_watchface_display() {
  if (!s_window) {
    APP_LOG(APP_LOG_LEVEL_ERROR,
            "Cannot refresh watchface display without a window");
    return;
  }

  watchface_composer_refresh();
}

#if defined(PBL_HEALTH)
static void health_handler(HealthEventType event, void* context) {
  (void)context;
  watchface_composer_handle_health_event(event);
}
#endif

static void battery_handler(BatteryChargeState state) {
  watchface_composer_update_battery(&state);
}

static void tick_handler(
    struct tm* tick_time,
    TimeUnits units_changed) {
  (void)tick_time;

  watchface_composer_handle_tick(units_changed);
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
    s_settings.time_format = (uint8_t)time_fmt;
    changed = true;
    refresh_watchface_display();
  } else if (tf) {
    APP_LOG(APP_LOG_LEVEL_WARNING,
            "AppMessage inbox invalid TIME_FORMAT");
  }

  Tuple* tu = dict_find(iter, MESSAGE_KEY_TEMP_UNIT);
  int temp_unit = 0;
  if (tu &&
      helper_tuple_to_int(tu, &temp_unit) &&
      TEMP_UNIT_VALID(temp_unit)) {
    s_settings.temp_unit = (uint8_t)temp_unit;
    changed = true;
    refresh_watchface_display();
  } else if (tu) {
    APP_LOG(APP_LOG_LEVEL_WARNING,
            "AppMessage inbox invalid TEMP_UNIT");
  }

  Tuple* tt = dict_find(iter, MESSAGE_KEY_TEMPERATURE);
  if (tt && tt->type == TUPLE_INT) {
    watchface_composer_update_temp(
        (int)tt->value->int32,
        s_settings.temp_unit);
  } else if (tt) {
    APP_LOG(APP_LOG_LEVEL_WARNING,
            "AppMessage inbox invalid TEMPERATURE");
  }

  Tuple* tw = dict_find(iter, MESSAGE_KEY_WEATHER_CONDITION);
  if (tw && tw->type == TUPLE_INT) {
    watchface_composer_update_weather_condition(
        (int)tw->value->int32,
        s_settings.temp_unit);
  } else if (tw) {
    APP_LOG(APP_LOG_LEVEL_WARNING,
            "AppMessage inbox invalid WEATHER_CONDITION");
  }

  Tuple* th = dict_find(iter, MESSAGE_KEY_HR_SAMPLE_MINUTES);
  int hr_minutes = 0;
  if (th &&
      helper_tuple_to_int(th, &hr_minutes) &&
      HR_SAMPLE_MINUTES_VALID(hr_minutes)) {
    s_settings.hr_sample_minutes = (uint8_t)hr_minutes;
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
        s_settings.display_mode != (uint8_t)display_mode) {
      s_settings.display_mode = (uint8_t)display_mode;
      refresh_watchface_display();
      changed = true;
    } else if (!display_mode_is_valid) {
      APP_LOG(APP_LOG_LEVEL_WARNING,
              "AppMessage inbox invalid DISPLAY_MODE");
    }
  }

  if (changed) {
    settings_save(&s_settings);
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

static uint32_t app_message_inbox_size(void) {
  return dict_calc_buffer_size(
      6,
      APP_MESSAGE_CONFIG_VALUE_SIZE,
      APP_MESSAGE_CONFIG_VALUE_SIZE,
      sizeof(int32_t),
      sizeof(int32_t),
      APP_MESSAGE_CONFIG_VALUE_SIZE,
      APP_MESSAGE_CONFIG_VALUE_SIZE);
}

static AppMessageResult open_app_message(void) {
  uint32_t inbox_size = app_message_inbox_size();
  bool pebblekit_connected =
      connection_service_peek_pebblekit_connection();
  AppMessageResult result = app_message_open(
      inbox_size,
      APP_MESSAGE_OUTBOX_SIZE);
  if (result == APP_MSG_OK) {
    return result;
  }

  APP_LOG(APP_LOG_LEVEL_WARNING,
          "AppMessage open failed: result=%d inbox=%lu",
          result,
          inbox_size);
  APP_LOG(APP_LOG_LEVEL_WARNING,
          "AppMessage sizes: outbox=%d kit=%d",
          APP_MESSAGE_OUTBOX_SIZE,
          pebblekit_connected);

  result = app_message_open(
      APP_MESSAGE_INBOX_SIZE_MINIMUM,
      APP_MESSAGE_OUTBOX_SIZE_MINIMUM);
  if (result != APP_MSG_OK) {
    APP_LOG(APP_LOG_LEVEL_ERROR,
            "AppMessage retry failed: result=%d kit=%d",
            result,
            pebblekit_connected);
  }

  return result;
}

static void main_window_load(Window* window) {
  if (!window) {
    APP_LOG(APP_LOG_LEVEL_ERROR,
            "Main window load received NULL window");
    return;
  }

  if (!watchface_composer_create(window, &s_settings)) {
    APP_LOG(APP_LOG_LEVEL_ERROR,
            "Watchface composer create failed");
    return;
  }

  refresh_watchface_display();
}

static void main_window_unload(Window* window) {
  (void)window;

  watchface_composer_destroy();
}

static void init(void) {
  // Create the window before initializing app state and services.
  #if defined(PBL_HEALTH)
  s_health_events_subscribed = false;
  #endif

  s_window = window_create();
  if (!s_window) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to create main window");
    return;
  }

  if (ATAGLANCE_USE_AND_PERSIST_SETTINGS) {
    settings_load(&s_settings);
  } else {
    settings_apply_defaults(&s_settings);
  }

  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  window_stack_push(s_window, true);

  // Subscribe to services
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
      s_health_events_subscribed = true;
      apply_hr_sample_period();
    }
  #endif

  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  open_app_message();
}

static void deinit(void) {
  if (!s_window) {
    return;
  }
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  app_message_deregister_callbacks();
  #if defined(PBL_HEALTH)
    if (s_health_events_subscribed) {
      health_service_events_unsubscribe();
      s_health_events_subscribed = false;
    }
  #endif
  if (ATAGLANCE_USE_AND_PERSIST_SETTINGS) {
    settings_save(&s_settings);
  }
  window_destroy(s_window);
  s_window = NULL;
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
