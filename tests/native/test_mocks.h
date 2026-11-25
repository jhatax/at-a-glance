#pragma once

#include <pebble.h>

#include "modules/watchface.h"

// Dictionary inputs.
// Only the dictionary lookup and construction behavior exercised by these tests.
struct DictionaryIterator {
  Tuple* tuples[16];
  size_t count;
};
typedef struct {
  uint8_t bytes[sizeof(Tuple) + 64];
} TestTuple;

void test_dict_int(
    DictionaryIterator* iter,
    TestTuple* storage,
    uint32_t key,
    int32_t value);
void test_dict_string(
    DictionaryIterator* iter,
    TestTuple* storage,
    uint32_t key,
    const char* value);

// Messaging controls and observations.
typedef struct {
  AppMessageResult open_result;
  AppMessageResult begin_result;
  AppMessageResult send_result;
  DictionaryResult write_result;
  bool connected;
  bool null_outbox;
  unsigned opens, begins, sends, writes, cancellations, registrations;
  uint32_t inbox_size, outbox_size, written_key;
  uint8_t written_value;
  uint16_t health_period;
  unsigned health_changes;
} TestPebble;

extern TestPebble test_pebble;
void test_pebble_reset(void);
unsigned test_active_timers(void);
uint32_t test_next_timer_delay(void);
void test_fire_timer(void);

// Storage controls and observations.
enum { NORMAL_IO = -2 };
extern unsigned test_storage_write_calls;
extern unsigned test_storage_delete_calls;
extern int test_storage_read_result;
extern int test_storage_write_result;
void test_storage_reset(void);
void test_storage_seed(
    uint32_t key,
    const uint8_t* data,
    size_t size);
const uint8_t* test_storage_bytes(uint32_t key);

// Watchface observations.
typedef struct {
  unsigned refreshes, repaints, relayouts;
  WatchfaceUpdateMask updates;
  WatchfaceEventData data;
#ifdef PBL_HEALTH
  unsigned bpm_calls, steps_calls;
  int bpm, steps;
#endif
} TestWatchface;
extern TestWatchface test_watchface;
void test_watchface_reset(void);
