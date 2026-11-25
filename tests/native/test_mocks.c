#include "test_mocks.h"

#include <assert.h>

#include "test_runner.h"
#ifdef PBL_HEALTH
#include "modules/bpm.h"
#include "modules/steps.h"
#endif

// Dictionary tuples.
Tuple* dict_find(
    const DictionaryIterator* iter,
    const uint32_t key) {
  for (size_t i = 0; i < iter->count; ++i) {
    if (iter->tuples[i]->key == key) {
      return iter->tuples[i];
    }
  }
  return NULL;
}

static Tuple* add_tuple(
    DictionaryIterator* iter,
    TestTuple* storage,
    uint32_t key) {
  assert(iter->count < ARRAY_LENGTH(iter->tuples));
  memset(storage, 0, sizeof(*storage));
  Tuple* tuple = (Tuple*)storage->bytes;
  tuple->key = key;
  iter->tuples[iter->count++] = tuple;
  return tuple;
}

void test_dict_int(
    DictionaryIterator* iter,
    TestTuple* storage,
    uint32_t key,
    int32_t value) {
  Tuple* tuple = add_tuple(iter, storage, key);
  tuple->type = TUPLE_INT;
  tuple->length = sizeof(value);
  memcpy(tuple->value, &value, sizeof(value));
}

void test_dict_string(
    DictionaryIterator* iter,
    TestTuple* storage,
    uint32_t key,
    const char* value) {
  assert(strlen(value) + 1 <= sizeof(storage->bytes) - sizeof(Tuple));
  Tuple* tuple = add_tuple(iter, storage, key);
  tuple->type = TUPLE_CSTRING;
  tuple->length = strlen(value) + 1;
  memcpy(tuple->value, value, tuple->length);
}

// Messaging and manual timers.
TestPebble test_pebble;
struct AppTimer {
  bool active;
  uint32_t delay;
  AppTimerCallback callback;
  void* data;
};
static AppTimer timers[16];
static DictionaryIterator outbox;

void test_pebble_reset(
    void) {
  memset(timers, 0, sizeof(timers));
  memset(&test_pebble, 0, sizeof(test_pebble));
  test_pebble.connected = true;
  memset(&outbox, 0, sizeof(outbox));
}

unsigned test_active_timers(
    void) {
  unsigned active = 0;
  for (size_t i = 0; i < ARRAY_LENGTH(timers); ++i) active += timers[i].active;
  return active;
}

static AppTimer* next_timer(
    void) {
  AppTimer* next = NULL;
  for (size_t i = 0; i < ARRAY_LENGTH(timers); ++i) {
    if (timers[i].active && (!next || timers[i].delay < next->delay)) next = &timers[i];
  }
  assert(next);
  return next;
}

uint32_t test_next_timer_delay(
    void) {
  return next_timer()->delay;
}

void test_fire_timer(
    void) {
  AppTimer* timer = next_timer();
  test_note("timer fire: delay=%u ms", timer->delay);
  timer->active = false;
  timer->callback(timer->data);
}

AppTimer* app_timer_register(
    uint32_t delay,
    AppTimerCallback callback,
    void* data) {
  for (size_t i = 0; i < ARRAY_LENGTH(timers); ++i) {
    if (!timers[i].active) {
      timers[i] = (AppTimer){true, delay, callback, data};
      test_note("timer register: delay=%u ms", delay);
      ++test_pebble.registrations;
      return &timers[i];
    }
  }
  assert(false);
  return NULL;
}

void app_timer_cancel(
    AppTimer* timer) {
  assert(timer && timer->active);
  timer->active = false;
  test_note("timer cancel: delay=%u ms", timer->delay);
  ++test_pebble.cancellations;
}

uint32_t test_dict_calc_buffer_size(
    uint32_t count,
    const uint32_t* sizes) {
  // SDK wire format: one count byte, then seven header bytes per tuple.
  uint32_t size = 1 + 7 * count;
  for (uint32_t i = 0; i < count; ++i) size += sizes[i];
  return size;
}

DictionaryResult dict_write_uint8(
    DictionaryIterator* iter,
    uint32_t key,
    uint8_t value) {
  assert(iter == &outbox);
  ++test_pebble.writes;
  test_pebble.written_key = key;
  test_pebble.written_value = value;
  return test_pebble.write_result;
}

AppMessageResult app_message_open(
    uint32_t inbox,
    uint32_t out) {
  ++test_pebble.opens;
  test_pebble.inbox_size = inbox;
  test_pebble.outbox_size = out;
  return test_pebble.open_result;
}

