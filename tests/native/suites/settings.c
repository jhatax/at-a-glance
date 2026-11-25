#include "modules/settings.h"

#include <limits.h>
#include <string.h>

#include "test_mocks.h"
#include "test_runner.h"

// Suite: settings defaults, persistence migration, validation, and setters.
enum { LEGACY_KEY = 2, CURRENT_KEY = 3 };

// Frozen little-endian Pebble records: Celsius, dark theme, 60-minute weather,
// vertical battery; health records also contain 30-minute HR and 20,000 steps.
// Key 2 contains the removed 24-hour time-format byte after temp_unit.
#ifdef PBL_HEALTH
static const uint8_t legacy_record[] = {1, 0, 1, 60, 30, 0, 0x20, 0x4e, 1, 0};
static const uint8_t current_record[] = {1, 1, 60, 30, 0x20, 0x4e, 1, 0};
#else
static const uint8_t legacy_record[] = {1, 0, 1, 60, 1};
static const uint8_t current_record[] = {1, 1, 60, 1};
#endif

_Static_assert(
    sizeof(SupportedDisplayModes) == 1,
    "Match Pebble enum storage");
_Static_assert(
    sizeof(WatchfaceSettings) == sizeof(current_record),
    "Match Pebble settings storage");

// Clar lifecycle: none; cases reset their dependencies before use.
// Suite helpers: called explicitly by test cases.
static void check_saved_values(
    const WatchfaceSettings* settings) {
  cl_assert_equal_i(TEMP_UNIT_C, settings->temp_unit);
  cl_assert_equal_i(DISPLAY_MODE_DARK_MONOCHROME, settings->display_mode);
  cl_assert_equal_i(60, settings->weather_update_minutes);
  cl_assert_equal_i(BATTERY_ORIENTATION_VERTICAL, settings->battery_orientation);
#ifdef PBL_HEALTH
  cl_assert_equal_i(30, settings->hr_sample_minutes);
  cl_assert_equal_i(20000, settings->steps_goal);
#endif
}

static void check_defaults(
    const WatchfaceSettings* settings) {
  cl_assert_equal_i(TEMP_UNIT_DEFAULT, settings->temp_unit);
  cl_assert_equal_i(DISPLAY_MODE_DEFAULT, settings->display_mode);
  cl_assert_equal_i(WEATHER_UPDATE_MINUTES_DEFAULT, settings->weather_update_minutes);
  cl_assert_equal_i(BATTERY_ORIENTATION_DEFAULT, settings->battery_orientation);
#ifdef PBL_HEALTH
  cl_assert_equal_i(HR_SAMPLE_MINUTES_DEFAULT, settings->hr_sample_minutes);
  cl_assert_equal_i(STEPS_GOAL_DEFAULT, settings->steps_goal);
#endif
}

// Test cases.
void test_settings__defaults(
    void) {
  test_storage_reset();
  WatchfaceSettings settings;
  memset(&settings, 0xff, sizeof(settings));
  settings_load(&settings);
  check_defaults(&settings);
  cl_assert(test_storage_write_calls == 0 && test_storage_delete_calls == 0);
}

void test_settings__current_load(
    void) {
  test_storage_reset();
  test_storage_seed(CURRENT_KEY, current_record, sizeof(current_record));
  WatchfaceSettings settings;
  settings_load(&settings);
  check_saved_values(&settings);
  cl_assert(test_storage_write_calls == 0 && test_storage_delete_calls == 0);
}

void test_settings__legacy_migration(
    void) {
  test_storage_reset();
  test_storage_seed(LEGACY_KEY, legacy_record, sizeof(legacy_record));
  WatchfaceSettings settings;
  settings_load(&settings);
  check_saved_values(&settings);
  cl_assert(!persist_exists(LEGACY_KEY));
  cl_assert_equal_i((int)sizeof(current_record), persist_get_size(CURRENT_KEY));
  cl_assert_equal_i(1, test_storage_write_calls);
  // A second launch must use the new record without another migration write.
  memset(&settings, 0xff, sizeof(settings));
  settings_load(&settings);
  check_saved_values(&settings);
  cl_assert_equal_i(1, test_storage_write_calls);
}

void test_settings__current_takes_precedence(
    void) {
  test_storage_reset();
  uint8_t old[sizeof(legacy_record)];
  memcpy(old, legacy_record, sizeof(old));
  old[0] = TEMP_UNIT_F;
  test_storage_seed(LEGACY_KEY, old, sizeof(old));
  test_storage_seed(CURRENT_KEY, current_record, sizeof(current_record));
  WatchfaceSettings settings;
  settings_load(&settings);
  check_saved_values(&settings);
  cl_assert(!persist_exists(LEGACY_KEY));
  cl_assert_equal_i(0, test_storage_write_calls);
}

