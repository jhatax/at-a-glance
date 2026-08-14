#include <stdint.h>
#include <string.h>

#include "ataglance_message_parser.h"
#include "ataglance_messages_adapter.h"
#include "modules/watchface.h"

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

static uint8_t s_pending_weather_update_minutes = 0;
static uint8_t s_weather_sync_attempts = 0;
static AppTimer* s_weather_sync_retry_timer = NULL;

static void send_pending_weather_update_minutes();

static void retry_pending_weather_update_minutes(
    void* context) {
  (void)context;
  s_weather_sync_retry_timer = NULL;
  ++s_weather_sync_attempts;
  send_pending_weather_update_minutes();
}

static void schedule_weather_sync_retry(
    void) {
  // Return if the number of retries has been exceeded or there is a timer already inflight
  if (s_weather_sync_retry_timer || s_weather_sync_attempts > MAX_ATTEMPTS) {
    return;
  }

  s_weather_sync_retry_timer = app_timer_register(
      MESSAGE_SEND_RETRY_MS * s_weather_sync_attempts,
      retry_pending_weather_update_minutes,
      NULL);
}

static void send_pending_weather_update_minutes() {
  DictionaryIterator* out_iter = NULL;
  AppMessageResult result = app_message_outbox_begin(&out_iter);
  if (result != APP_MSG_OK || !out_iter) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "Weather cadence outbox begin failed: result=%d", result);
    schedule_weather_sync_retry();
    return;
  }

  DictionaryResult dict_result = dict_write_int32(
      out_iter,
      MESSAGE_KEY_WEATHER_UPDATE_MINUTES,
      s_pending_weather_update_minutes);
  if (dict_result != DICT_OK) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "Weather cadence dict write failed: result=%d", dict_result);
    schedule_weather_sync_retry();
    return;
  }

  result = app_message_outbox_send();
  if (result != APP_MSG_OK) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "Weather cadence outbox send failed: result=%d", result);
    schedule_weather_sync_retry();
    return;
  }
}

void send_loaded_weather_update_minutes(
    uint8_t minutes) {
  if (s_weather_sync_retry_timer) {
    app_timer_cancel(s_weather_sync_retry_timer);
    s_weather_sync_retry_timer = NULL;
  }

  s_pending_weather_update_minutes = minutes;
  s_weather_sync_attempts = 1;
  send_pending_weather_update_minutes();
}

bool find_the_canary(
    DictionaryIterator* iter) {
  return parse_ready_sentinel(iter);
}

void handle_the_message(
    DictionaryIterator* iter) {
  if (!iter) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "AppMessage inbox received NULL iterator");
    return;
  }

  WatchfaceEventData data = {0};

  parse_settings_data(iter, &data);
  parse_weather_and_location_data(iter, &data);

#ifdef PBL_HEALTH
  parse_health_settings_data(iter, &data);
  parse_oneshot_health_data(iter, &data);
#endif

  ataglance_apply_received_data(&data);
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
  schedule_weather_sync_retry();
}

void outbox_sent_callback(
    DictionaryIterator* iterator,
    void* context) {
  (void)iterator;
  (void)context;
  s_weather_sync_attempts = 0;
  if (s_weather_sync_retry_timer) {
    app_timer_cancel(s_weather_sync_retry_timer);
    s_weather_sync_retry_timer = NULL;
  }
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
  // 10. LOCATION | 10011 | 15 Latin-1 characters, up to 31 UTF-8 bytes
  // 11. JS_READY | 10012 | sizeof(int32_t)
  //
  // One-shot-only tuples:
  // 12. ONESHOT_BPM | 10020 | sizeof(int32_t)
  // 13. ONESHOT_STEPS | 10021 | sizeof(int32_t)
#if defined(PBL_HEALTH)
  return dict_calc_buffer_size(
      13,  // tuples
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
      sizeof(int32_t));
#else
  return dict_calc_buffer_size(
      11,  // tuples
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
      sizeof(int32_t));
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
  AppMessageResult result = app_message_open(inbox_size, APP_MESSAGE_OUTBOX_SIZE_MINIMUM);
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
