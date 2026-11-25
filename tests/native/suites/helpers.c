#include <string.h>

#include "c/ataglance_message_parser.h"
#include "modules/helper_computations.h"
#include "modules/substratum_computations.h"
#include "test_runner.h"

// Suite: shared value macros, tuple conversions, and icon coordinate computations.
// Clar lifecycle: none; each case builds its inputs in local variables.

// Test cases.
void test_helpers__ranges(
    void) {
  cl_assert(HELPER_VALUE_IN_RANGE(0, 0, 10));
  cl_assert(HELPER_VALUE_IN_RANGE(10, 0, 10));
  cl_assert(!HELPER_VALUE_IN_RANGE(-1, 0, 10));
  cl_assert(!HELPER_VALUE_IN_RANGE(11, 0, 10));

  cl_assert(SUBSTRATUM_VALID_DESIGN_X(0));
  cl_assert(SUBSTRATUM_VALID_DESIGN_X(27));
  cl_assert(!SUBSTRATUM_VALID_DESIGN_X(-1));
  cl_assert(!SUBSTRATUM_VALID_DESIGN_X(28));
  cl_assert(SUBSTRATUM_VALID_DESIGN_Y(0));
  cl_assert(SUBSTRATUM_VALID_DESIGN_Y(27));
  cl_assert(!SUBSTRATUM_VALID_DESIGN_Y(-1));
  cl_assert(!SUBSTRATUM_VALID_DESIGN_Y(28));

  cl_assert_equal_i(10, HELPER_CLAMP_MIN(8, 10));
  cl_assert_equal_i(12, HELPER_CLAMP_MIN(12, 10));
  cl_assert_equal_i(8, HELPER_CLAMP_MAX(8, 10));
  cl_assert_equal_i(10, HELPER_CLAMP_MAX(12, 10));
  cl_assert_equal_i(0, HELPER_CLAMP_TO_RANGE(-1, 0, 10));
  cl_assert_equal_i(5, HELPER_CLAMP_TO_RANGE(5, 0, 10));
  cl_assert_equal_i(10, HELPER_CLAMP_TO_RANGE(11, 0, 10));
}

void test_helpers__math(
    void) {
  cl_assert_equal_i(7, HELPER_MAX(3, 7));
  cl_assert_equal_i(-4, HELPER_MAX(-4, -9));
  cl_assert_equal_i(3, HELPER_MIN(3, 7));
  cl_assert_equal_i(-9, HELPER_MIN(-4, -9));

  cl_assert_equal_i(12, HELPER_IF_ELSE(1, 12, 34));
  cl_assert_equal_i(34, HELPER_IF_ELSE(0, 12, 34));

  cl_assert_equal_i(28, HELPER_SCALE_ROUND(28, 28, 28));
  cl_assert_equal_i(9, HELPER_SCALE_ROUND(14, 18, 28));
  cl_assert_equal_i(3, HELPER_SCALE_ROUND(5, 18, 28));
  cl_assert_equal_i(-3, HELPER_SCALE_ROUND(-5, 18, 28));
  cl_assert_equal_i(3, HELPER_ROUND_UP(5, 2));
  cl_assert_equal_i(-3, HELPER_ROUND_UP(-5, 2));
}

void test_helpers__colors(
    void) {
  GColor white = {.argb = 0xFF};
  GColor same_white = {.argb = 0xFF};
  GColor black = {.argb = 0x00};
  cl_assert(HELPER_COLOR_EQUAL(white, same_white));
  cl_assert(!HELPER_COLOR_EQUAL(white, black));
}

