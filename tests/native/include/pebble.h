#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct {
  uint8_t argb;
} GColor;

#define ARRAY_LENGTH(array) (sizeof(array) / sizeof((array)[0]))

typedef enum {
  TUPLE_BYTE_ARRAY = 0,
  TUPLE_CSTRING = 1,
  TUPLE_UINT = 2,
  TUPLE_INT = 3,
} TupleType;

// Match the SDK's serialized tuple layout, including inline variable-length data.
typedef struct __attribute__((__packed__)) {
  uint32_t key;
  TupleType type : 8;
  uint16_t length;
  // The serialized value starts at byte seven, without native alignment.
  union __attribute__((__packed__)) {
    uint8_t data[0];
    char cstring[0];
    uint8_t uint8;
    uint16_t uint16;
    uint32_t uint32;
    int8_t int8;
    int16_t int16;
    int32_t int32;
  } value[];
} Tuple;

_Static_assert(
    sizeof(Tuple) == 7,
    "Pebble tuple header is seven bytes");
_Static_assert(
    offsetof(
        Tuple,
        value) == 7,
    "Tuple payload follows the packed header");

typedef struct DictionaryIterator DictionaryIterator;
typedef struct Window Window;

Tuple* dict_find(
    const DictionaryIterator* iter,
    const uint32_t key);

typedef int32_t status_t;

bool persist_exists(uint32_t key);
int persist_get_size(uint32_t key);
int persist_read_data(
    uint32_t key,
    void* buffer,
    size_t buffer_size);
int persist_write_data(
    uint32_t key,
    const void* data,
    size_t size);
status_t persist_delete(uint32_t key);

// Manifest-backed keys referenced by the production parser.
enum {
  MESSAGE_KEY_TEMP_UNIT = 10001,
  MESSAGE_KEY_TEMPERATURE = 10002,
  MESSAGE_KEY_WEATHER_CONDITION = 10003,
  MESSAGE_KEY_IS_DAY = 10004,
  MESSAGE_KEY_WEATHER_UPDATE_MINUTES = 10005,
  MESSAGE_KEY_DISPLAY_MODE = 10006,
  MESSAGE_KEY_HR_SAMPLE_MINUTES = 10007,
  MESSAGE_KEY_STEPS_GOAL = 10008,
  MESSAGE_KEY_MAYBE_CURRENT_LOCATION = 10011,
  MESSAGE_KEY_SYNC = 10012,
  MESSAGE_KEY_BATTERY_ORIENTATION = 10013,
};

// Evaluate log arguments so warning checks match the production build.
#define APP_LOG(...) test_app_log(__VA_ARGS__)
enum { APP_LOG_LEVEL_WARNING, APP_LOG_LEVEL_ERROR };
void test_app_log(
    int level,
    const char* format,
    ...);

#define GColorClear ((GColor){.argb = 0})
#define GColorWhite ((GColor){.argb = 0xFF})

#ifndef PBL_DISPLAY_WIDTH
#define PBL_DISPLAY_WIDTH 144
#endif
#ifndef PBL_DISPLAY_HEIGHT
#define PBL_DISPLAY_HEIGHT 168
#endif

typedef struct {
  int16_t x;
  int16_t y;
} GPoint;

typedef struct {
  int16_t w;
  int16_t h;
} GSize;

typedef struct {
  GPoint origin;
  GSize size;
} GRect;

#define GPoint(x_value, y_value) ((GPoint){.x = (x_value), .y = (y_value)})
#define GRect(x_value, y_value, w_value, h_value) \
  ((GRect){.origin = GPoint((x_value), (y_value)), .size = (GSize){.w = (w_value), .h = (h_value)}})

#ifdef PBL_ROUND
#define PBL_IF_ROUND_ELSE(round_value, rectangular_value) (round_value)
#define PBL_IF_RECT_ELSE(rectangular_value, round_value) (round_value)
#else
#define PBL_IF_ROUND_ELSE(round_value, rectangular_value) (rectangular_value)
#define PBL_IF_RECT_ELSE(rectangular_value, round_value) (rectangular_value)
#endif

typedef enum { GTextAlignmentLeft, GTextAlignmentCenter, GTextAlignmentRight } GTextAlignment;
typedef struct Layer Layer;
typedef struct {
  uint32_t num_points;
  GPoint* points;
  int32_t rotation;
  GPoint offset;
} GPath;
typedef struct {
  int16_t top, right, bottom, left;
} GEdgeInsets;
#define GEdgeInsets(value) ((GEdgeInsets){value, value, value, value})
GRect grect_inset(
    GRect rect,
    GEdgeInsets insets);

typedef struct AppTimer AppTimer;
typedef void (*AppTimerCallback)(void* data);
AppTimer* app_timer_register(
    uint32_t timeout_ms,
    AppTimerCallback callback,
    void* data);
void app_timer_cancel(AppTimer* timer);

typedef enum {
  APP_MSG_OK = 0,
  APP_MSG_SEND_TIMEOUT = 1 << 1,
  APP_MSG_NOT_CONNECTED = 1 << 3,
  APP_MSG_BUSY = 1 << 6,
} AppMessageResult;
typedef enum { DICT_OK = 0, DICT_NOT_ENOUGH_STORAGE = 1 << 1 } DictionaryResult;
#define APP_MESSAGE_INBOX_SIZE_MINIMUM 124
#define APP_MESSAGE_OUTBOX_SIZE_MINIMUM 636
// Host size_t is wider than Pebble's 32-bit varargs. Convert each size explicitly.
#define dict_calc_buffer_size(count, ...) \
  test_dict_calc_buffer_size(count, (const uint32_t[]){__VA_ARGS__})
uint32_t test_dict_calc_buffer_size(
    uint32_t tuple_count,
    const uint32_t* sizes);
DictionaryResult dict_write_uint8(
    DictionaryIterator* iter,
    uint32_t key,
    uint8_t value);
AppMessageResult app_message_open(
    uint32_t inbox_size,
    uint32_t outbox_size);
AppMessageResult app_message_outbox_begin(DictionaryIterator** iterator);
AppMessageResult app_message_outbox_send(void);
bool connection_service_peek_pebble_app_connection(void);
bool health_service_set_heart_rate_sample_period(uint16_t interval_sec);

typedef void* GFont;
