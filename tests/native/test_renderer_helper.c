#include "../src/modules/helper.h"
#include "../src/modules/substratum_computations.h"
#include "test_framework.h"

int test_helper_ranges(
    void) {
  TEST_CHECK(HELPER_VALUE_IN_RANGE(0, 0, 10));
  TEST_CHECK(HELPER_VALUE_IN_RANGE(10, 0, 10));
  TEST_CHECK(!HELPER_VALUE_IN_RANGE(-1, 0, 10));
  TEST_CHECK(!HELPER_VALUE_IN_RANGE(11, 0, 10));

  TEST_CHECK(SUBSTRATUM_VALID_DESIGN_X(0));
  TEST_CHECK(SUBSTRATUM_VALID_DESIGN_X(27));
  TEST_CHECK(!SUBSTRATUM_VALID_DESIGN_X(-1));
  TEST_CHECK(!SUBSTRATUM_VALID_DESIGN_X(28));
  TEST_CHECK(SUBSTRATUM_VALID_DESIGN_Y(0));
  TEST_CHECK(SUBSTRATUM_VALID_DESIGN_Y(27));
  TEST_CHECK(!SUBSTRATUM_VALID_DESIGN_Y(-1));
  TEST_CHECK(!SUBSTRATUM_VALID_DESIGN_Y(28));

  TEST_CHECK(HELPER_CLAMP_MIN(8, 10) == 10);
  TEST_CHECK(HELPER_CLAMP_MIN(12, 10) == 12);
  TEST_CHECK(HELPER_CLAMP_MAX(8, 10) == 8);
  TEST_CHECK(HELPER_CLAMP_MAX(12, 10) == 10);
  TEST_CHECK(HELPER_CLAMP_TO_RANGE(-1, 0, 10) == 0);
  TEST_CHECK(HELPER_CLAMP_TO_RANGE(5, 0, 10) == 5);
  TEST_CHECK(HELPER_CLAMP_TO_RANGE(11, 0, 10) == 10);

  return 0;
}

int test_helper_math(
    void) {
  TEST_CHECK(HELPER_MAX(3, 7) == 7);
  TEST_CHECK(HELPER_MAX(-4, -9) == -4);
  TEST_CHECK(HELPER_MIN(3, 7) == 3);
  TEST_CHECK(HELPER_MIN(-4, -9) == -9);

  TEST_CHECK(HELPER_IF_ELSE(1, 12, 34) == 12);
  TEST_CHECK(HELPER_IF_ELSE(0, 12, 34) == 34);

  TEST_CHECK(HELPER_SCALE_ROUND(28, 28, 28) == 28);
  TEST_CHECK(HELPER_SCALE_ROUND(14, 18, 28) == 9);
  TEST_CHECK(HELPER_SCALE_ROUND(5, 18, 28) == 3);
  TEST_CHECK(HELPER_SCALE_ROUND(-5, 18, 28) == -3);
  TEST_CHECK(HELPER_ROUND_UP(5, 2) == 3);
  TEST_CHECK(HELPER_ROUND_UP(-5, 2) == -3);

  return 0;
}

int test_helper_colors(
    void) {
  GColor white = {.argb = 0xFF};
  GColor same_white = {.argb = 0xFF};
  GColor black = {.argb = 0x00};
  TEST_CHECK(HELPER_COLOR_EQUAL(white, same_white));
  TEST_CHECK(!HELPER_COLOR_EQUAL(white, black));

  return 0;
}

int test_helper_tuple_parsing(
    void) {
  TupleValue integer_value = {.int32 = 1234};
  Tuple integer_tuple = {.type = TUPLE_INT, .value = &integer_value};
  int parsed = 0;

  TEST_CHECK(helper_tuple_to_int(&integer_tuple, &parsed));
  TEST_CHECK(parsed == 1234);

  const char* text = "32767";
  TupleValue string_value = {.cstring = text};
  Tuple string_tuple = {.type = TUPLE_CSTRING, .value = &string_value};
  TEST_CHECK(helper_tuple_to_int(&string_tuple, &parsed));
  TEST_CHECK(parsed == 32767);

  const char* invalid_texts[] = {"", "-1", "12x", "32768"};
  for (size_t i = 0; i < ARRAY_LENGTH(invalid_texts); ++i) {
    string_value.cstring = invalid_texts[i];
    TEST_CHECK(!helper_tuple_to_int(&string_tuple, &parsed));
  }

  Tuple wrong_type = {.type = TUPLE_BYTES, .value = &integer_value};
  TEST_CHECK(!helper_tuple_to_int(&wrong_type, &parsed));
  TEST_CHECK(!helper_tuple_to_int(NULL, &parsed));
  TEST_CHECK(!helper_tuple_to_int(&integer_tuple, NULL));

  return 0;
}