void test_helpers__tuple_parsing(
    void) {
  uint8_t storage[sizeof(Tuple) + 8] = {0};
  Tuple* tuple = (Tuple*)storage;
  int parsed = 0;

  const struct {
    uint16_t length;
    uint8_t bytes[4];
    int32_t expected;
  } integer_cases[] = {
      {1, {0x00, 0xA5, 0xA5, 0xA5}, 0},
      {1, {0x01, 0xA5, 0xA5, 0xA5}, 1},
      {1, {0x7F, 0xA5, 0xA5, 0xA5}, INT8_MAX},
      {1, {0x80, 0xA5, 0xA5, 0xA5}, INT8_MIN},
      {1, {0xFF, 0xA5, 0xA5, 0xA5}, -1},
      {2, {0x00, 0x00, 0xA5, 0xA5}, 0},
      {2, {0x01, 0x00, 0xA5, 0xA5}, 1},
      {2, {0xFF, 0x7F, 0xA5, 0xA5}, INT16_MAX},
      {2, {0x00, 0x80, 0xA5, 0xA5}, INT16_MIN},
      {2, {0xFF, 0xFF, 0xA5, 0xA5}, -1},
      {4, {0x00, 0x00, 0x00, 0x00}, 0},
      {4, {0x01, 0x00, 0x00, 0x00}, 1},
      {4, {0xFF, 0xFF, 0xFF, 0x7F}, INT32_MAX},
      {4, {0x00, 0x00, 0x00, 0x80}, INT32_MIN},
      {4, {0xFF, 0xFF, 0xFF, 0xFF}, -1},
  };

  // Nonzero bytes after short values expose reads beyond the declared width.
  tuple->type = TUPLE_INT;
  for (size_t i = 0; i < ARRAY_LENGTH(integer_cases); ++i) {
    test_context("integer_cases row=%zu", i);
    tuple->length = integer_cases[i].length;
    memcpy(tuple->value, integer_cases[i].bytes, sizeof(integer_cases[i].bytes));
    cl_assert(tuple_to_int(tuple, &parsed));
    cl_assert_equal_i(integer_cases[i].expected, parsed);
  }

  const uint16_t invalid_lengths[] = {0, 3, 5};
  for (size_t i = 0; i < ARRAY_LENGTH(invalid_lengths); ++i) {
    test_context("invalid_lengths row=%zu", i);
    tuple->length = invalid_lengths[i];
    parsed = 1234;
    cl_assert(!tuple_to_int(tuple, &parsed));
    cl_assert_equal_i(1234, parsed);
  }

  const char* text = "32767";
  tuple->type = TUPLE_CSTRING;
  tuple->length = strlen(text) + 1;
  memcpy(tuple->value->cstring, text, tuple->length);
  cl_assert(tuple_to_int(tuple, &parsed));
  cl_assert_equal_i(32767, parsed);

  const char* invalid_texts[] = {"", "-1", "12x", "32768"};
  for (size_t i = 0; i < ARRAY_LENGTH(invalid_texts); ++i) {
    test_context("invalid_texts row=%zu", i);
    tuple->length = strlen(invalid_texts[i]) + 1;
    memcpy(tuple->value->cstring, invalid_texts[i], tuple->length);
    cl_assert(!tuple_to_int(tuple, &parsed));
  }

  tuple->type = TUPLE_BYTE_ARRAY;
  cl_assert(!tuple_to_int(tuple, &parsed));
  tuple->type = TUPLE_UINT;
  cl_assert(!tuple_to_int(tuple, &parsed));
  cl_assert(!tuple_to_int(NULL, &parsed));
  cl_assert(!tuple_to_int(tuple, NULL));
}

