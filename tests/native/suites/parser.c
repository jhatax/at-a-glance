#include "c/ataglance_message_parser.h"
#include "test_mocks.h"
#include "test_runner.h"

// Suite: interpret settings, weather, and location tuples and their presence masks.
// Clar lifecycle: none; each case builds its inputs in local variables.

// Test cases.
void test_parser__received_and_parsed(
    void) {
  DictionaryIterator iter = {0};
  TestTuple storage[3];
  test_dict_int(&iter, &storage[0], MESSAGE_KEY_TEMP_UNIT, 1);
  test_dict_string(&iter, &storage[1], MESSAGE_KEY_DISPLAY_MODE, "bad");
  test_dict_int(&iter, &storage[2], 999, 123);
  WatchfaceEventData data = {.display_mode = 73};
  parse_settings_data(&iter, &data);
  cl_assert_equal_i((WATCHFACE_DATA_TEMP_UNIT | WATCHFACE_DATA_DISPLAY_MODE), data.received);
  cl_assert_equal_i(WATCHFACE_DATA_TEMP_UNIT, data.parsed);
  cl_assert(data.temp_unit == 1 && data.display_mode == 73);
  cl_assert(!data.settings_changed);
}

void test_parser__settings_mapping(
    void) {
  const struct {
    uint32_t key;
    WatchfaceDataMask mask;
    int value;
  } cases[] = {
      {MESSAGE_KEY_TEMP_UNIT, WATCHFACE_DATA_TEMP_UNIT, 1},
      {MESSAGE_KEY_DISPLAY_MODE, WATCHFACE_DATA_DISPLAY_MODE, 1},
      {MESSAGE_KEY_WEATHER_UPDATE_MINUTES, WATCHFACE_DATA_WEATHER_UPDATE_MINUTES, 60},
      {MESSAGE_KEY_BATTERY_ORIENTATION, WATCHFACE_DATA_BATTERY_ORIENTATION, 1},
#ifdef PBL_HEALTH
      {MESSAGE_KEY_HR_SAMPLE_MINUTES, WATCHFACE_DATA_HR_SAMPLE_MINUTES, 30},
      {MESSAGE_KEY_STEPS_GOAL, WATCHFACE_DATA_STEPS_GOAL, 20000},
#endif
  };
  DictionaryIterator iter = {0};
  TestTuple storage[ARRAY_LENGTH(cases)];
  WatchfaceDataMask expected = 0;
  for (size_t i = 0; i < ARRAY_LENGTH(cases); ++i) {
    test_context("cases row=%zu", i);
    test_dict_int(&iter, &storage[i], cases[i].key, cases[i].value);
    expected |= cases[i].mask;
  }
  WatchfaceEventData data = {0};
  parse_settings_data(&iter, &data);
  cl_assert(data.received == expected && data.parsed == expected);
  cl_assert(data.temp_unit == 1 && data.display_mode == 1);
  cl_assert(data.weather_update_minutes == 60 && data.battery_orientation == 1);
#ifdef PBL_HEALTH
  cl_assert(data.hr_sample_minutes == 30 && data.steps_goal == 20000);
#endif
  // Parsing describes input; settings validation belongs to the runtime boundary.
  data = (WatchfaceEventData){0};
  ((Tuple*)storage[0].bytes)->value->int32 = -99;
  parse_settings_data(&iter, &data);
  cl_assert(data.temp_unit == -99 && (data.parsed & WATCHFACE_DATA_TEMP_UNIT));
#ifndef PBL_HEALTH
  iter = (DictionaryIterator){0};
  data = (WatchfaceEventData){0};
  test_dict_int(&iter, &storage[0], MESSAGE_KEY_HR_SAMPLE_MINUTES, 30);
  test_dict_int(&iter, &storage[1], MESSAGE_KEY_STEPS_GOAL, 20000);
  parse_settings_data(&iter, &data);
  cl_assert(!data.received && !data.parsed);
  cl_assert(!data.hr_sample_minutes && !data.steps_goal);
#endif
}

