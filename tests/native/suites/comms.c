#include "c/ataglance_comms_adapter.h"
#include "test_mocks.h"
#include "test_runner.h"

// Suite: communications initialization, retries, inbox handling, and persistence.
// The adapter retains this address, just as it retains the app-owned timer slot.
static AppTimer* retry_timer;

// Clar lifecycle: cleanup runs after each case; cases initialize their dependencies.
void test_comms__cleanup(
    void) {
  // Settle adapter state before resetting the fake timers it still references.
  test_pebble.open_result = APP_MSG_OK;
  initialize_inbox_outbox(&retry_timer);
  outbox_sent_callback(NULL, NULL);
  test_pebble_reset();
}

// Suite helpers: called explicitly by test cases.
// Start each sync scenario with fresh dependencies and initialized communications.
static void start_comms_scenario(
    WatchfaceSettings* settings) {
  test_pebble_reset();
  test_storage_reset();
  test_watchface_reset();
  settings_load(settings);
  initialize_inbox_outbox(&retry_timer);
}

static void receive_sync(
    WatchfaceSettings* settings) {
  DictionaryIterator iter = {0};
  TestTuple tuple;
  test_dict_int(&iter, &tuple, MESSAGE_KEY_SYNC, 1);
  inbox_received_handler(&iter, settings);
}

// Test cases.
void test_comms__open_retries(
    void) {
  test_pebble_reset();
  test_pebble.open_result = APP_MSG_BUSY;
  initialize_inbox_outbox(&retry_timer);
  cl_assert(test_pebble.opens == 1 && test_active_timers() == 1);
  for (unsigned attempt = 2; attempt <= 6; ++attempt) {
    cl_assert_equal_i(attempt * 1000, test_next_timer_delay());
    test_fire_timer();
    cl_assert_equal_i(attempt, test_pebble.opens);
    cl_assert_equal_i((attempt < 6), test_active_timers());
  }
  // Exhaustion stops. A later initialization starts a fresh bounded sequence.
  test_pebble.open_result = APP_MSG_OK;
  initialize_inbox_outbox(&retry_timer);
  cl_assert(test_pebble.opens == 7 && !test_active_timers());
#ifdef PBL_HEALTH
  // Thirteen seven-byte tuple headers, one count byte, 69 value bytes.
  cl_assert_equal_i(161, test_pebble.inbox_size);
#else
  cl_assert_equal_i(124, test_pebble.inbox_size);
#endif
  cl_assert_equal_i(APP_MESSAGE_OUTBOX_SIZE_MINIMUM, test_pebble.outbox_size);
}

void test_comms__reinitialize_cancels_timer(
    void) {
  test_pebble_reset();
  test_pebble.open_result = APP_MSG_BUSY;
  initialize_inbox_outbox(&retry_timer);
  cl_assert_equal_i(1, test_active_timers());
  initialize_inbox_outbox(&retry_timer);
  cl_assert(test_pebble.cancellations == 1 && test_active_timers() == 1);
  cl_assert_equal_i(2000, test_next_timer_delay());
  test_pebble.open_result = APP_MSG_OK;
  test_fire_timer();
  cl_assert(test_pebble.opens == 3 && !test_active_timers());
}