void test_settings__legacy_read_failure(
    void) {
  const int failures[] = {-1, (int)sizeof(legacy_record) - 1};
  for (size_t i = 0; i < ARRAY_LENGTH(failures); ++i) {
    test_context("failures row=%zu", i);
    test_storage_reset();
    test_storage_seed(LEGACY_KEY, legacy_record, sizeof(legacy_record));
    test_storage_read_result = failures[i];
    WatchfaceSettings settings;
    settings_load(&settings);
    check_defaults(&settings);
    cl_assert(!persist_exists(LEGACY_KEY) && !persist_exists(CURRENT_KEY));
    cl_assert_equal_i(0, test_storage_write_calls);
  }
}

void test_settings__migration_write_failure(
    void) {
  const int failures[] = {-1, (int)sizeof(current_record) - 1};
  for (size_t i = 0; i < ARRAY_LENGTH(failures); ++i) {
    test_context("failures row=%zu", i);
    test_storage_reset();
    test_storage_seed(LEGACY_KEY, legacy_record, sizeof(legacy_record));
    test_storage_write_result = failures[i];
    WatchfaceSettings settings;
    settings_load(&settings);
    check_saved_values(&settings);
    cl_assert(persist_exists(LEGACY_KEY) && !persist_exists(CURRENT_KEY));
    cl_assert(memcmp(test_storage_bytes(LEGACY_KEY), legacy_record, sizeof(legacy_record)) == 0);
    test_storage_write_result = NORMAL_IO;
    settings_load(&settings);
    check_saved_values(&settings);
    cl_assert(!persist_exists(LEGACY_KEY) && persist_exists(CURRENT_KEY));
  }
}

void test_settings__current_read_failure(
    void) {
  const int failures[] = {-1, (int)sizeof(current_record) - 1};
  for (size_t i = 0; i < ARRAY_LENGTH(failures); ++i) {
    test_context("failures row=%zu", i);
    test_storage_reset();
    test_storage_seed(CURRENT_KEY, current_record, sizeof(current_record));
    test_storage_read_result = failures[i];
    WatchfaceSettings settings;
    settings_load(&settings);
    cl_assert(!persist_exists(CURRENT_KEY));
    cl_assert_equal_i(0, test_storage_write_calls);
    test_storage_read_result = NORMAL_IO;
    settings_load(&settings);
    check_defaults(&settings);
  }
}

void test_settings__save_reload(
    void) {
  test_storage_reset();
  WatchfaceSettings settings;
  settings_load(&settings);
  cl_assert(settings_set_temp_unit(&settings, TEMP_UNIT_C));
  cl_assert(settings_set_display_mode(&settings, DISPLAY_MODE_DARK_MONOCHROME));
  cl_assert(settings_set_weather_update_minutes(&settings, 60));
  cl_assert(settings_set_battery_orientation(&settings, BATTERY_ORIENTATION_VERTICAL));
#ifdef PBL_HEALTH
  cl_assert(settings_set_hr_sample_minutes(&settings, 30));
  cl_assert(settings_set_steps_goal(&settings, 20000));
#endif
  cl_assert(settings_save(&settings));
  cl_assert(!persist_exists(LEGACY_KEY));
  cl_assert_equal_i((int)sizeof(current_record), persist_get_size(CURRENT_KEY));
  memset(&settings, 0, sizeof(settings));
  settings_load(&settings);
  check_saved_values(&settings);
  cl_assert_equal_i(1, test_storage_write_calls);
}

void test_settings__invalid_stored_values(
    void) {
  test_storage_reset();
  WatchfaceSettings invalid;
  memset(&invalid, 0xff, sizeof(invalid));
  invalid.weather_update_minutes = 15;
  test_storage_seed(CURRENT_KEY, (const uint8_t*)&invalid, sizeof(invalid));
  WatchfaceSettings settings;
  settings_load(&settings);
  check_defaults(&settings);
  cl_assert(test_storage_write_calls == 0 && test_storage_delete_calls == 0);
  cl_assert(memcmp(test_storage_bytes(CURRENT_KEY), &invalid, sizeof(invalid)) == 0);
}

void test_settings__weather_interval_sanitization(
    void) {
  const struct {
    uint8_t stored;
    uint8_t expected;
  } cases[] = {
      {0, 15},
      {14, 15},
      {15, 15},
      {16, 15},
      {29, 15},
      {30, 30},
      {44, 30},
      {45, 45},
      {59, 45},
      {60, 60},
      {61, 60},
      {255, 60},
  };
  for (size_t i = 0; i < ARRAY_LENGTH(cases); ++i) {
    test_context("cases row=%zu", i);
    test_storage_reset();
    uint8_t record[sizeof(current_record)];
    memcpy(record, current_record, sizeof(record));
    record[2] = cases[i].stored;
    test_storage_seed(CURRENT_KEY, record, sizeof(record));
    WatchfaceSettings settings;
    settings_load(&settings);
    cl_assert_equal_i(cases[i].expected, settings.weather_update_minutes);
    cl_assert_equal_i(TEMP_UNIT_C, settings.temp_unit);
    cl_assert_equal_i(BATTERY_ORIENTATION_VERTICAL, settings.battery_orientation);
    cl_assert(test_storage_write_calls == 0 && test_storage_delete_calls == 0);
  }
}