void test_parser__weather_location(
    void) {
  DictionaryIterator iter = {0};
  TestTuple storage[4];
  test_dict_int(&iter, &storage[0], MESSAGE_KEY_TEMPERATURE, -123);
  test_dict_int(&iter, &storage[1], MESSAGE_KEY_WEATHER_CONDITION, 63);
  test_dict_int(&iter, &storage[2], MESSAGE_KEY_IS_DAY, 0);
  test_dict_string(&iter, &storage[3], MESSAGE_KEY_MAYBE_CURRENT_LOCATION, "Montréal");
  WatchfaceEventData data = {0};
  parse_weather_and_location_data(&iter, &data);
  WatchfaceDataMask expected = WATCHFACE_DATA_TEMPERATURE | WATCHFACE_DATA_WEATHER_CONDITION |
                               WATCHFACE_DATA_IS_DAY | WATCHFACE_DATA_LOCATION;
  cl_assert(data.received == expected && data.parsed == expected);
  cl_assert(data.temperature_celsius_tenths == -123 && data.weather_condition == 63);
  cl_assert(data.is_day == 0 && data.pin.received);
  cl_assert(strcmp(data.pin.name, "Montréal") == 0);
  cl_assert(!data.current_climate.received && !data.current_climate.complete);

  ((Tuple*)storage[1].bytes)->type = TUPLE_BYTE_ARRAY;
  ((Tuple*)storage[3].bytes)->type = TUPLE_INT;
  data = (WatchfaceEventData){0};
  parse_weather_and_location_data(&iter, &data);
  cl_assert_equal_i(expected, data.received);
  cl_assert_equal_i((WATCHFACE_DATA_TEMPERATURE | WATCHFACE_DATA_IS_DAY), data.parsed);
  cl_assert(data.pin.received);
}

void test_parser__string_boundaries(
    void) {
  DictionaryIterator iter = {0};
  TestTuple storage;
  const char* names[] = {
      "",
      "123456789012345678901234567890",
      "123456789012345678901234567890EXTRA"};
  for (size_t i = 0; i < ARRAY_LENGTH(names); ++i) {
    test_context("names row=%zu", i);
    iter = (DictionaryIterator){0};
    test_dict_string(&iter, &storage, MESSAGE_KEY_MAYBE_CURRENT_LOCATION, names[i]);
    struct {
      WatchfaceEventData data;
      uint8_t guard;
    } packet = {.guard = 0xA5};
    parse_weather_and_location_data(&iter, &packet.data);
    cl_assert(packet.data.pin.received);
    cl_assert_equal_i(WATCHFACE_DATA_LOCATION, packet.data.parsed);
    cl_assert(packet.data.pin.name[30] == '\0' && packet.guard == 0xA5);
    cl_assert(strncmp(packet.data.pin.name, names[i], 30) == 0);
  }
  char bounded[] = {'L', '?', 'R'};
  cl_assert(tuple_to_string((Tuple*)storage.bytes, &bounded[1], 1));
  cl_assert(bounded[0] == 'L' && bounded[1] == '\0' && bounded[2] == 'R');
}

void test_parser__missing_and_null(
    void) {
  DictionaryIterator iter = {0};
  WatchfaceEventData data = {.temperature_celsius_tenths = 73};
  parse_settings_data(NULL, &data);
  parse_settings_data(&iter, NULL);
  parse_weather_and_location_data(NULL, &data);
  parse_weather_and_location_data(&iter, NULL);
  parse_settings_data(&iter, &data);
  parse_weather_and_location_data(&iter, &data);
  cl_assert(!data.received && !data.parsed && !data.pin.received);
  cl_assert_equal_i(73, data.temperature_celsius_tenths);
  int value = 19;
  parse_int_tuple(NULL, 1, &data, WATCHFACE_DATA_TEMP_UNIT, &value);
  parse_int_tuple(&iter, 1, NULL, WATCHFACE_DATA_TEMP_UNIT, &value);
  parse_int_tuple(&iter, 1, &data, WATCHFACE_DATA_TEMP_UNIT, NULL);
  char text[] = "keep";
  parse_string_tuple(&iter, 1, &data, WATCHFACE_DATA_LOCATION, text, 0);
  cl_assert(value == 19 && strcmp(text, "keep") == 0);
  cl_assert(!parse_ready_sentinel(NULL) && !parse_ready_sentinel(&iter));
  TestTuple sentinel;
  test_dict_string(&iter, &sentinel, MESSAGE_KEY_SYNC, "invalid");
  cl_assert(!parse_ready_sentinel(&iter));
  ((Tuple*)sentinel.bytes)->type = TUPLE_INT;
  ((Tuple*)sentinel.bytes)->length = 4;
  ((Tuple*)sentinel.bytes)->value->int32 = 1;
  cl_assert(parse_ready_sentinel(&iter));
}
