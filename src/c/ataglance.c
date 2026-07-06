#include <pebble.h>

#include "../modules/helper.h"
#include "../modules/watchface.h"
// if we are debugging, set the flag in the watchface-runtime

#define APP_MESSAGE_CONFIG_VALUE_SIZE 2
#define APP_MESSAGE_OUTBOX_SIZE 64

static Window* s_window = NULL;
static WatchfaceSettings s_settings = {0};

#ifdef PBL_HEALTH
static bool s_health_events_subscribed = false;
static void health_handler(HealthEventType event, void* context);
static void parse_health_settings_data(DictionaryIterator* iter, WatchfaceEventData* data);
static void parse_oneshot_health_data(DictionaryIterator* iter, WatchfaceEventData* data);
#endif

static void battery_handler(BatteryChargeState state);
static void tick_handler(struct tm* tick_time, TimeUnits units_changed);
static void inbox_received_callback(DictionaryIterator* iter, void* context);
static void parse_int_tuple(DictionaryIterator* iter,
  uint32_t key,
  WatchfaceEventData* data,
  WatchfaceDataMask mask,
  int* value);
static void parse_settings_data(DictionaryIterator* iter, WatchfaceEventData* data);
static void parse_weather_data(DictionaryIterator* iter, WatchfaceEventData* data);
static void inbox_dropped_callback(AppMessageResult reason, void* context);
static void outbox_failed_callback(DictionaryIterator* iterator,
  AppMessageResult reason,
  void* context);
static void outbox_sent_callback(DictionaryIterator* iterator, void* context);
static uint32_t app_message_inbox_size(void);
static AppMessageResult initialize_inbox_outbox(void);
static void send_loaded_weather_update_minutes(void);
static void main_window_load(Window* window);
static void main_window_unload(Window* window);
static void init(void);
static void deinit(void);

static void battery_handler(
  BatteryChargeState state) {
  (void)state;

  WatchfaceEventData data = {0};
  // It is sufficient to set these values because they trigger a battery refresh
  data.received = WATCHFACE_DATA_BATTERY_EVENT;
  data.parsed = WATCHFACE_DATA_BATTERY_EVENT;
  watchface_apply_received_data(&data, &s_settings);
}

static void tick_handler(
  struct tm* tick_time,
  TimeUnits units_changed) {
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

  watchface_apply_received_data(&data, &s_settings);
}

static void parse_int_tuple(
  DictionaryIterator* iter,
  uint32_t key,
  WatchfaceEventData* data,
  WatchfaceDataMask mask,
  int* value) {
  if (!iter || !data || !value) {
    return;
  }

  Tuple* tuple = dict_find(iter, key);
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

static void parse_settings_data(
  DictionaryIterator* iter,
  WatchfaceEventData* data) {
  if (!iter || !data) {
    return;
  }

  parse_int_tuple(iter,
    MESSAGE_KEY_TIME_FORMAT,
    data,
    WATCHFACE_DATA_TIME_FORMAT,
    &data->time_format);
  parse_int_tuple(iter, MESSAGE_KEY_TEMP_UNIT, data, WATCHFACE_DATA_TEMP_UNIT, &data->temp_unit);
  parse_int_tuple(iter,
    MESSAGE_KEY_DISPLAY_MODE,
    data,
    WATCHFACE_DATA_DISPLAY_MODE,
    &data->display_mode);
  parse_int_tuple(iter,
    MESSAGE_KEY_WEATHER_UPDATE_MINUTES,
    data,
    WATCHFACE_DATA_WEATHER_UPDATE_MINUTES,
    &data->weather_update_minutes);
}

static void parse_weather_data(
  DictionaryIterator* iter,
  WatchfaceEventData* data) {
  if (!iter || !data) {
    return;
  }

  parse_int_tuple(iter,
    MESSAGE_KEY_TEMPERATURE,
    data,
    WATCHFACE_DATA_TEMPERATURE,
    &data->temperature_celsius_tenths);
  parse_int_tuple(iter,
    MESSAGE_KEY_WEATHER_CONDITION,
    data,
    WATCHFACE_DATA_WEATHER_CONDITION,
    &data->weather_condition);
  parse_int_tuple(iter, MESSAGE_KEY_IS_DAY, data, WATCHFACE_DATA_IS_DAY, &data->is_day);
}

#ifdef PBL_HEALTH
static void health_handler(
  HealthEventType event,
  void* context) {
  (void)context;
  // We ignore the event because we use any health update to update all health items
  (void)event;

  WatchfaceEventData data = {0};
  // It is sufficient to set these values because they trigger a health refresh
  data.received = WATCHFACE_DATA_HEALTH_EVENT;
  data.parsed = WATCHFACE_DATA_HEALTH_EVENT;
  watchface_apply_received_data(&data, &s_settings);
}

static void parse_health_settings_data(
  DictionaryIterator* iter,
  WatchfaceEventData* data) {
  if (!iter || !data) {
    return;
  }
  parse_int_tuple(iter,
    MESSAGE_KEY_HR_SAMPLE_MINUTES,
    data,
    WATCHFACE_DATA_HR_SAMPLE_MINUTES,
    &data->hr_sample_minutes);
  parse_int_tuple(iter, MESSAGE_KEY_STEPS_GOAL, data, WATCHFACE_DATA_STEPS_GOAL, &data->steps_goal);
}

static void parse_oneshot_health_data(
  DictionaryIterator* iter,
  WatchfaceEventData* data) {
  if (!iter || !data) {
    return;
  }

  parse_int_tuple(iter,
    WATCHFACE_ONESHOT_MESSAGE_KEY_BPM,
    data,
    WATCHFACE_DATA_ONESHOT_BPM,
    &data->oneshot_bpm);
  parse_int_tuple(iter,
    WATCHFACE_ONESHOT_MESSAGE_KEY_STEPS,
    data,
    WATCHFACE_DATA_ONESHOT_STEPS,
    &data->oneshot_steps);
}
#endif

static void inbox_received_callback(
  DictionaryIterator* iter,
  void* context) {
  (void)context;

  if (!iter) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "AppMessage inbox received NULL iterator");
    return;
  }

  WatchfaceEventData data = {0};

  // Copy current settings over before retrieving them from storage.
  // If there are any changes, apply them.
  WatchfaceSettings previous_settings = s_settings;

  parse_settings_data(iter, &data);
  parse_weather_data(iter, &data);