void test_comms__sync_failure_paths(
    void) {
  enum { NO_CONNECTION, BEGIN_BUSY, NULL_OUTBOX, WRITE_FULL, SEND_BUSY, ACK_TIMEOUT };
  const struct {
    const char* name;
    unsigned begins, writes, sends;
  } scenarios[] = {
      [NO_CONNECTION] = {"no connection", 0, 0, 0},
      [BEGIN_BUSY] = {"outbox begin busy", 6, 0, 0},
      [NULL_OUTBOX] = {"missing outbox iterator", 6, 0, 0},
      [WRITE_FULL] = {"dictionary full", 6, 6, 0},
      [SEND_BUSY] = {"outbox send busy", 6, 6, 6},
      [ACK_TIMEOUT] = {"delivery acknowledgment timeout", 6, 6, 6},
  };
  // Each transport failure must use the same six-attempt budget.
  for (size_t failure = 0; failure < ARRAY_LENGTH(scenarios); ++failure) {
    test_context("sync failure=%s", scenarios[failure].name);
    WatchfaceSettings settings;
    start_comms_scenario(&settings);
    switch (failure) {
      case NO_CONNECTION:
        test_pebble.connected = false;
        break;
      case BEGIN_BUSY:
        test_pebble.begin_result = APP_MSG_BUSY;
        break;
      case NULL_OUTBOX:
        test_pebble.null_outbox = true;
        break;
      case WRITE_FULL:
        test_pebble.write_result = DICT_NOT_ENOUGH_STORAGE;
        break;
      case SEND_BUSY:
        test_pebble.send_result = APP_MSG_BUSY;
        break;
      case ACK_TIMEOUT:
        break;  // The API accepts the message; delivery fails asynchronously.
    }
    receive_sync(&settings);
    if (failure == ACK_TIMEOUT) outbox_failed_callback(NULL, APP_MSG_SEND_TIMEOUT, &settings);
    for (unsigned attempt = 1; attempt < 6; ++attempt) {
      cl_assert(test_active_timers() == 1 && retry_timer);
      cl_assert_equal_i(attempt * 1000, test_next_timer_delay());
      test_fire_timer();
      if (failure == ACK_TIMEOUT) outbox_failed_callback(NULL, APP_MSG_SEND_TIMEOUT, &settings);
    }
    cl_assert(!test_active_timers() && !retry_timer);
    cl_assert_equal_i(scenarios[failure].begins, test_pebble.begins);
    cl_assert_equal_i(scenarios[failure].writes, test_pebble.writes);
    cl_assert_equal_i(scenarios[failure].sends, test_pebble.sends);
    cl_assert(!test_storage_write_calls);
  }
}

void test_comms__sync_recovery_and_ack(
    void) {
  WatchfaceSettings settings;
  start_comms_scenario(&settings);
  test_pebble.connected = false;
  receive_sync(&settings);
  cl_assert_equal_i(1, test_active_timers());
  test_pebble.connected = true;
  test_fire_timer();
  cl_assert(test_pebble.sends == 1 && !test_active_timers() && !retry_timer);
  cl_assert(test_pebble.written_key == MESSAGE_KEY_SYNC && test_pebble.written_value == 1);
  outbox_failed_callback(NULL, APP_MSG_SEND_TIMEOUT, &settings);
  cl_assert_equal_i(2000, test_next_timer_delay());
  // Repeated failure notifications must not add a second timer.
  unsigned registrations = test_pebble.registrations;
  outbox_failed_callback(NULL, APP_MSG_SEND_TIMEOUT, &settings);
  cl_assert(test_pebble.registrations == registrations && test_active_timers() == 1);
  outbox_sent_callback(NULL, &settings);
  cl_assert(!retry_timer && !test_active_timers() && test_pebble.cancellations == 1);
}

void test_comms__resync_replaces_retry(
    void) {
  WatchfaceSettings settings;
  start_comms_scenario(&settings);
  test_pebble.connected = false;
  receive_sync(&settings);
  test_fire_timer();
  cl_assert_equal_i(2000, test_next_timer_delay());
  receive_sync(&settings);
  cl_assert(test_pebble.cancellations == 1 && test_next_timer_delay() == 1000);
  for (unsigned attempt = 1; attempt < 6; ++attempt) test_fire_timer();
  cl_assert(!retry_timer && !test_active_timers());
  // Phone-app resync remains available after exhaustion.
  test_pebble.connected = true;
  receive_sync(&settings);
  cl_assert_equal_i(1, test_pebble.sends);
  outbox_sent_callback(NULL, &settings);
}

