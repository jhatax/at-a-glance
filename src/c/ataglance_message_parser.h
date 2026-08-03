#pragma once

#include <pebble.h>
#include <stdint.h>

#include "modules/watchface.h"

/*
 * Header invariants:
 *
 * - narrow parser contract only
 *   This header exposes only the tuple-to-WatchfaceEventData parsing entry
 *   points needed by the Pebble message handler.
 *
 * - no transport-handler ownership
 *   Inbox/outbox callbacks, AppMessage sizing/opening, and adapter-side sends
 *   belong to the Pebble messages adapter header and implementation.
 *
 * - no runtime or watchface policy
 *   These declarations support parsing only. Settings mutation, refresh
 *   decisions, persistence, and rendering behavior belong elsewhere.
 */

// Message parsing
void parse_int_tuple(DictionaryIterator* iter,
    uint32_t key,
    WatchfaceEventData* data,
    WatchfaceDataMask mask,
    int* value);
void parse_string_tuple(DictionaryIterator* iter,
    uint32_t key,
    WatchfaceEventData* data,
    WatchfaceDataMask mask,
    char* value,
    uint8_t max_len);
void parse_settings_data(DictionaryIterator* iter,
    WatchfaceEventData* data);
void parse_weather_data(DictionaryIterator* iter,
    WatchfaceEventData* data);
void parse_location_data(DictionaryIterator* iter,
    WatchfaceEventData* data);
#ifdef PBL_HEALTH
void parse_health_settings_data(DictionaryIterator* iter,
    WatchfaceEventData* data);
void parse_oneshot_health_data(DictionaryIterator* iter,
    WatchfaceEventData* data);
#endif
