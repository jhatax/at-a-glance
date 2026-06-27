#include <pebble.h>

#include "../modules/helper.h"
#include "../modules/watchface.h"
// if we are debugging, set the flag in the watchface-runtime

#define APP_MESSAGE_CONFIG_VALUE_SIZE 2
#define APP_MESSAGE_OUTBOX_SIZE 64

static Window *s_window = NULL;
static WatchfaceSettings s_settings = {0};

#ifdef PBL_HEALTH
static bool s_health_events_subscribed = false;
static void apply_hr_sample_period();
static void health_handler(HealthEventType event, void *context);
#endif

static void battery_handler(BatteryChargeState state);
static void tick_handler(struct tm *tick_time, TimeUnits units_changed);
static void inbox_received_callback(DictionaryIterator *iter, void *context);
static void parse_int_tuple(DictionaryIterator *iter, uint32_t key, WatchfaceEventData *data,
                            WatchfaceDataMask mask, int *value);
static void parse_settings_data(DictionaryIterator *iter, WatchfaceEventData *data);
static void parse_weather_data(DictionaryIterator *iter, WatchfaceEventData *data);
#ifdef PBL_HEALTH
static void parse_health_settings_data(DictionaryIterator *iter, WatchfaceEventData *data);
#if ATAGLANCE_DEBUG
static void parse_debug_health_data(DictionaryIterator *iter, WatchfaceEventData *data);
#endif
#endif
static void inbox_dropped_callback(AppMessageResult reason, void *context);
static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason,
                                   void *context);
static void outbox_sent_callback(DictionaryIterator *iterator, void *context);
static uint32_t app_message_inbox_size(void);
static AppMessageResult open_app_message(void);
static void main_window_load(Window *window);
static void main_window_unload(Window *window);
static void init(void);
static void deinit(void);

#ifdef PBL_HEALTH
static void apply_hr_sample_period() {
  uint8_t minutes = settings_get_hr_sample_minutes(s_settings.hr_sample_minutes);
  uint16_t interval_sec = (uint16_t)minutes * 60;
  health_service_set_heart_rate_sample_period(interval_sec);
}
#endif

#ifdef PBL_HEALTH
static void health_handler(HealthEventType event, void *context) {
  (void)context;

  WatchfaceEventData data = {0};
  data.received = WATCHFACE_DATA_HEALTH_EVENT;
  data.parsed = WATCHFACE_DATA_HEALTH_EVENT;
  data.health_event = (int)event;
  watchface_apply_received_data(&data, &s_settings);
}
#endif

static void battery_handler(BatteryChargeState state) {
  (void)state;

  WatchfaceEventData data = {0};
  data.received = WATCHFACE_DATA_BATTERY_EVENT;
  data.parsed = WATCHFACE_DATA_BATTERY_EVENT;
  watchface_apply_received_data(&data, &s_settings);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  (void)tick_time;

  WatchfaceEventData data = {0};
  if (units_changed & MINUTE_UNIT) {
    data.received = (WatchfaceDataMask)(data.received | WATCHFACE_DATA_TIME_TICK);
    data.parsed = (WatchfaceDataMask)(data.parsed | WATCHFACE_DATA_TIME_TICK);
  }
  if (units_changed & DAY_UNIT) {
    data.received = (WatchfaceDataMask)(data.received | WATCHFACE_DATA_DATE_TICK);
    data.parsed = (WatchfaceDataMask)(data.parsed | WATCHFACE_DATA_DATE_TICK);
  }

  data.time_units_changed = (int)units_changed;
  watchface_apply_received_data(&data, &s_settings);
}