void test_helpers__substratum_computations(
    void) {
  GSize reference = {.w = 28, .h = 28};
  GSize compact = {.w = 18, .h = 18};
  GSize tall = {.w = 18, .h = 24};

  cl_assert_equal_i(0, substratum_renderer_scale_icon_x(&reference, 0));
  cl_assert_equal_i(27, substratum_renderer_scale_icon_x(&reference, 27));
  cl_assert_equal_i(9, substratum_renderer_scale_icon_x(&compact, 14));
  cl_assert_equal_i(17, substratum_renderer_scale_icon_x(&compact, 27));
  cl_assert_equal_i(0, substratum_renderer_scale_icon_x(&compact, -1));
  cl_assert_equal_i(0, substratum_renderer_scale_icon_x(&compact, 28));
  cl_assert_equal_i(0, substratum_renderer_scale_icon_x(NULL, 14));

  cl_assert_equal_i(0, substratum_renderer_scale_icon_y(&reference, 0));
  cl_assert_equal_i(27, substratum_renderer_scale_icon_y(&reference, 27));
  cl_assert_equal_i(9, substratum_renderer_scale_icon_y(&compact, 14));
  cl_assert_equal_i(17, substratum_renderer_scale_icon_y(&compact, 27));
  cl_assert_equal_i(0, substratum_renderer_scale_icon_y(&compact, -1));
  cl_assert_equal_i(0, substratum_renderer_scale_icon_y(&compact, 28));
  cl_assert_equal_i(0, substratum_renderer_scale_icon_y(NULL, 14));

  cl_assert_equal_i(9, substratum_renderer_scale_icon_coord(&compact, 14));
  cl_assert_equal_i(17, substratum_renderer_scale_icon_coord(&compact, 27));
  cl_assert_equal_i(9, substratum_renderer_scale_icon_coord(&tall, 14));
  cl_assert_equal_i(27, substratum_renderer_scale_icon_coord(&reference, 27));
  cl_assert_equal_i(0, substratum_renderer_scale_icon_coord(&compact, -1));
  cl_assert_equal_i(0, substratum_renderer_scale_icon_coord(&compact, 28));
  cl_assert_equal_i(0, substratum_renderer_scale_icon_coord(NULL, 14));

  GPoint point = substratum_renderer_scale_icon_point(&compact, 14, 14);
  cl_assert(point.x == 9 && point.y == 9);
  point = substratum_renderer_scale_icon_point(NULL, 14, 14);
  cl_assert(point.x == 14 && point.y == 14);
  point = substratum_renderer_scale_icon_point(&compact, -1, 14);
  cl_assert(point.x == 0 && point.y == 0);
  point = substratum_renderer_scale_icon_point(&compact, 14, 28);
  cl_assert(point.x == 0 && point.y == 0);

  GRect frame = GRect(100, 130, 18, 18);
  cl_assert_equal_i(100, substratum_renderer_scale_icon_x_in_frame(&frame, 0));
  cl_assert_equal_i(117, substratum_renderer_scale_icon_x_in_frame(&frame, 27));
  cl_assert_equal_i(100, substratum_renderer_scale_icon_x_in_frame(&frame, -1));
  cl_assert_equal_i(0, substratum_renderer_scale_icon_x_in_frame(NULL, 14));
  cl_assert_equal_i(130, substratum_renderer_scale_icon_y_in_frame(&frame, 0));
  cl_assert_equal_i(147, substratum_renderer_scale_icon_y_in_frame(&frame, 27));
  cl_assert_equal_i(130, substratum_renderer_scale_icon_y_in_frame(&frame, 28));
  cl_assert_equal_i(0, substratum_renderer_scale_icon_y_in_frame(NULL, 14));
  cl_assert_equal_i(135, substratum_renderer_scale_icon_y_in_frame(&frame, 7));
  cl_assert_equal_i(105, substratum_renderer_scale_icon_x_in_frame(&frame, 7));
  cl_assert_equal_i(139, substratum_renderer_scale_icon_y_in_frame(&frame, 14));
  cl_assert_equal_i(109, substratum_renderer_scale_icon_x_in_frame(&frame, 14));

  point = GPoint(14, 14);
  substratum_renderer_scale_icon_point_in_frame(&frame, &point);
  cl_assert(point.x == 109 && point.y == 139);
  point = GPoint(14, 14);
  substratum_renderer_scale_icon_point_in_frame(NULL, &point);
  cl_assert(point.x == 0 && point.y == 0);
  point = GPoint(7, 14);
  substratum_renderer_scale_icon_point_in_frame(&frame, &point);
  cl_assert(point.x == 105 && point.y == 139);
  point = GPoint(7, 7);
  substratum_renderer_scale_icon_point_in_frame(&frame, &point);
  cl_assert(point.x == 105 && point.y == 135);

  GRect subframe = {0};
  substratum_renderer_create_subframe(&frame, &subframe, 0, 0, 28, 28);
  cl_assert(subframe.origin.x == 100 && subframe.origin.y == 130);
  cl_assert(subframe.size.w == 18 && subframe.size.h == 18);

  // Compact sleet cloud: design 20x12 becomes 13x8 in an 18x18 icon.
  substratum_renderer_create_subframe(&frame, &subframe, 3, 0, 20, 12);
  cl_assert(subframe.origin.x == 102 && subframe.origin.y == 130);
  cl_assert(subframe.size.w == 13 && subframe.size.h == 8);
  point = substratum_renderer_scale_icon_x_y_in_frame(&subframe, 15, 18);
  cl_assert(point.x == 109 && point.y == 135);
  cl_assert_equal_i(2, substratum_renderer_scale_icon_coord(&subframe.size, 8));

  // Compact sleet and snow subframes: design 13x13 and 12x12 both become 8x8.
  substratum_renderer_create_subframe(&frame, &subframe, 14, 12, 13, 13);
  cl_assert(subframe.origin.x == 109 && subframe.origin.y == 138);
  cl_assert(subframe.size.w == 8 && subframe.size.h == 8);
  point = substratum_renderer_scale_icon_x_y_in_frame(&subframe, 4, 1);
  cl_assert(point.x == 110 && point.y == 138);
  point = substratum_renderer_scale_icon_x_y_in_frame(&subframe, 1, 26);
  cl_assert(point.x == 109 && point.y == 145);
  cl_assert_equal_i(1, substratum_renderer_scale_icon_coord(&subframe.size, 5));

  substratum_renderer_create_subframe(&frame, &subframe, 0, 14, 12, 12);
  cl_assert(subframe.origin.x == 100 && subframe.origin.y == 139);
  cl_assert(subframe.size.w == 8 && subframe.size.h == 8);
  point = substratum_renderer_scale_icon_x_y_in_frame(&subframe, 14, 14);
  cl_assert(point.x == 104 && point.y == 143);

  GRect full_frame = GRect(100, 130, 28, 28);
  cl_assert_equal_i(100, substratum_renderer_scale_icon_x_in_frame(&full_frame, 0));
  cl_assert_equal_i(127, substratum_renderer_scale_icon_x_in_frame(&full_frame, 27));
  cl_assert_equal_i(100, substratum_renderer_scale_icon_x_in_frame(&full_frame, -1));
  cl_assert_equal_i(130, substratum_renderer_scale_icon_y_in_frame(&full_frame, 0));
  cl_assert_equal_i(157, substratum_renderer_scale_icon_y_in_frame(&full_frame, 27));
  cl_assert_equal_i(130, substratum_renderer_scale_icon_y_in_frame(&full_frame, 28));
  cl_assert_equal_i(137, substratum_renderer_scale_icon_y_in_frame(&full_frame, 7));
  cl_assert_equal_i(107, substratum_renderer_scale_icon_x_in_frame(&full_frame, 7));
  cl_assert_equal_i(144, substratum_renderer_scale_icon_y_in_frame(&full_frame, 14));
  cl_assert_equal_i(114, substratum_renderer_scale_icon_x_in_frame(&full_frame, 14));

  point = GPoint(14, 14);
  substratum_renderer_scale_icon_point_in_frame(&full_frame, &point);
  cl_assert(point.x == 114 && point.y == 144);
  point = GPoint(7, 14);
  substratum_renderer_scale_icon_point_in_frame(&full_frame, &point);
  cl_assert(point.x == 107 && point.y == 144);
  point = GPoint(7, 7);
  substratum_renderer_scale_icon_point_in_frame(&full_frame, &point);
  cl_assert(point.x == 107 && point.y == 137);

  substratum_renderer_create_subframe(&full_frame, &subframe, 0, 0, 28, 28);
  cl_assert(subframe.origin.x == 100 && subframe.origin.y == 130);
  cl_assert(subframe.size.w == 28 && subframe.size.h == 28);
  substratum_renderer_create_subframe(&full_frame, &subframe, 3, 0, 20, 12);
  cl_assert(subframe.origin.x == 103 && subframe.origin.y == 130);
  cl_assert(subframe.size.w == 20 && subframe.size.h == 12);
  point = substratum_renderer_scale_icon_x_y_in_frame(&subframe, 15, 18);
  cl_assert(point.x == 114 && point.y == 138);
  cl_assert_equal_i(3, substratum_renderer_scale_icon_coord(&subframe.size, 8));
  substratum_renderer_create_subframe(&full_frame, &subframe, 14, 12, 13, 13);
  cl_assert(subframe.origin.x == 114 && subframe.origin.y == 142);
  cl_assert(subframe.size.w == 13 && subframe.size.h == 13);
  point = substratum_renderer_scale_icon_x_y_in_frame(&subframe, 4, 1);
  cl_assert(point.x == 116 && point.y == 142);
  point = substratum_renderer_scale_icon_x_y_in_frame(&subframe, 1, 26);
  cl_assert(point.x == 114 && point.y == 154);
  cl_assert_equal_i(2, substratum_renderer_scale_icon_coord(&subframe.size, 5));
  substratum_renderer_create_subframe(&full_frame, &subframe, 0, 14, 12, 12);
  cl_assert(subframe.origin.x == 100 && subframe.origin.y == 144);
  cl_assert(subframe.size.w == 12 && subframe.size.h == 12);
  point = substratum_renderer_scale_icon_x_y_in_frame(&subframe, 14, 14);
  cl_assert(point.x == 106 && point.y == 150);

  GRect edge_frame = GRect(PBL_DISPLAY_WIDTH - 5, PBL_DISPLAY_HEIGHT - 5, 18, 18);
  cl_assert_equal_i(
      (PBL_DISPLAY_WIDTH - 1),
      substratum_renderer_scale_icon_x_in_frame(&edge_frame, 27));
  cl_assert_equal_i(
      (PBL_DISPLAY_HEIGHT - 1),
      substratum_renderer_scale_icon_y_in_frame(&edge_frame, 27));
  GRect unchanged = GRect(1, 2, 3, 4);
  substratum_renderer_create_subframe(NULL, &unchanged, 0, 0, 1, 1);
  cl_assert(unchanged.origin.x == 1 && unchanged.origin.y == 2);
  cl_assert(unchanged.size.w == 3 && unchanged.size.h == 4);
  substratum_renderer_create_subframe(&frame, NULL, 0, 0, 1, 1);
}