#ifdef PBL_HEALTH
  parse_health_settings_data(iter, &data);
  parse_oneshot_health_data(iter, &data);
#endif

  watchface_apply_received_data(&data, &s_settings);

  // The check for weather update interval is managed in JS directly
  // You could be defensive here and send a message but it is redundant
  if (previous_settings.time_format != s_settings.time_format ||
      previous_settings.temp_unit != s_settings.temp_unit ||
      previous_settings.display_mode != s_settings.display_mode ||
#ifdef PBL_HEALTH
      previous_settings.steps_goal != s_settings.steps_goal ||
      previous_settings.hr_sample_minutes != s_settings.hr_sample_minutes ||
#endif
      previous_settings.weather_update_minutes != s_settings.weather_update_minutes) {
    settings_save(&s_settings);
  }

#ifdef PBL_HEALTH
  if (previous_settings.hr_sample_minutes != s_settings.hr_sample_minutes) {
    health_service_set_heart_rate_sample_period((uint16_t)s_settings.hr_sample_minutes * 60);
  }
#endif
}

static void inbox_dropped_callback(
  AppMessageResult reason,
  void* context) {
  (void)context;
  APP_LOG(APP_LOG_LEVEL_WARNING, "AppMessage inbox dropped: reason=%d", reason);
}

static void outbox_failed_callback(
  DictionaryIterator* iterator,
  AppMessageResult reason,
  void* context) {
  (void)iterator;
  (void)context;
  APP_LOG(APP_LOG_LEVEL_ERROR, "AppMessage outbox failed: reason=%d", reason);
}

static void outbox_sent_callback(
  DictionaryIterator* iterator,
  void* context) {
  (void)iterator;
  (void)context;
}