void test_comms__coalesced_inbox(
    void) {
  WatchfaceSettings settings;
  start_comms_scenario(&settings);
  DictionaryIterator iter = {0};
  TestTuple tuples[8];
  test_dict_int(&iter, &tuples[0], MESSAGE_KEY_SYNC, 1);
  test_dict_int(&iter, &tuples[1], MESSAGE_KEY_TEMP_UNIT, TEMP_UNIT_C);
  test_dict_int(&iter, &tuples[2], MESSAGE_KEY_TEMPERATURE, 215);
  test_dict_int(&iter, &tuples[3], MESSAGE_KEY_WEATHER_CONDITION, 3);
  test_dict_int(&iter, &tuples[4], MESSAGE_KEY_IS_DAY, 1);
  test_dict_string(&iter, &tuples[5], MESSAGE_KEY_MAYBE_CURRENT_LOCATION, "San Jose");
  test_dict_int(&iter, &tuples[6], 10020, 72);
  test_dict_int(&iter, &tuples[7], 10021, 12345);
  inbox_received_handler(&iter, &settings);
  cl_assert(test_pebble.sends == 1 && settings.temp_unit == TEMP_UNIT_C);
  cl_assert(test_storage_write_calls == 1 && persist_exists(3));
  cl_assert(test_watchface.refreshes == 1 && !test_watchface.repaints);
  WatchfaceUpdateMask expected = WATCHFACE_UPDATE_CLIMATE | WATCHFACE_UPDATE_LOCATION;
#ifdef PBL_HEALTH
  expected |= WATCHFACE_UPDATE_HEALTH;
  cl_assert(test_watchface.bpm == 72 && test_watchface.steps == 12345);
#endif
  cl_assert_equal_i(expected, test_watchface.updates);
  cl_assert(test_watchface.data.current_climate.complete);
  cl_assert_equal_i(215, test_watchface.data.current_climate.celsius_tenths);
  cl_assert(strcmp(test_watchface.data.pin.name, "San Jose") == 0);
  WatchfaceSettings reloaded;
  settings_load(&reloaded);
  cl_assert_equal_i(TEMP_UNIT_C, reloaded.temp_unit);
  inbox_received_handler(&iter, &settings);
  cl_assert_equal_i(1, test_storage_write_calls);
  outbox_sent_callback(NULL, &settings);
}

void test_comms__inbox_persistence_policy(
    void) {
  WatchfaceSettings settings;
  start_comms_scenario(&settings);
  DictionaryIterator iter = {0};
  TestTuple tuple;
  test_dict_int(&iter, &tuple, MESSAGE_KEY_TEMP_UNIT, 99);
  inbox_received_handler(&iter, &settings);
  cl_assert(!test_storage_write_calls && settings.temp_unit == TEMP_UNIT_DEFAULT);
  iter = (DictionaryIterator){0};
  test_dict_int(&iter, &tuple, MESSAGE_KEY_TEMPERATURE, 200);
  inbox_received_handler(&iter, &settings);
  cl_assert(!test_storage_write_calls && test_watchface.data.current_climate.received);
  cl_assert(!test_watchface.data.current_climate.complete);
#ifdef PBL_HEALTH
  iter = (DictionaryIterator){0};
  test_dict_int(&iter, &tuple, MESSAGE_KEY_HR_SAMPLE_MINUTES, 30);
  inbox_received_handler(&iter, &settings);
  cl_assert(test_pebble.health_changes == 1 && test_pebble.health_period == 1800);
  cl_assert_equal_i(1, test_storage_write_calls);
  inbox_received_handler(&iter, &settings);
  cl_assert(test_pebble.health_changes == 1 && test_storage_write_calls == 1);
  ((Tuple*)tuple.bytes)->value->int32 = 0;
  inbox_received_handler(&iter, &settings);
  cl_assert(test_pebble.health_changes == 1 && test_storage_write_calls == 1);
#endif
  unsigned writes = test_storage_write_calls;
  test_watchface_reset();
  inbox_received_handler(NULL, &settings);
  inbox_received_handler(&iter, NULL);
  inbox_dropped_callback(APP_MSG_BUSY, &settings);
  cl_assert(!test_watchface.refreshes && test_storage_write_calls == writes);
}
