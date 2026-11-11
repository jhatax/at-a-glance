#include "ataglance_message_parser.h"

#include <stdint.h>
#include <string.h>

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
  if (tuple_to_int(tuple, value)) {
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
  if (!iter || !data || !value || !max_len) {
    return;
  }

  Tuple* tuple = dict_find(iter, key);
  if (!tuple) {
    return;
  }

  data->received = (WatchfaceDataMask)(data->received | mask);
  if (tuple_to_string(tuple, value, max_len)) {
    data->parsed = (WatchfaceDataMask)(data->parsed | mask);
  } else {
    APP_LOG(APP_LOG_LEVEL_WARNING, "AppMessage inbox invalid string tuple: key=%lu", key);
  }
}

bool parse_ready_sentinel(
    DictionaryIterator* iter) {
  if (iter) {
    int ready = 0;
    Tuple* tuple = dict_find(iter, MESSAGE_KEY_JS_READY);
    if (tuple && tuple_to_int(tuple, &ready)) {
      return true;
    }
  }
  return false;
}

void parse_settings_data(
    DictionaryIterator* iter,
    WatchfaceEventData* data) {
  if (!iter || !data) {
    return;
  }
  parse_int_tuple(
      iter,
      MESSAGE_KEY_TIME_FORMAT,
      data,
      WATCHFACE_DATA_TIME_FORMAT,
      &data->time_format);
  parse_int_tuple(iter, MESSAGE_KEY_TEMP_UNIT, data, WATCHFACE_DATA_TEMP_UNIT, &data->temp_unit);
  parse_int_tuple(
      iter,
      MESSAGE_KEY_DISPLAY_MODE,
      data,
      WATCHFACE_DATA_DISPLAY_MODE,
      &data->display_mode);
  parse_int_tuple(
      iter,
      MESSAGE_KEY_WEATHER_UPDATE_MINUTES,
      data,
      WATCHFACE_DATA_WEATHER_UPDATE_MINUTES,
      &data->weather_update_minutes);
  parse_int_tuple(
      iter,
      MESSAGE_KEY_BATTERY_ORIENTATION,
      data,
      WATCHFACE_DATA_BATTERY_ORIENTATION,
      &data->battery_orientation);

#ifdef PBL_HEALTH
  parse_int_tuple(
      iter,
      MESSAGE_KEY_HR_SAMPLE_MINUTES,
      data,
      WATCHFACE_DATA_HR_SAMPLE_MINUTES,
      &data->hr_sample_minutes);
  parse_int_tuple(iter, MESSAGE_KEY_STEPS_GOAL, data, WATCHFACE_DATA_STEPS_GOAL, &data->steps_goal);
#endif
}

void parse_weather_and_location_data(
    DictionaryIterator* iter,
    WatchfaceEventData* data) {
  if (!iter || !data) {
    return;
  }

  parse_int_tuple(
      iter,
      MESSAGE_KEY_TEMPERATURE,
      data,
      WATCHFACE_DATA_TEMPERATURE,
      &data->temperature_celsius_tenths);

  parse_int_tuple(
      iter,
      MESSAGE_KEY_WEATHER_CONDITION,
      data,
      WATCHFACE_DATA_WEATHER_CONDITION,
      &data->weather_condition);
  parse_int_tuple(iter, MESSAGE_KEY_IS_DAY, data, WATCHFACE_DATA_IS_DAY, &data->is_day);
  data->pin.received = false;
  parse_string_tuple(
      iter,
      MESSAGE_KEY_MAYBE_CURRENT_LOCATION,
      data,
      WATCHFACE_DATA_LOCATION,
      data->pin.name,
      ARRAY_LENGTH(data->pin.name));
  data->pin.received = data->received & WATCHFACE_DATA_LOCATION;
}

bool tuple_to_int(
    Tuple* tuple,
    int* value) {
  if (!tuple || !value) {
    return false;
  }

  switch (tuple->type) {
    case TUPLE_INT:
      switch (tuple->length) {
        case 1:
          *value = tuple->value->int8;
          return true;
        case 2:
          *value = tuple->value->int16;
          return true;
        case 4:
          *value = tuple->value->int32;
          return true;
        default:
          return false;
      }
    case TUPLE_CSTRING: {
      const char* text = tuple->value->cstring;
      int parsed = 0;
      if (!text || text[0] == '\0') {
        return false;
      }
      for (const char* p = text; *p; ++p) {
        if (*p < '0' || *p > '9') {
          return false;
        }
        int digit = *p - '0';
        if (parsed > (INT16_MAX - digit) / 10) {
          return false;
        }
        parsed = (parsed * 10) + digit;
      }
      *value = parsed;
      return true;
    }

    default:
      return false;
  }
}

bool tuple_to_string(
    Tuple* tuple,
    char* value,
    uint8_t max_len) {
  if (!tuple || !value || !max_len) {
    return false;
  }

  if (TUPLE_CSTRING == tuple->type) {
    memset(value, 0, max_len * sizeof(*value));
    const char* text = tuple->value->cstring;
    if (!text) {
      return false;
    }
    // If the empty string is sent, that means location couldn't be found
    snprintf(value, max_len, "%s", text);
    return true;
  }

  return false;
}