static uint32_t app_message_inbox_size() {
  // Inbound tuples, in order:
  // Refer to package.json for tuple ordering
  // Key-name | Key-Value | Size
  // 1. TIME_FORMAT | 10000 | APP_MESSAGE_CONFIG_VALUE_SIZE
  // 2. TEMP_UNIT | 10001 | APP_MESSAGE_CONFIG_VALUE_SIZE
  // 3. TEMPERATURE | 10002 | sizeof(int32_t)
  // 4. WEATHER_CONDITION | 10003 | sizeof(int32_t)
  // 5. IS_DAY | 10004 | sizeof(int32_t)
  // 6. WEATHER_UPDATE_MINUTES | 10005 | APP_MESSAGE_CONFIG_VALUE_SIZE
  // 7. DISPLAY_MODE | 10006 | APP_MESSAGE_CONFIG_VALUE_SIZE
  // 8. HR_SAMPLE_MINUTES | 10007 | APP_MESSAGE_CONFIG_VALUE_SIZE
  // 9. STEPS_GOAL | 10008 | sizeof(int32_t)
  // STEPS_GOAL_CUSTOM and STEPS_GOAL_PRESET do not cross the JS<->C boundary
  // these two are processed in the handler for WebViewClosed.
  //
  // One-shot-only tuples:
  // 10. ONESHOT_BPM | 10020 | sizeof(int32_t)
  // 11. ONESHOT_STEPS | 10021 | sizeof(int32_t)
#if defined(PBL_HEALTH)
  return dict_calc_buffer_size(11,  // tuples
    APP_MESSAGE_CONFIG_VALUE_SIZE,
    APP_MESSAGE_CONFIG_VALUE_SIZE,
    sizeof(int32_t),
    sizeof(int32_t),
    sizeof(int32_t),
    APP_MESSAGE_CONFIG_VALUE_SIZE,
    APP_MESSAGE_CONFIG_VALUE_SIZE,
    sizeof(int32_t),
    sizeof(int32_t),
    sizeof(int32_t));
#else
  return dict_calc_buffer_size(9,  // tuples
    APP_MESSAGE_CONFIG_VALUE_SIZE,
    APP_MESSAGE_CONFIG_VALUE_SIZE,
    sizeof(int32_t),
    sizeof(int32_t),
    sizeof(int32_t),
    APP_MESSAGE_CONFIG_VALUE_SIZE,
    APP_MESSAGE_CONFIG_VALUE_SIZE,
    sizeof(int32_t));
#endif
}

static AppMessageResult initialize_inbox_outbox() {
  uint32_t inbox_size = app_message_inbox_size();
  bool pebblekit_connected = connection_service_peek_pebblekit_connection();
  AppMessageResult result = app_message_open(inbox_size, APP_MESSAGE_OUTBOX_SIZE);
  if (result == APP_MSG_OK) {
    return result;
  }

  APP_LOG(APP_LOG_LEVEL_WARNING, "AppMessage open failed: result=%d inbox=%lu", result, inbox_size);
  APP_LOG(APP_LOG_LEVEL_WARNING,
    "AppMessage sizes: outbox=%d kit=%d",
    APP_MESSAGE_OUTBOX_SIZE,
    pebblekit_connected);

  result = app_message_open(APP_MESSAGE_INBOX_SIZE_MINIMUM, APP_MESSAGE_OUTBOX_SIZE_MINIMUM);
  if (result != APP_MSG_OK) {
    APP_LOG(APP_LOG_LEVEL_ERROR,
      "AppMessage retry failed: result=%d kit=%d",
      result,
      pebblekit_connected);
  }

  return result;
}

static void send_loaded_weather_update_minutes() {
  DictionaryIterator* out_iter = NULL;
  AppMessageResult result = app_message_outbox_begin(&out_iter);
  if (result != APP_MSG_OK || !out_iter) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "Weather cadence outbox begin failed: result=%d", result);
    return;
  }

  DictionaryResult dict_result = dict_write_int32(out_iter,
    MESSAGE_KEY_WEATHER_UPDATE_MINUTES,
    s_settings.weather_update_minutes);
  if (dict_result != DICT_OK) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "Weather cadence dict write failed: result=%d", dict_result);
    return;
  }

  result = app_message_outbox_send();
  if (result != APP_MSG_OK) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "Weather cadence outbox send failed: result=%d", result);
  }
}

static void main_window_load(
  Window* window) {
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

static void main_window_unload(
  Window* window) {
  (void)window;
  watchface_destroy();
}

static void init() {
// Create the window before initializing app state and services.
#ifdef PBL_HEALTH
  s_health_events_subscribed = false;
#endif

  s_window = window_create();
  if (!s_window) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to create main window");
    return;
  }

  // Load settings
  settings_load(&s_settings);

  window_set_window_handlers(s_window,
    (WindowHandlers){.load = main_window_load, .unload = main_window_unload});
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
    health_service_set_heart_rate_sample_period((uint16_t)s_settings.hr_sample_minutes * 60);
  }
#endif

  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  if (initialize_inbox_outbox() == APP_MSG_OK) {
    send_loaded_weather_update_minutes();
  }
}

static void deinit() {
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

int main() {
  init();
  app_event_loop();
  deinit();
}