int test_substratum_computations(
    void) {
  GSize reference = {.w = 28, .h = 28};
  GSize compact = {.w = 18, .h = 18};
  GSize tall = {.w = 18, .h = 24};

  TEST_CHECK(substratum_renderer_scale_icon_x(&reference, 0) == 0);
  TEST_CHECK(substratum_renderer_scale_icon_x(&reference, 27) == 27);
  TEST_CHECK(substratum_renderer_scale_icon_x(&compact, 14) == 9);
  TEST_CHECK(substratum_renderer_scale_icon_x(&compact, 27) == 17);
  TEST_CHECK(substratum_renderer_scale_icon_x(&compact, -1) == 0);
  TEST_CHECK(substratum_renderer_scale_icon_x(&compact, 28) == 0);
  TEST_CHECK(substratum_renderer_scale_icon_x(NULL, 14) == 0);

  TEST_CHECK(substratum_renderer_scale_icon_y(&reference, 0) == 0);
  TEST_CHECK(substratum_renderer_scale_icon_y(&reference, 27) == 27);
  TEST_CHECK(substratum_renderer_scale_icon_y(&compact, 14) == 9);
  TEST_CHECK(substratum_renderer_scale_icon_y(&compact, 27) == 17);
  TEST_CHECK(substratum_renderer_scale_icon_y(&compact, -1) == 0);
  TEST_CHECK(substratum_renderer_scale_icon_y(&compact, 28) == 0);
  TEST_CHECK(substratum_renderer_scale_icon_y(NULL, 14) == 0);

  TEST_CHECK(substratum_renderer_scale_icon_coord(&compact, 14) == 9);
  TEST_CHECK(substratum_renderer_scale_icon_coord(&compact, 27) == 17);
  TEST_CHECK(substratum_renderer_scale_icon_coord(&tall, 14) == 9);
  TEST_CHECK(substratum_renderer_scale_icon_coord(&reference, 27) == 27);
  TEST_CHECK(substratum_renderer_scale_icon_coord(&compact, -1) == 0);
  TEST_CHECK(substratum_renderer_scale_icon_coord(&compact, 28) == 0);
  TEST_CHECK(substratum_renderer_scale_icon_coord(NULL, 14) == 0);

  GPoint point = substratum_renderer_scale_icon_point(&compact, 14, 14);
  TEST_CHECK(point.x == 9 && point.y == 9);
  point = substratum_renderer_scale_icon_point(NULL, 14, 14);
  TEST_CHECK(point.x == 14 && point.y == 14);
  point = substratum_renderer_scale_icon_point(&compact, -1, 14);
  TEST_CHECK(point.x == 0 && point.y == 0);
  point = substratum_renderer_scale_icon_point(&compact, 14, 28);
  TEST_CHECK(point.x == 0 && point.y == 0);

  GRect frame = GRect(10, 20, 18, 18);
  TEST_CHECK(substratum_renderer_scale_icon_x_in_frame(&frame, 0) == 10);
  TEST_CHECK(substratum_renderer_scale_icon_x_in_frame(&frame, 27) == 27);
  TEST_CHECK(substratum_renderer_scale_icon_x_in_frame(&frame, -1) == 10);
  TEST_CHECK(substratum_renderer_scale_icon_x_in_frame(NULL, 14) == 0);
  TEST_CHECK(substratum_renderer_scale_icon_y_in_frame(&frame, 0) == 20);
  TEST_CHECK(substratum_renderer_scale_icon_y_in_frame(&frame, 27) == 37);
  TEST_CHECK(substratum_renderer_scale_icon_y_in_frame(&frame, 28) == 20);
  TEST_CHECK(substratum_renderer_scale_icon_y_in_frame(NULL, 14) == 0);

  point = GPoint(14, 14);
  substratum_renderer_scale_icon_point_in_frame(&frame, &point);
  TEST_CHECK(point.x == 19 && point.y == 29);
  point = GPoint(14, 14);
  substratum_renderer_scale_icon_point_in_frame(NULL, &point);
  TEST_CHECK(point.x == 0 && point.y == 0);

  GRect subframe = {0};
  substratum_renderer_create_subframe(&frame, &subframe, 0, 0, 28, 28);
  TEST_CHECK(subframe.origin.x == 10 && subframe.origin.y == 20);
  TEST_CHECK(subframe.size.w == 18 && subframe.size.h == 18);

  // Compact sleet cloud: design 20x12 becomes 13x8 in an 18x18 icon.
  substratum_renderer_create_subframe(&frame, &subframe, 3, 0, 20, 12);
  TEST_CHECK(subframe.origin.x == 12 && subframe.origin.y == 20);
  TEST_CHECK(subframe.size.w == 13 && subframe.size.h == 8);
  point = substratum_renderer_scale_icon_x_y_in_frame(&subframe, 15, 18);
  TEST_CHECK(point.x == 19 && point.y == 25);
  TEST_CHECK(substratum_renderer_scale_icon_coord(&subframe.size, 8) == 2);

  // Compact sleet and snow subframes: design 13x13 and 12x12 both become 8x8.
  substratum_renderer_create_subframe(&frame, &subframe, 14, 12, 13, 13);
  TEST_CHECK(subframe.origin.x == 19 && subframe.origin.y == 28);
  TEST_CHECK(subframe.size.w == 8 && subframe.size.h == 8);
  point = substratum_renderer_scale_icon_x_y_in_frame(&subframe, 4, 1);
  TEST_CHECK(point.x == 20 && point.y == 28);
  point = substratum_renderer_scale_icon_x_y_in_frame(&subframe, 1, 26);
  TEST_CHECK(point.x == 19 && point.y == 35);
  TEST_CHECK(substratum_renderer_scale_icon_coord(&subframe.size, 5) == 1);

  substratum_renderer_create_subframe(&frame, &subframe, 0, 14, 12, 12);
  TEST_CHECK(subframe.origin.x == 10 && subframe.origin.y == 29);
  TEST_CHECK(subframe.size.w == 8 && subframe.size.h == 8);
  point = substratum_renderer_scale_icon_x_y_in_frame(&subframe, 14, 14);
  TEST_CHECK(point.x == 14 && point.y == 33);
  GRect unchanged = GRect(1, 2, 3, 4);
  substratum_renderer_create_subframe(NULL, &unchanged, 0, 0, 1, 1);
  TEST_CHECK(unchanged.origin.x == 1 && unchanged.origin.y == 2);
  TEST_CHECK(unchanged.size.w == 3 && unchanged.size.h == 4);
  substratum_renderer_create_subframe(&frame, NULL, 0, 0, 1, 1);

  return 0;
}