static void parse_int_tuple(DictionaryIterator *iter, uint32_t key, WatchfaceEventData *data,
                            WatchfaceDataMask mask, int *value) {
  if (!iter || !data || !value) {
    return;
  }

  Tuple *tuple = dict_find(iter, key);
  if (!tuple) {
    return;
  }

  data->received = (WatchfaceDataMask)(data->received | mask);
  if (helper_tuple_to_int(tuple, value)) {
    data->parsed = (WatchfaceDataMask)(data->parsed | mask);
  } else {
    APP_LOG(APP_LOG_LEVEL_WARNING, "AppMessage inbox invalid int tuple: key=%lu", key);
  }
}

static void parse_settings_data(DictionaryIterator *iter, WatchfaceEventData *data) {
  if (!iter || !data) {
    return;
  }

  parse_int_tuple(iter, MESSAGE_KEY_TIME_FORMAT, data, WATCHFACE_DATA_TIME_FORMAT,
                  &data->time_format);
  parse_int_tuple(iter, MESSAGE_KEY_TEMP_UNIT, data, WATCHFACE_DATA_TEMP_UNIT, &data->temp_unit);
  parse_int_tuple(iter, MESSAGE_KEY_DISPLAY_MODE, data, WATCHFACE_DATA_DISPLAY_MODE,
                  &data->display_mode);
}

static void parse_weather_data(DictionaryIterator *iter, WatchfaceEventData *data) {
  if (!iter || !data) {
    return;
  }

  parse_int_tuple(iter, MESSAGE_KEY_TEMPERATURE, data, WATCHFACE_DATA_TEMPERATURE,
                  &data->temperature_celsius_tenths);
  parse_int_tuple(iter, MESSAGE_KEY_WEATHER_CONDITION, data, WATCHFACE_DATA_WEATHER_CONDITION,
                  &data->weather_condition);
  parse_int_tuple(iter, MESSAGE_KEY_IS_DAY, data, WATCHFACE_DATA_IS_DAY, &data->is_day);
}

#ifdef PBL_HEALTH
static void parse_health_settings_data(DictionaryIterator *iter, WatchfaceEventData *data) {
  if (!iter || !data) {
    return;
  }
  parse_int_tuple(iter, MESSAGE_KEY_HR_SAMPLE_MINUTES, data, WATCHFACE_DATA_HR_SAMPLE_MINUTES,
                  &data->hr_sample_minutes);
  parse_int_tuple(iter, MESSAGE_KEY_STEPS_GOAL, data, WATCHFACE_DATA_STEPS_GOAL,
                  &data->steps_goal);

}

#if ATAGLANCE_DEBUG
static void parse_debug_health_data(DictionaryIterator *iter, WatchfaceEventData *data) {
  if (!iter || !data) {
    return;
  }

  parse_int_tuple(iter, WATCHFACE_DEBUG_MESSAGE_KEY_BPM, data, WATCHFACE_DATA_DEBUG_BPM,
                  &data->debug_bpm);
  parse_int_tuple(iter, WATCHFACE_DEBUG_MESSAGE_KEY_STEPS, data, WATCHFACE_DATA_DEBUG_STEPS,
                  &data->debug_steps);
}
#endif
#endif

static void inbox_received_callback(DictionaryIterator *iter, void *context) {
  (void)context;

  if (!iter) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "AppMessage inbox received NULL iterator");
    return;
  }

  WatchfaceEventData data = {0};
  WatchfaceSettings previous_settings = s_settings;

  parse_settings_data(iter, &data);
  parse_weather_data(iter, &data);

#ifdef PBL_HEALTH
  parse_health_settings_data(iter, &data);
#if ATAGLANCE_DEBUG
  parse_debug_health_data(iter, &data);
#endif
#endif

  watchface_apply_received_data(&data, &s_settings);

  if (previous_settings.time_format != s_settings.time_format ||
      previous_settings.temp_unit != s_settings.temp_unit ||
      previous_settings.hr_sample_minutes != s_settings.hr_sample_minutes ||
      previous_settings.display_mode != s_settings.display_mode ||
      previous_settings.steps_goal != s_settings.steps_goal) {
    settings_save(&s_settings);
  }

