#include "ataglance_comms_adapter.h"
#include "ataglance_message_parser.h"
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

static uint8_t s_weather_sync_attempts = 0;
static AppTimer** s_retry_timer = NULL;
static AppTimer** s_refresh_timer = NULL;

static uint8_t s_init_attempts = 1;
static AppTimer* s_init_timer = NULL;

void attempt_inbox_initialization(void* context);
static void request_refresh(void* context);
static void try_refresh(void* context);
static void schedule_retry(void* context);
static void schedule_next_cycle(void* context);
static void repeat_refresh(void* context);

// AppMessage initialization.
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

void initialize_inbox_outbox(
    AppTimer** refresh_timer,
    AppTimer** retry_timer) {
  if (refresh_timer && retry_timer) {
    s_refresh_timer = refresh_timer;
    s_retry_timer = retry_timer;
  }
  if (s_init_timer) {
    app_timer_cancel(s_init_timer);
    s_init_timer = NULL;
  }
  s_init_attempts = 1;
  attempt_inbox_initialization(&s_init_attempts);
}

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

// Incoming messages and refresh requests.
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
#ifdef PBL_HEALTH
  uint8_t prev_hr_sampling = settings->hr_sample_minutes;
#endif

  if (!settings) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "AppMessage context should never be NULL");
    return;
  }
  if (parse_ready_sentinel(iter)) {
    request_refresh(context);
  }

  // Sometimes, PKJS can coalesce messages, so don't assume
  // that the sentinel is being sent as its own message
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

  // Capture the value of update minutes before it will be applied to settings
  const uint8_t prev_update_minutes = settings->weather_update_minutes;

  // A side-effect of applying settings to the pointer is that if settings change,
  // the app_message_context is changed under the covers.
  watchface_apply_received_data(&data, settings);

  // It is critical that we are deliberately frugal with writing to persisted storage
  // We should persist only if a setting has changed vs. all the time.
  // Additionally, check whether a new refresh cycle needs to be initiated
  // We don't need to inform PKJS because that layer has its own copy of settings
  if (data.settings_changed) {
    settings_save(settings);
    if (prev_update_minutes != settings->weather_update_minutes) {
      request_refresh(context);
      schedule_next_cycle(context);
    }
  }

#ifdef PBL_HEALTH
  // Change HR-Sample Minutes per user's preference
  if (prev_hr_sampling != settings->hr_sample_minutes) {
    health_service_set_heart_rate_sample_period((uint16_t)settings->hr_sample_minutes * 60);
  }
#endif
}

// Mechanism to request a single refresh of climate and location
static void request_refresh(
    void* context) {
  if (s_retry_timer && *s_retry_timer) {
    app_timer_cancel(*s_retry_timer);
    *s_retry_timer = NULL;
  }

  APP_LOG(APP_LOG_LEVEL_INFO, "Refresh requested");

  s_weather_sync_attempts = 0;
  try_refresh(context);
}

static void try_refresh(
    void* context) {
  // Set the retry-timer to NULL => we got here because of a direct-call from request_refresh or
  // the timer fired after interval elapsed.
  *s_retry_timer = NULL;
  // Record this as an attempt
  ++s_weather_sync_attempts;

  DictionaryIterator* out_iter = NULL;
  AppMessageResult result = app_message_outbox_begin(&out_iter);
  if (result != APP_MSG_OK || !out_iter) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "Weather cadence outbox begin failed: result=%d", result);
    schedule_retry(context);
    return;
  }

  DictionaryResult dict_result = dict_write_uint8(out_iter, MESSAGE_KEY_REFRESH, 1);
  if (dict_result != DICT_OK) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "Weather refresh dict write failed: result=%d", dict_result);
    schedule_retry(context);
    return;
  }

  result = app_message_outbox_send();
  // Will check for immediate delivery issues, such as a busy-mailbox.
  // Does not confirm that the message was actually delivered or accepted.
  // Final delivery failure needs to be handled in outbox_failed_callback.
  if (result != APP_MSG_OK) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "Weather refresh outbox send failed: result=%d", result);
    schedule_retry(context);
    return;
  }
}

static void schedule_retry(
    void* context) {
  if (s_weather_sync_attempts >= MAX_ATTEMPTS) {
    // This means no more attempts should be made
    if (s_retry_timer && *s_retry_timer) {
      app_timer_cancel(*s_retry_timer);
      *s_retry_timer = NULL;
    }
    // The next refresh should be scheduled here as well:
    // Just because weather sync request failed, doesn't mean we give up.
    schedule_next_cycle(context);
    return;
  }
  if (s_retry_timer && *s_retry_timer) {
    // A timer has been set; don't set a new one; wait for the current one to fire
    return;
  }

  *s_retry_timer =
      app_timer_register(MESSAGE_SEND_RETRY_MS * s_weather_sync_attempts, try_refresh, NULL);
}

// Mechanism to wait for the requested interval and then request new data
static void schedule_next_cycle(
    void* context) {
  // If there is an existing timer, cancel it.
  if (*s_refresh_timer) {
    app_timer_cancel(*s_refresh_timer);
    *s_refresh_timer = NULL;
  }

  if (!context) {
    context = app_message_get_context();
  }
  WatchfaceSettings* settings = (WatchfaceSettings*)context;
  const uint32_t interval = settings->weather_update_minutes * 60 * 1000;
  APP_LOG(APP_LOG_LEVEL_INFO, "Setting Refresh Timer to be: %d", interval);
  *s_refresh_timer = app_timer_register(interval, repeat_refresh, context);
}

static void repeat_refresh(
    void* context) {
  *s_refresh_timer = NULL;
  request_refresh(context);
}

// Inbox, Outbox Callbacks
void inbox_dropped_callback(
    AppMessageResult reason,
    void* context) {
  (void)context;
  APP_LOG(APP_LOG_LEVEL_WARNING, "AppMessage inbox dropped: reason=%d", reason);
}

// Send failed; retry
void outbox_failed_callback(
    DictionaryIterator* iterator,
    AppMessageResult reason,
    void* context) {
  (void)iterator;
  APP_LOG(APP_LOG_LEVEL_ERROR, "AppMessage outbox failed: reason=%d", reason);
  schedule_retry(context);
}

// Send succeeded; schedule the next refresh to occur after user requested interval
void outbox_sent_callback(
    DictionaryIterator* iterator,
    void* context) {
  (void)iterator;
  s_weather_sync_attempts = 0;
  if (s_retry_timer && *s_retry_timer) {
    app_timer_cancel(*s_retry_timer);
    *s_retry_timer = NULL;
  }

  // Schedule the next refresh
  schedule_next_cycle(context);
}
