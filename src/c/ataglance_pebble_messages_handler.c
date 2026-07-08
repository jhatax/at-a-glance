#include "ataglance_message_parser.h"
#include "ataglance_pebble_messages_adapter.h"

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

void send_loaded_weather_update_minutes(
  uint8_t minutes) {
  DictionaryIterator* out_iter = NULL;
  AppMessageResult result = app_message_outbox_begin(&out_iter);
  if (result != APP_MSG_OK || !out_iter) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "Weather cadence outbox begin failed: result=%d", result);
    return;
  }

  DictionaryResult dict_result =
    dict_write_int32(out_iter, MESSAGE_KEY_WEATHER_UPDATE_MINUTES, minutes);
  if (dict_result != DICT_OK) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "Weather cadence dict write failed: result=%d", dict_result);
    return;
  }

  result = app_message_outbox_send();
  if (result != APP_MSG_OK) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "Weather cadence outbox send failed: result=%d", result);
  }
}

void inbox_received_callback(
  DictionaryIterator* iter,
  void* context) {
  (void)context;

  if (!iter) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "AppMessage inbox received NULL iterator");
    return;
  }

  WatchfaceEventData data = {0};

  parse_settings_data(iter, &data);
  parse_weather_data(iter, &data);

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
    APP_MESSAGE_CONFIG_VALUE_SIZE,
    sizeof(int32_t),
    sizeof(int32_t),
    sizeof(int32_t));
#else
  return dict_calc_buffer_size(9,  // tuples
    APP_MESSAGE_CONFIG_VALUE_SIZE,
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

AppMessageResult initialize_inbox_outbox() {
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
