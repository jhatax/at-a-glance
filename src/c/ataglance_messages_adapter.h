#pragma once

#include <pebble.h>

#include "modules/watchface.h"

/*
 * Header invariants:
 *
 * - narrow Pebble-message adapter contract
 *   This header exposes the AppMessage-facing adapter entry points used by
 *   ataglance.c and their bridge back into the watchface runtime.
 *
 * - app-facing only
 *   This contract is private to src/c. Feature modules and watchface runtime
 *   code should not depend on it.
 *
 * - inbox-outbox message handling APIs only
 *   Tuple parsing has its own header and implementation so that parsing and
 *   transport handling stay distinct.
 */
// Apply changes
void ataglance_apply_received_data(WatchfaceEventData* parsed);

// Inbox-Outbox handling
void initialize_inbox_outbox(void* context);
void inbox_received_handler(
    DictionaryIterator* iter,
    void* context);
bool find_the_canary(DictionaryIterator* iter);
void handle_the_message(DictionaryIterator* iter);
void inbox_dropped_callback(
    AppMessageResult reason,
    void* context);
void outbox_failed_callback(
    DictionaryIterator* iterator,
    AppMessageResult reason,
    void* context);
void outbox_sent_callback(
    DictionaryIterator* iterator,
    void* context);
void send_loaded_weather_update_minutes(uint8_t minutes);