AppMessageResult app_message_outbox_begin(
    DictionaryIterator** iter) {
  ++test_pebble.begins;
  *iter = test_pebble.null_outbox ? NULL : &outbox;
  return test_pebble.begin_result;
}

AppMessageResult app_message_outbox_send(
    void) {
  test_note("outbox send: result=%d", test_pebble.send_result);
  ++test_pebble.sends;
  return test_pebble.send_result;
}

bool connection_service_peek_pebble_app_connection(
    void) {
  return test_pebble.connected;
}
bool health_service_set_heart_rate_sample_period(
    uint16_t seconds) {
  test_pebble.health_period = seconds;
  ++test_pebble.health_changes;
  return true;
}

// Persistent storage.
static struct {
  uint8_t data[16];
  size_t size;
} storage[4];

unsigned test_storage_write_calls;
unsigned test_storage_delete_calls;
int test_storage_read_result;
int test_storage_write_result;

const uint8_t* test_storage_bytes(
    uint32_t key) {
  assert(key < ARRAY_LENGTH(storage));
  return storage[key].data;
}

void test_storage_reset(
    void) {
  memset(storage, 0, sizeof(storage));
  test_storage_write_calls = test_storage_delete_calls = 0;
  test_storage_read_result = test_storage_write_result = NORMAL_IO;
}

void test_storage_seed(
    uint32_t key,
    const uint8_t* data,
    size_t size) {
  assert(key < ARRAY_LENGTH(storage));
  assert(size <= sizeof(storage[key].data));
  memcpy(storage[key].data, data, size);
  storage[key].size = size;
}

bool persist_exists(
    uint32_t key) {
  assert(key < ARRAY_LENGTH(storage));
  return storage[key].size != 0;
}

int persist_get_size(
    uint32_t key) {
  return persist_exists(key) ? (int)storage[key].size : -1;
}

int persist_read_data(
    uint32_t key,
    void* buffer,
    size_t buffer_size) {
  if (test_storage_read_result == -1 || !persist_exists(key)) {
    return -1;
  }
  size_t size = storage[key].size;
  if (size > buffer_size) size = buffer_size;
  if (test_storage_read_result >= 0 && size > (size_t)test_storage_read_result)
    size = (size_t)test_storage_read_result;
  memcpy(buffer, storage[key].data, size);
  return (int)size;
}

int persist_write_data(
    uint32_t key,
    const void* data,
    size_t size) {
  ++test_storage_write_calls;
  if (test_storage_write_result == -1) return -1;
  if (test_storage_write_result >= 0 && size > (size_t)test_storage_write_result)
    size = (size_t)test_storage_write_result;
  test_storage_seed(key, data, size);
  return (int)size;
}

status_t persist_delete(
    uint32_t key) {
  assert(key < ARRAY_LENGTH(storage));
  ++test_storage_delete_calls;
  storage[key].size = 0;
  return 0;
}

// Watchface recording.
TestWatchface test_watchface;
void test_watchface_reset(
    void) {
  memset(&test_watchface, 0, sizeof(test_watchface));
}
void watchface_refresh(
    WatchfaceUpdateMask updates,
    const WatchfaceEventData* data) {
  ++test_watchface.refreshes;
  test_watchface.updates = updates;
  test_watchface.data = *data;
}
void watchface_repaint(
    const WatchfaceEventData* data) {
  ++test_watchface.repaints;
  test_watchface.data = *data;
}
void watchface_maybe_relayout(
    WatchfaceUpdateMask* updates) {
  ++test_watchface.relayouts;
  // Add a marker to prove runtime dispatch observes the updated mask.
  *updates |= WATCHFACE_UPDATE_BT_LAYOUT;
}
#ifdef PBL_HEALTH
void bpm_module_oneshot_set_bpm(
    int bpm) {
  ++test_watchface.bpm_calls;
  test_watchface.bpm = bpm;
}
void steps_module_oneshot_set_steps(
    int steps) {
  ++test_watchface.steps_calls;
  test_watchface.steps = steps;
}
#endif

// SDK geometry primitive.
GRect grect_inset(
    GRect rect,
    GEdgeInsets insets) {
  rect.origin.x += insets.left;
  rect.origin.y += insets.top;
  rect.size.w -= insets.left + insets.right;
  rect.size.h -= insets.top + insets.bottom;
  return rect;
}
