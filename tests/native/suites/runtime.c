#include "test_mocks.h"
#include "test_runner.h"

// Suite: apply runtime inputs and select refresh, repaint, and relayout actions.
// Clar lifecycle: none; cases reset their dependencies before use.
// Suite helpers: called explicitly by test cases.
static WatchfaceSettings default_settings(
    void) {
  WatchfaceSettings settings = {
      .temp_unit = TEMP_UNIT_DEFAULT,
      .display_mode = DISPLAY_MODE_DEFAULT,
      .weather_update_minutes = WEATHER_UPDATE_MINUTES_DEFAULT,
      .battery_orientation = BATTERY_ORIENTATION_DEFAULT,
#ifdef PBL_HEALTH
      .hr_sample_minutes = HR_SAMPLE_MINUTES_DEFAULT,
      .steps_goal = STEPS_GOAL_DEFAULT,
#endif
  };
  return settings;
}

// Test cases.
void test_runtime__settings_dispatch(
    void) {
  const struct {
    WatchfaceDataMask field;
    WatchfaceUpdateMask updates;
  } cases[] = {
      {WATCHFACE_DATA_TEMP_UNIT, WATCHFACE_UPDATE_CLIMATE},
      {WATCHFACE_DATA_WEATHER_UPDATE_MINUTES, WATCHFACE_UPDATE_NONE},
      {WATCHFACE_DATA_BATTERY_ORIENTATION,
       WATCHFACE_UPDATE_BATTERY_ORIENTATION | WATCHFACE_UPDATE_BT_LAYOUT},
#ifdef PBL_HEALTH
      {WATCHFACE_DATA_HR_SAMPLE_MINUTES, WATCHFACE_UPDATE_NONE},
      {WATCHFACE_DATA_STEPS_GOAL, WATCHFACE_UPDATE_HEALTH},
#endif
  };
  for (size_t i = 0; i < ARRAY_LENGTH(cases); ++i) {
    test_context("cases row=%zu", i);
    test_watchface_reset();
    WatchfaceSettings settings = default_settings();
    WatchfaceEventData data = {
        .parsed = cases[i].field,
        .received = cases[i].field,
        .temp_unit = TEMP_UNIT_C,
        .weather_update_minutes = 60,
        .battery_orientation = !BATTERY_ORIENTATION_DEFAULT,
        .hr_sample_minutes = 30,
        .steps_goal = 20000};
    watchface_apply_received_data(&data, &settings);
    cl_assert(data.settings_changed);
    cl_assert_equal_i(0, test_watchface.repaints);
    cl_assert_equal_i(cases[i].updates, test_watchface.updates);
    cl_assert_equal_i((cases[i].updates != WATCHFACE_UPDATE_NONE), test_watchface.refreshes);
    cl_assert(test_watchface.relayouts == (cases[i].field == WATCHFACE_DATA_BATTERY_ORIENTATION));
    if (cases[i].field == WATCHFACE_DATA_TEMP_UNIT)
      cl_assert_equal_i(TEMP_UNIT_C, settings.temp_unit);
    if (cases[i].field == WATCHFACE_DATA_WEATHER_UPDATE_MINUTES)
      cl_assert_equal_i(60, settings.weather_update_minutes);
    if (cases[i].field == WATCHFACE_DATA_BATTERY_ORIENTATION)
      cl_assert_equal_i(!BATTERY_ORIENTATION_DEFAULT, settings.battery_orientation);
#ifdef PBL_HEALTH
    if (cases[i].field == WATCHFACE_DATA_HR_SAMPLE_MINUTES)
      cl_assert_equal_i(30, settings.hr_sample_minutes);
    if (cases[i].field == WATCHFACE_DATA_STEPS_GOAL) cl_assert_equal_i(20000, settings.steps_goal);
#endif
    // Repeating an acknowledged setting has no refresh or persistence trigger.
    test_watchface_reset();
    data.settings_changed = false;
    watchface_apply_received_data(&data, &settings);
    cl_assert(!data.settings_changed && !test_watchface.refreshes && !test_watchface.relayouts);
  }
}

