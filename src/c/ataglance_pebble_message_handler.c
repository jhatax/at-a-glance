#include "ataglance_message_parser.h"
#include "ataglance_messages_adapter.h"
#include "modules/settings.h"

/*
 * File invariants:
 *
 * - own Pebble AppMessage transport behavior
 *   This file owns inbox/outbox callbacks, AppMessage sizing/opening, retry
 *   behavior, and app-owned startup sync sends.
 *
 * - parse, then delegate
 *   Inbox handling may build WatchfaceEventData through parser helpers and then
 *   delegate the resolved payload back to ataglance.c.
 *
 * - no settings persistence or runtime interpretation ownership
 *   This file must not decide repaint versus refresh, mutate persisted
 *   settings, or apply feature/runtime policy beyond transport handling.
 *
 * - no layout, palette, or module behavior leakage
 *   AppMessage transport ends at parsed event data and adapter callbacks.
 */

#define APP_MESSAGE_CONFIG_VALUE_SIZE 2
#define APP_MESSAGE_OUTBOX_SIZE 64

#define MESSAGE_SEND_RETRY_MS 1000
#define MAX_ATTEMPTS 6

void inbox_received_handler(
    DictionaryIterator* iter,
    void* context) {
  if (!context) {
    return;
  }

  if (!iter) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "AppMessage inbox received NULL iterator");
    return;
  }
  WatchfaceSettings* settings = (WatchfaceSettings*)context;
  if (!settings) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "AppMessage context should never be NULL");
    return;
  }
  WatchfaceSettings previous = *settings;

  if (parse_ready_sentinel(iter)) {
    return;
  }

  // This was more than the JS_READY message
  WatchfaceEventData data = {0};
  parse_settings_data(iter, &data);
  parse_weather_and_location_data(iter, &data);

#ifdef PBL_HEALTH
  // Parse one-shot health data
  parse_int_tuple(
      iter,
      WATCHFACE_ONESHOT_MESSAGE_KEY_BPM,
      &data,
      WATCHFACE_DATA_ONESHOT_BPM,
      &data.oneshot_bpm);
  parse_int_tuple(
      iter,
      WATCHFACE_ONESHOT_MESSAGE_KEY_STEPS,
      &data,
      WATCHFACE_DATA_ONESHOT_STEPS,
      &data.oneshot_steps);
#endif

  // A side-effect of applying settings to the pointer is that if settings change,
  // the app_message_context is changed under the covers. This is a hidden benefit.
  watchface_apply_received_data(&data, settings);

  // The check for weather update interval is managed in JS directly, so sending an app
  // message to PKJS is redundant.
  if (data.settings_changed) {
    // It is critical that you are deliberately frugal with writing to persisted storage
    settings_save(settings);
  }

#ifdef PBL_HEALTH
  // Change HR-Sample Minutes per user's preference
  if (previous.hr_sample_minutes != settings->hr_sample_minutes) {
    health_service_set_heart_rate_sample_period((uint16_t)settings->hr_sample_minutes * 60);
  }
#endif
}

void inbox_dropped_callback(
    AppMessageResult reason,
    void* context) {
  (void)context;
  APP_LOG(APP_LOG_LEVEL_WARNING, "AppMessage inbox dropped: reason=%d", reason);
}

void outbox_failed_callback(
    DictionaryIterator* iterator,
    AppMessageResult reason,
    void* context) {
  (void)iterator;
  (void)context;
  APP_LOG(APP_LOG_LEVEL_ERROR, "AppMessage outbox failed: reason=%d", reason);
}

void outbox_sent_callback(
    DictionaryIterator* iterator,
    void* context) {
  (void)iterator;
  (void)context;
}

uint32_t app_message_inbox_size() {
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
  // 10. LOCATION | 10011 | 15 Latin-1 characters, up to 31 UTF-8 bytes
  // 11. JS_READY | 10012 | sizeof(int32_t)
  // 12. BATTERY_ORIENTATION | 10013 | APP_MESSAGE_CONFIG_VALUE_SIZE
  //
  // One-shot-only tuples:
  // 13. ONESHOT_BPM | 10020 | sizeof(int32_t)
  // 14. ONESHOT_STEPS | 10021 | sizeof(int32_t)
#if defined(PBL_HEALTH)
  return dict_calc_buffer_size(
      14,  // tuples
      APP_MESSAGE_CONFIG_VALUE_SIZE,
      APP_MESSAGE_CONFIG_VALUE_SIZE,
      sizeof(int32_t),
      sizeof(int32_t),
      sizeof(int32_t),
      APP_MESSAGE_CONFIG_VALUE_SIZE,
      APP_MESSAGE_CONFIG_VALUE_SIZE,
      APP_MESSAGE_CONFIG_VALUE_SIZE,
      sizeof(int32_t),
      sizeof(int32_t),
      sizeof(int32_t),
      WATCHFACE_EVENT_LOCATION_BUFFER_SIZE,
      sizeof(int32_t),
      APP_MESSAGE_CONFIG_VALUE_SIZE);
#else
  return dict_calc_buffer_size(
      12,  // tuples
      APP_MESSAGE_CONFIG_VALUE_SIZE,
      APP_MESSAGE_CONFIG_VALUE_SIZE,
      APP_MESSAGE_CONFIG_VALUE_SIZE,
      sizeof(int32_t),
      sizeof(int32_t),
      sizeof(int32_t),
      APP_MESSAGE_CONFIG_VALUE_SIZE,
      APP_MESSAGE_CONFIG_VALUE_SIZE,
      sizeof(int32_t),
      WATCHFACE_EVENT_LOCATION_BUFFER_SIZE,
      sizeof(int32_t),
      APP_MESSAGE_CONFIG_VALUE_SIZE);
#endif
}

static uint8_t s_init_attempts = 1;
static AppTimer* s_init_timer = NULL;
void attempt_inbox_initialization(void*);

void attempt_inbox_initialization(
    void* context) {
  if (!context) {
    return;
  }
  uint8_t* attempts = (uint8_t*)context;
  uint32_t inbox_size = HELPER_MAX(APP_MESSAGE_INBOX_SIZE_MINIMUM, app_message_inbox_size());
  // Need a minimum of 10-bytes to send the weather_update_minutes to JS
  AppMessageResult result = app_message_open(
      HELPER_MAX(APP_MESSAGE_INBOX_SIZE_MINIMUM, app_message_inbox_size()),
      HELPER_MAX(10, APP_MESSAGE_OUTBOX_SIZE_MINIMUM));

  // We got here because we were called -> by initialize_inbox_outbox or a timer fired
  // therefore, we should reset s_init_timer to NULL
  s_init_timer = NULL;
  if (result != APP_MSG_OK) {
    APP_LOG(
        APP_LOG_LEVEL_WARNING,
        "AppMessage open failed: result=%d inbox=%lu",
        result,
        inbox_size);
    APP_LOG(APP_LOG_LEVEL_WARNING, "AppMessage sizes: outbox=%d", APP_MESSAGE_OUTBOX_SIZE);
    ++(*attempts);
    if (*attempts <= MAX_ATTEMPTS) {
      s_init_timer = app_timer_register(
          *attempts * MESSAGE_SEND_RETRY_MS,
          attempt_inbox_initialization,
          context);
    }
    return;
  }
}

void initialize_inbox_outbox(
    void* context) {
  (void)context;
  if (s_init_timer) {
    app_timer_cancel(s_init_timer);
    s_init_timer = NULL;
  }
  s_init_attempts = 1;
  attempt_inbox_initialization(&s_init_attempts);
}