void test_settings__invalid_record_size(
    void) {
  const size_t sizes[] = {1, sizeof(current_record) - 1, sizeof(current_record) + 1};
  const uint8_t record[sizeof(current_record) + 1] = {0};
  for (size_t i = 0; i < ARRAY_LENGTH(sizes); ++i) {
    test_context("sizes row=%zu", i);
    test_storage_reset();
    test_storage_seed(CURRENT_KEY, record, sizes[i]);
    WatchfaceSettings settings;
    settings_load(&settings);
    check_defaults(&settings);
    cl_assert(!persist_exists(CURRENT_KEY));
    cl_assert_equal_i(0, test_storage_write_calls);
  }
}

void test_settings__setter_boundaries(
    void) {
  test_storage_reset();
  WatchfaceSettings settings;
  settings_load(&settings);
  const int weather_intervals[] = {15, 30, 45, 60};
  for (size_t i = 0; i < ARRAY_LENGTH(weather_intervals); ++i) {
    test_context("weather_intervals row=%zu", i);
    cl_assert(settings_set_weather_update_minutes(&settings, weather_intervals[i]));
    cl_assert_equal_i(weather_intervals[i], settings.weather_update_minutes);
  }
  for (int value = TEMP_UNIT_MIN; value <= TEMP_UNIT_MAX; ++value) {
    cl_assert(settings_set_temp_unit(&settings, value));
    cl_assert_equal_i(value, settings.temp_unit);
  }
  for (int value = DISPLAY_MODE_MIN; value <= DISPLAY_MODE_MAX; ++value) {
    cl_assert(settings_set_display_mode(&settings, value));
    cl_assert_equal_i(value, settings.display_mode);
  }
  for (int value = BATTERY_ORIENTATION_MIN; value <= BATTERY_ORIENTATION_MAX; ++value) {
    cl_assert(settings_set_battery_orientation(&settings, value));
    cl_assert_equal_i(value, settings.battery_orientation);
  }
#ifdef PBL_HEALTH
  cl_assert(settings_set_hr_sample_minutes(&settings, 10));
  cl_assert_equal_i(10, settings.hr_sample_minutes);
  cl_assert(settings_set_hr_sample_minutes(&settings, 120));
  cl_assert_equal_i(120, settings.hr_sample_minutes);
  cl_assert(settings_set_steps_goal(&settings, 4000));
  cl_assert_equal_i(4000, settings.steps_goal);
  cl_assert(settings_set_steps_goal(&settings, 32000));
  cl_assert_equal_i(32000, settings.steps_goal);
#endif
  cl_assert(test_storage_write_calls == 0 && test_storage_delete_calls == 0);
}

void test_settings__rejected_updates(
    void) {
  const struct {
    bool (*set)(
        WatchfaceSettings*,
        int);
    int invalid;
  } cases[] = {
      {settings_set_temp_unit, 2},
      {settings_set_display_mode, DISPLAY_MODE_MAX + 1},
      {settings_set_battery_orientation, 2},
      {settings_set_weather_update_minutes, 0},
      {settings_set_weather_update_minutes, 14},
      {settings_set_weather_update_minutes, 16},
      {settings_set_weather_update_minutes, 59},
      {settings_set_weather_update_minutes, 61},
#ifdef PBL_HEALTH
      {settings_set_hr_sample_minutes, 9},
      {settings_set_hr_sample_minutes, 121},
      {settings_set_steps_goal, 3999},
      {settings_set_steps_goal, 32001},
#endif
  };
  test_storage_reset();
  test_storage_seed(CURRENT_KEY, current_record, sizeof(current_record));
  WatchfaceSettings settings;
  settings_load(&settings);
  uint8_t before[sizeof(settings)];
  memcpy(before, &settings, sizeof(before));
  for (size_t i = 0; i < ARRAY_LENGTH(cases); ++i) {
    test_context("cases row=%zu", i);
    const int invalid[] = {cases[i].invalid, -1, INT_MIN, INT_MAX};
    for (size_t j = 0; j < ARRAY_LENGTH(invalid); ++j) {
      test_context("setter row=%zu invalid=%d", i, invalid[j]);
      cl_assert(!cases[i].set(&settings, invalid[j]));
      cl_assert(memcmp(&settings, before, sizeof(before)) == 0);
    }
    cl_assert(!cases[i].set(NULL, 0));
  }
  cl_assert(test_storage_write_calls == 0 && test_storage_delete_calls == 0);
}