void test_runtime__repaint_precedence(
    void) {
  test_watchface_reset();
  WatchfaceSettings settings = default_settings();
  WatchfaceEventData data = {
      .parsed = WATCHFACE_DATA_DISPLAY_MODE | WATCHFACE_DATA_TEMP_UNIT | WATCHFACE_DATA_TIME_TICK |
                WATCHFACE_DATA_BATTERY_ORIENTATION,
      .display_mode = DISPLAY_MODE_DARK_MONOCHROME,
      .temp_unit = TEMP_UNIT_C,
      .battery_orientation = !BATTERY_ORIENTATION_DEFAULT};
  watchface_apply_received_data(&data, &settings);
  cl_assert(data.settings_changed && settings.display_mode == DISPLAY_MODE_DARK_MONOCHROME);
  cl_assert(test_watchface.repaints == 1 && test_watchface.refreshes == 0);
  cl_assert_equal_i(1, test_watchface.relayouts);
  test_watchface_reset();
  data = (WatchfaceEventData){
      .parsed = WATCHFACE_DATA_DISPLAY_MODE,
      .display_mode = DISPLAY_MODE_DARK_MONOCHROME};
  watchface_apply_received_data(&data, &settings);
  cl_assert(!data.settings_changed && !test_watchface.repaints && !test_watchface.refreshes);
}

void test_runtime__rejected_settings(
    void) {
  WatchfaceDataMask mask = WATCHFACE_DATA_TEMP_UNIT | WATCHFACE_DATA_DISPLAY_MODE |
                           WATCHFACE_DATA_WEATHER_UPDATE_MINUTES |
                           WATCHFACE_DATA_BATTERY_ORIENTATION;
#ifdef PBL_HEALTH
  mask |= WATCHFACE_DATA_HR_SAMPLE_MINUTES | WATCHFACE_DATA_STEPS_GOAL;
#endif
  for (int parsed = 0; parsed <= 1; ++parsed) {
    test_watchface_reset();
    WatchfaceSettings settings = default_settings();
    uint8_t before[sizeof(settings)];
    memcpy(before, &settings, sizeof(before));
    WatchfaceEventData data = {
        .received = mask,
        .parsed = parsed ? mask : 0,
        .temp_unit = -1,
        .display_mode = -1,
        .weather_update_minutes = 16,
        .battery_orientation = 2,
        .hr_sample_minutes = 0,
        .steps_goal = 0};
    watchface_apply_received_data(&data, &settings);
    cl_assert(memcmp(&settings, before, sizeof(before)) == 0);
    cl_assert(!data.settings_changed && !test_watchface.repaints && !test_watchface.refreshes);
  }
}

void test_runtime__climate_completeness(
    void) {
  const WatchfaceDataMask weather =
      WATCHFACE_DATA_TEMPERATURE | WATCHFACE_DATA_WEATHER_CONDITION | WATCHFACE_DATA_IS_DAY;
  const WatchfaceDataMask parts[] = {
      WATCHFACE_DATA_TEMPERATURE,
      WATCHFACE_DATA_WEATHER_CONDITION,
      WATCHFACE_DATA_IS_DAY};
  for (unsigned combination = 0; combination < 8; ++combination) {
    test_context("weather parsed combination=%u", combination);
    test_watchface_reset();
    WatchfaceSettings settings = default_settings();
    WatchfaceDataMask parsed = 0;
    for (unsigned bit = 0; bit < 3; ++bit)
      if (combination & (1u << bit)) parsed |= parts[bit];
    WatchfaceEventData data = {
        .received = weather,
        .parsed = parsed,
        .temperature_celsius_tenths = -125,
        .weather_condition = 63,
        .is_day = 0};
    watchface_apply_received_data(&data, &settings);
    cl_assert_equal_i(WATCHFACE_UPDATE_CLIMATE, test_watchface.updates);
    cl_assert(test_watchface.refreshes == 1 && !data.settings_changed);
    cl_assert(data.current_climate.received && data.current_climate.complete == (combination == 7));
    if (combination == 7) {
      cl_assert_equal_i(-125, data.current_climate.celsius_tenths);
      cl_assert(data.current_climate.weather_condition == 63 && data.current_climate.is_day == 0);
    }
  }
  for (size_t i = 0; i < ARRAY_LENGTH(parts); ++i) {
    test_context("parts row=%zu", i);
    test_watchface_reset();
    WatchfaceSettings settings = default_settings();
    WatchfaceEventData data = {.received = parts[i], .parsed = parts[i]};
    watchface_apply_received_data(&data, &settings);
    cl_assert(data.current_climate.received && !data.current_climate.complete);
    cl_assert_equal_i(WATCHFACE_UPDATE_CLIMATE, test_watchface.updates);
  }
}