#ifdef PBL_HEALTH
  if (previous_settings.hr_sample_minutes != s_settings.hr_sample_minutes) {
    apply_hr_sample_period();
  }
#endif
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  (void)context;
  APP_LOG(APP_LOG_LEVEL_WARNING, "AppMessage inbox dropped: reason=%d", reason);
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason,
                                   void *context) {
  (void)iterator;
  (void)context;
  APP_LOG(APP_LOG_LEVEL_ERROR, "AppMessage outbox failed: reason=%d", reason);
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  (void)iterator;
  (void)context;
}

static uint32_t app_message_inbox_size(void) {
  // the only debug messages we can receive for state are for health
#if defined(PBL_HEALTH) && (ATAGLANCE_DEBUG)
  return dict_calc_buffer_size(10, APP_MESSAGE_CONFIG_VALUE_SIZE, APP_MESSAGE_CONFIG_VALUE_SIZE,
                               sizeof(int32_t), sizeof(int32_t), sizeof(int32_t),
                               APP_MESSAGE_CONFIG_VALUE_SIZE, APP_MESSAGE_CONFIG_VALUE_SIZE,
                               sizeof(int32_t), sizeof(int32_t), sizeof(int32_t));
#else
  return dict_calc_buffer_size(8, APP_MESSAGE_CONFIG_VALUE_SIZE, APP_MESSAGE_CONFIG_VALUE_SIZE,
                               sizeof(int32_t), sizeof(int32_t), sizeof(int32_t),
                               APP_MESSAGE_CONFIG_VALUE_SIZE, APP_MESSAGE_CONFIG_VALUE_SIZE,
                               sizeof(int32_t));
#endif
}

static AppMessageResult open_app_message(void) {
  uint32_t inbox_size = app_message_inbox_size();
  bool pebblekit_connected = connection_service_peek_pebblekit_connection();
  AppMessageResult result = app_message_open(inbox_size, APP_MESSAGE_OUTBOX_SIZE);
  if (result == APP_MSG_OK) {
    return result;
  }

  APP_LOG(APP_LOG_LEVEL_WARNING, "AppMessage open failed: result=%d inbox=%lu", result, inbox_size);
  APP_LOG(APP_LOG_LEVEL_WARNING, "AppMessage sizes: outbox=%d kit=%d", APP_MESSAGE_OUTBOX_SIZE,
          pebblekit_connected);

  result = app_message_open(APP_MESSAGE_INBOX_SIZE_MINIMUM, APP_MESSAGE_OUTBOX_SIZE_MINIMUM);
  if (result != APP_MSG_OK) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "AppMessage retry failed: result=%d kit=%d", result,
            pebblekit_connected);
  }

  return result;
}

static void main_window_load(Window *window) {
  if (!window) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Main window load received NULL window");
    return;
  }

  if (!watchface_create(window, &s_settings)) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Watchface creation failed");
    window_stack_remove(window, true);
    return;
  }
}

static void main_window_unload(Window *window) {
  (void)window;
  watchface_destroy();
}

static void init(void) {
// Create the window before initializing app state and services.
#ifdef PBL_HEALTH
  s_health_events_subscribed = false;
#endif

  s_window = window_create();
  if (!s_window) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to create main window");
    return;
  }

  settings_load(&s_settings);

  window_set_window_handlers(s_window, (WindowHandlers){
                                           .load = main_window_load,
                                           .unload = main_window_unload,
                                       });
  window_stack_push(s_window, true);

  // Subscribe to services
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  battery_state_service_subscribe(battery_handler);
#ifdef PBL_HEALTH
  bool health_available = health_service_events_subscribe(health_handler, NULL);
  if (!health_available) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "Health service subscription failed");
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
#ifdef PBL_HEALTH
  if (s_health_events_subscribed) {
    health_service_events_unsubscribe();
    s_health_events_subscribed = false;
  }
#endif
  settings_save(&s_settings);
  window_destroy(s_window);
  s_window = NULL;
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
