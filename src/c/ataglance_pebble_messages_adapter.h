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
 * - app-facing, not module-facing
 *   This contract is private to src/c. Feature modules and watchface runtime
 *   code should not depend on it.
 *
 * - no tuple parsing declarations
 *   Tuple parsing has its own header and implementation so that parsing and
 *   transport handling stay distinct.
 */

// Apply changes
void ataglance_apply_received_data(WatchfaceEventData* parsed);

// Inbox-Outbox handling
AppMessageResult initialize_inbox_outbox();
void inbox_received_callback(DictionaryIterator* iter, void* context);
void inbox_dropped_callback(AppMessageResult reason, void* context);
void outbox_failed_callback(DictionaryIterator* iterator, AppMessageResult reason, void* context);
void outbox_sent_callback(DictionaryIterator* iterator, void* context);
void send_loaded_weather_update_minutes(uint8_t minutes);