void test_runtime__service_dispatch(
    void) {
  const struct {
    WatchfaceDataMask field;
    WatchfaceUpdateMask update;
  } cases[] = {
      {WATCHFACE_DATA_TIME_TICK, WATCHFACE_UPDATE_TIME},
      {WATCHFACE_DATA_DATE_TICK, WATCHFACE_UPDATE_DATE},
      {WATCHFACE_DATA_BATTERY_EVENT, WATCHFACE_UPDATE_BATTERY},
      {WATCHFACE_DATA_BLUETOOTH, WATCHFACE_UPDATE_BLUETOOTH},
#ifdef PBL_HEALTH
      {WATCHFACE_DATA_HEALTH_EVENT, WATCHFACE_UPDATE_HEALTH},
#endif
  };
  WatchfaceSettings settings = default_settings();
  for (size_t i = 0; i < ARRAY_LENGTH(cases); ++i) {
    test_context("cases row=%zu", i);
    test_watchface_reset();
    WatchfaceEventData data = {.received = cases[i].field};
    watchface_apply_received_data(&data, &settings);
    cl_assert(!test_watchface.refreshes);
    data.parsed = cases[i].field;
    watchface_apply_received_data(&data, &settings);
    cl_assert(test_watchface.refreshes == 1 && test_watchface.updates == cases[i].update);
  }
  test_watchface_reset();
  WatchfaceEventData location = {.received = WATCHFACE_DATA_LOCATION};
  watchface_apply_received_data(&location, &settings);
  cl_assert_equal_i(WATCHFACE_UPDATE_LOCATION, test_watchface.updates);
  cl_assert(!location.settings_changed);
  test_watchface_reset();
  WatchfaceEventData combined = {.received = WATCHFACE_DATA_LOCATION};
  WatchfaceUpdateMask expected = WATCHFACE_UPDATE_LOCATION;
  for (size_t i = 0; i < ARRAY_LENGTH(cases); ++i) {
    test_context("cases row=%zu", i);
    combined.parsed |= cases[i].field;
    expected |= cases[i].update;
  }
  watchface_apply_received_data(&combined, &settings);
  cl_assert(test_watchface.refreshes == 1 && test_watchface.updates == expected);
  test_watchface_reset();
  WatchfaceEventData empty = {0};
  watchface_apply_received_data(&empty, &settings);
  watchface_apply_received_data(NULL, &settings);
  watchface_apply_received_data(&empty, NULL);
  cl_assert(!test_watchface.refreshes && !test_watchface.repaints);
}

void test_runtime__health_overrides(
    void) {
#ifdef PBL_HEALTH
  test_watchface_reset();
  WatchfaceSettings settings = default_settings();
  WatchfaceEventData data = {
      .received = WATCHFACE_DATA_ONESHOT_BPM | WATCHFACE_DATA_ONESHOT_STEPS,
      .oneshot_bpm = 72,
      .oneshot_steps = 12345};
  watchface_apply_received_data(&data, &settings);
  cl_assert(!test_watchface.bpm_calls && !test_watchface.steps_calls);
  data.parsed = data.received;
  watchface_apply_received_data(&data, &settings);
  cl_assert(test_watchface.bpm_calls == 1 && test_watchface.bpm == 72);
  cl_assert(test_watchface.steps_calls == 1 && test_watchface.steps == 12345);
  cl_assert(test_watchface.updates == WATCHFACE_UPDATE_HEALTH && !data.settings_changed);
#else
  test_skip("requires PBL_HEALTH");
#endif
}
