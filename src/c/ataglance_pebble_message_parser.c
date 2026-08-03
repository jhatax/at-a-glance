#include <stdint.h>
#include <string.h>

#include "ataglance_message_parser.h"
#include "modules/helper.h"
#include "modules/watchface.h"

/*
 * File invariants:
 *
 * - parse raw tuples into WatchfaceEventData only
 *   This file translates AppMessage tuples into caller-owned event data and
 *   parsed/received masks.
 *
 * - no runtime side effects
 *   This file must not persist settings, update services, call watchface
 *   runtime APIs, or make repaint/refresh decisions.
 *
 * - no transport-handler ownership
 *   Inbox/outbox callbacks, AppMessage sizing, open/retry logic, and outbox
 *   sends belong to the Pebble messages handler.
 *
 * - no layout, palette, or module behavior leakage
 *   Parsing ends at typed event data. Visual and runtime behavior begin
 *   elsewhere.
 */

void parse_int_tuple(
    DictionaryIterator* iter,
    uint32_t key,
    WatchfaceEventData* data,
    WatchfaceDataMask mask,
    int* value) {
  if (!iter || !data || !value) {
    return;
  }

  Tuple* tuple = dict_find(iter, key);
  if (!tuple) {
    return;
  }

  data->received = (WatchfaceDataMask)(data->received | mask);
  if (helper_tuple_to_int(tuple, value)) {
    data->parsed = (WatchfaceDataMask)(data->parsed | mask);
  } else {
    APP_LOG(APP_LOG_LEVEL_WARNING, "AppMessage inbox invalid int tuple: key=%lu", key);
  }
}

void parse_string_tuple(
    DictionaryIterator* iter,
    uint32_t key,
    WatchfaceEventData* data,
    WatchfaceDataMask mask,
    char* value,
    uint8_t max_len) {
  if (!iter || !data || !value) {
    return;
  }

  Tuple* tuple = dict_find(iter, key);
  if (!tuple) {
    return;
  }

  data->received = (WatchfaceDataMask)(data->received | mask);
  if (TUPLE_CSTRING == tuple->type) {
    const char* text = tuple->value->cstring;
    if (!text) {
      return;
    }
    // If the empty string is sent, that means location couldn't be found
    snprintf(value, max_len, "%s", text);
    data->parsed = (WatchfaceDataMask)(data->parsed | mask);
    return;
  }
}

void parse_settings_data(
    DictionaryIterator* iter,
    WatchfaceEventData* data) {
  if (!iter || !data) {
    return;
  }
  parse_int_tuple(iter,
      MESSAGE_KEY_TIME_FORMAT,
      data,
      WATCHFACE_DATA_TIME_FORMAT,
      &data->time_format);
  parse_int_tuple(iter, MESSAGE_KEY_TEMP_UNIT, data, WATCHFACE_DATA_TEMP_UNIT, &data->temp_unit);
  parse_int_tuple(iter,
      MESSAGE_KEY_DISPLAY_MODE,
      data,
      WATCHFACE_DATA_DISPLAY_MODE,
      &data->display_mode);
  parse_int_tuple(iter,
      MESSAGE_KEY_WEATHER_UPDATE_MINUTES,
      data,
      WATCHFACE_DATA_WEATHER_UPDATE_MINUTES,
      &data->weather_update_minutes);
}

void parse_weather_data(
    DictionaryIterator* iter,
    WatchfaceEventData* data) {
  if (!iter || !data) {
    return;
  }
  parse_int_tuple(iter,
      MESSAGE_KEY_TEMPERATURE,
      data,
      WATCHFACE_DATA_TEMPERATURE,
      &data->temperature_celsius_tenths);
  parse_int_tuple(iter,
      MESSAGE_KEY_WEATHER_CONDITION,
      data,
      WATCHFACE_DATA_WEATHER_CONDITION,
      &data->weather_condition);
  parse_int_tuple(iter, MESSAGE_KEY_IS_DAY, data, WATCHFACE_DATA_IS_DAY, &data->is_day);
}

void parse_location_data(
    DictionaryIterator* iter,
    WatchfaceEventData* data) {
  if (!iter || !data) {
    return;
  }
  memset(data->location, 0, sizeof(data->location));
  parse_string_tuple(iter,
      MESSAGE_KEY_MAYBE_CURRENT_LOCATION,
      data,
      WATCHFACE_DATA_LOCATION,
      data->location,
      ARRAY_LENGTH(data->location));
}

#ifdef PBL_HEALTH
void parse_health_settings_data(
    DictionaryIterator* iter,
    WatchfaceEventData* data) {
  if (!iter || !data) {
    return;
  }
  parse_int_tuple(iter,
      MESSAGE_KEY_HR_SAMPLE_MINUTES,
      data,
      WATCHFACE_DATA_HR_SAMPLE_MINUTES,
      &data->hr_sample_minutes);
  parse_int_tuple(iter, MESSAGE_KEY_STEPS_GOAL, data, WATCHFACE_DATA_STEPS_GOAL, &data->steps_goal);
}

void parse_oneshot_health_data(
    DictionaryIterator* iter,
    WatchfaceEventData* data) {
  if (!iter || !data) {
    return;
  }

  parse_int_tuple(iter,
      WATCHFACE_ONESHOT_MESSAGE_KEY_BPM,
      data,
      WATCHFACE_DATA_ONESHOT_BPM,
      &data->oneshot_bpm);
  parse_int_tuple(iter,
      WATCHFACE_ONESHOT_MESSAGE_KEY_STEPS,
      data,
      WATCHFACE_DATA_ONESHOT_STEPS,
      &data->oneshot_steps);
}
#endif
