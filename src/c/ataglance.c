#include <pebble.h>

#include "ataglance_messages_adapter.h"
#include "modules/settings.h"
#include "modules/watchface.h"

/*
 * File invariants:
 *
 * - Pebble OS adapter and app-lifecycle owner
 *   This file owns the main window lifecycle, Pebble service subscription,
 *   settings load/save triggers, and app-level teardown.
 *
 * - construct event data, then delegate
 *   Pebble callbacks may build WatchfaceEventData and pass it into the
 *   watchface runtime. Runtime interpretation belongs elsewhere.
 *
 * - app-owner side effects only
 *   This file may perform side effects that only the app owner can perform,
 *   such as service subscription, callback registration, and health sample
 *   period updates.
 *
 * - no AppMessage parsing or transport-handler ownership
 *   Raw AppMessage parsing, inbox/outbox callbacks, and transport sizing/open
 *   belong to the Pebble message adapter helpers.
 *
 * - no watchface implementation leakage
 *   This file must not own layout geometry, palette policy, glyph rendering,
 *   repaint-versus-refresh interpretation, or feature-module behavior.
 *
 * - callback bodies stay thin
 *   Callbacks should capture the Pebble event, build minimal event data, and
 *   delegate without accumulating unrelated policy.
 */

static Window* s_window = NULL;
static WatchfaceSettings s_settings = {0};
#ifdef PBL_HEALTH
static bool s_health_events_subscribed = false;
#endif

// Lifecycle
static void init();
static void deinit();
static void main_window_load(Window* window);
static void main_window_unload(Window* window);

// Event handlers
static void battery_handler(BatteryChargeState state);
static void tick_handler(struct tm* tick_time, TimeUnits units_changed);
#ifdef PBL_HEALTH
static void health_handler(HealthEventType event, void* context);
#endif

// Double-tap
static AppTimer* s_tap_debounce_timer = NULL;
static bool s_first_tap_seen = false;
#define DOUBLE_TAP_TIMEOUT_MS 500

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
#endif

static void tap_debounce_timer_callback(
  void* data) {
  s_first_tap_seen = false;
}

static void accel_tap_handler(
  AccelAxisType axis,
  int32_t direction) {
  if (!s_first_tap_seen) {
    s_first_tap_seen = true;
    s_tap_debounce_timer =
      app_timer_register(DOUBLE_TAP_TIMEOUT_MS, tap_debounce_timer_callback, NULL);
  } else {
    app_timer_cancel(s_tap_debounce_timer);
    s_first_tap_seen = false;

    WatchfaceEventData data = {0};
    // It is sufficient to set these values because they trigger a health refresh
    data.received = WATCHFACE_DATA_DISPLAY_MODE;
    data.parsed = WATCHFACE_DATA_DISPLAY_MODE;
    data.display_mode = (s_settings.display_mode + 1) % DISPLAY_MODE_COUNT;
    APP_LOG(APP_LOG_LEVEL_WARNING,
      "Received double-tap. Changing mode from %d to %d",
      s_settings.display_mode,
      data.display_mode);
    watchface_apply_received_data(&data, &s_settings);
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
  accel_tap_service_subscribe(accel_tap_handler);

  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  if (initialize_inbox_outbox() == APP_MSG_OK) {
    send_loaded_weather_update_minutes(s_settings.weather_update_minutes);
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
  accel_tap_service_unsubscribe();
  settings_save(&s_settings);
  window_destroy(s_window);
  s_window = NULL;
}

// External API

// Connect to the watch face's adapter to display received changes
// using the established visual vocabulary.
void ataglance_apply_received_data(
  WatchfaceEventData* parsed) {
  if (!parsed) {
    return;
  }

  // Copy current settings over before retrieving them from storage.
  // If there are any changes, apply them.
  WatchfaceSettings previous_settings = s_settings;

  watchface_apply_received_data(parsed, &s_settings);

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

int main() {
  init();
  app_event_loop();
  deinit();
}
