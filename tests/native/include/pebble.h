#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

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
  union {
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

#define GColorClear ((GColor){.argb = 0})
#define GColorWhite ((GColor){.argb = 0xFF})

#define PBL_DISPLAY_WIDTH 144
#define PBL_DISPLAY_HEIGHT 168

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

#define PBL_IF_ROUND_ELSE(round_value, rectangular_value) (rectangular_value)

typedef void* GFont;

typedef void GBitmap;

typedef enum {
  GBitmapFormat1BitPalette,
  GBitmapFormat2BitPalette,
  GBitmapFormat4BitPalette,
} GBitmapFormat;

GBitmapFormat gbitmap_get_format(GBitmap* bitmap);
GColor* gbitmap_get_palette(GBitmap* bitmap);
