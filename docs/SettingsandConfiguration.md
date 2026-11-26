# Settings and Configuration

This document defines settings, defaults, valid values, message keys, persistence, and runtime effects for _At A Glance_. It traces a preference from Clay through PKJS and AppMessage to the watch, and defines weather scheduling and delivery.

`ataglance.c` loads persisted settings before creating the watchface. The runtime validates incoming settings; the communications handler saves accepted changes and applies service side effects.

## Adjacent

- [ProductInvariants](ProductInvariants.md) for the product properties settings must preserve.
- [VisualVocabulary](VisualVocabulary.md) for the visual rules affected by display settings.

## Settings Catalog

The runtime setting source of truth is `WatchfaceSettings` in `src/modules/settings.h`. On health-capable builds, there are six persisted runtime settings. Time follows the watch’s system 12/24-hour preference through `clock_is_24h_style()`; Clay has no time-format setting.

| Setting | C field | Canonical key | Default | Valid values | Persisted | Runtime effect |
| --- | --- | --- | --- | --- | --- | --- |
| 1. Temperature unit | `temp_unit` | `TEMP_UNIT` | `0` / `TEMP_UNIT_F` | `0` Fahrenheit, `1` Celsius | Yes | Refreshes climate text |
| 2. Display mode | `display_mode` | `DISPLAY_MODE` | `0` / `DISPLAY_MODE_LIGHT_MONOCHROME` | `0`, `1` on B/W; `0`, `1`, `2`, `3` on color | Yes | Repaints watch face |
| 3. Weather cadence | `weather_update_minutes` | `WEATHER_UPDATE_MINUTES` | `15` | `15`, `30`, `45`, `60` | Yes | PKJS schedules updates; C persists its copy |
| 4. Heart-rate cadence | `hr_sample_minutes` | `HR_SAMPLE_MINUTES` | `15` | C accepts integers `10` through `120`; Clay offers `10`, `15`, `30`, `60`, `120` | Yes on `PBL_HEALTH` | Updates HealthService sample period |
| 5. Steps goal | `steps_goal` | `STEPS_GOAL` | `10000` | `4000` through `32000` | Yes on `PBL_HEALTH` | Refreshes steps display |
| 6. Battery orientation | `battery_orientation` | `BATTERY_ORIENTATION` | `0` horizontal on rectangular displays; `1` vertical on round displays | `0` horizontal, `1` vertical | Yes | Relayouts battery presentation |

Clay also exposes two settings-page inputs that are not canonical runtime settings:

| Clay input | Message key | Purpose | Crosses JS-to-C boundary |
| --- | --- | --- | --- |
| Steps goal preset | `STEPS_GOAL_PRESET` | Preset candidate for steps goal | No |
| Custom steps goal | `STEPS_GOAL_CUSTOM` | Optional custom override for steps goal | No |

PKJS collapses these two Clay inputs into the canonical `STEPS_GOAL` value before sending settings to C.

Weather, location, and one-shot health overrides are transient AppMessage data. They stay outside the persisted settings catalog.

## Source Of Truth Map

| Source | Owns |
| --- | --- |
| `package.json` | Explicit `messageKeys` numeric IDs and generated symbols |
| `src/pkjs/config.json` | Clay settings page fields, labels, options, capability filters, and form defaults |
| `src/pkjs/index.js` | Clay result parsing, settings normalization, health capability checks, and startup synchronization |
| `src/pkjs/weather_location.js` | Refresh scheduling, connectivity probes, serialized acquisition, location reuse, and changed-field payloads |
| `src/pkjs/app_message_queue.js` | Serialized sends and bounded retries for each queue |
| `src/modules/settings.h` | Runtime setting enums, defaults, validation macros, and `WatchfaceSettings` shape |
| `src/modules/settings.c` | Defaults application, persisted read/write, and persisted-value sanitization |
| `src/c/ataglance.c` | Settings loading, service subscriptions, and AppMessage callback registration |
| `src/c/ataglance_message_parser.c` | Tuple parsing and receipt/parse masks |
| `src/c/ataglance_comms_handler.c` | Inbox sizing/opening, startup sync replies and retries, settings-save trigger, and HealthService cadence updates |
| `src/modules/watchface_runtime_boundary.c` | Runtime setting validation, mutation, repaint/refresh decisions |
| `src/modules/watchface.c` | Settings consumption during repaint and module refresh |
| [SettingsandConfiguration](SettingsandConfiguration.md) | Human-readable settings contract |

## Clay And PKJS Flow

Clay is configured in `src/pkjs/config.json`. PKJS disables Clay’s automatic event handlers and owns configuration events. `clay.getSettings()` persists submitted preferences in phone local storage and returns the outbound dictionary:

```text
Pebble showConfiguration event
  -> clay.generateUrl()
  -> user edits settings page
  -> webviewclosed event
  -> clay.getSettings(e.response)
  -> PKJS normalizes settings, including the weather interval
  -> settings/control queue
  -> Pebble.sendAppMessage(dict)
```

[UserInterface](UserInterface.md#settings) shows the captured settings page.

Clay presents `BATTERY_ORIENTATION` as “Battery orientation:” with “Central bar” for horizontal and “Right Margin” for vertical. Capability-filtered fields default to horizontal on rectangular displays and vertical on round displays. Saved preferences override these defaults.

### PKJS Normalization

Clay maps form fields to numeric message keys. PKJS normalizes steps goal and weather cadence before sending the dictionary.

**1. Steps goal**

```text
STEPS_GOAL_CUSTOM valid
  -> clamp custom to 4000..32000
  -> send STEPS_GOAL

else STEPS_GOAL_PRESET valid
  -> clamp preset to 4000..32000
  -> send STEPS_GOAL

else
  -> send STEPS_GOAL_DEFAULT
```

PKJS computes `STEPS_GOAL` when active-watch information identifies a non-Aplite platform with firmware 3.8 or newer. Otherwise, it removes `STEPS_GOAL` and `HR_SAMPLE_MINUTES` from the outbound dictionary. Clay also filters health controls by capability, and C guards health paths with `PBL_HEALTH`. PKJS always removes `STEPS_GOAL_PRESET` and `STEPS_GOAL_CUSTOM`, so inbox sizing excludes those two form inputs.

**2. Weather update minutes**

PKJS owns the weather schedule. Clay offers `15`, `30`, `45`, and `60` minutes. PKJS parses saved or submitted values, defaults unparseable values to 15, and clamps the result to 15–60. C persists its own copy, accepts only the four supported intervals, and retains its current value when a live update is invalid.

## Weather Scheduling And Delivery

Data accuracy requires refresh operations to complete in request order. The Promise chain includes acquisition, changed-data delivery, unavailable delivery, and cached replay. Each queued operation waits for prior sends to succeed or exhaust their retries before starting. This prevents an older unavailable update from clearing newer weather and location data.

### Startup synchronization

- On `ready`, PKJS sends `SYNC=1` through the settings/control queue.
- C creates the watchface before registering callbacks and opening AppMessage. On receipt of `SYNC`, C replies with `SYNC=1` and continues parsing other tuples.
- C checks the phone app connection before each reply attempt. Disconnected checks and failed sends share a limit of six attempts, with retry delays of one through five seconds. Successful delivery resets retry state; exhaustion ends the sync attempt.
- After startup sync exhausts its attempts, resync from the phone app to start another attempt.
- On receipt of C’s `SYNC=1`, PKJS reads `clay-settings` from local storage, validates `WEATHER_UPDATE_MINUTES`, and calls `kickoffUpdates(minutes)`. Missing, malformed, or unparseable saved settings use 15 minutes.

### Refresh scheduling

- Startup and interval changes start a connectivity probe immediately. PKJS sends `PING=1` through the control queue, allowing three total attempts with retry delays of one and two seconds. Its send-success callback starts acquisition; C sends no application-level PING reply.
- Failed probes skip acquisition and schedule the next check after the selected interval. Existing watch data remains visible.
- PKJS schedules the next check with one timeout after the acquisition chain settles. Both changed-data and unavailable-message sends remain in the chain until delivery succeeds or retries are exhausted.
- A changed interval starts a new probe; acquisition clears the prior timeout. The next timeout uses the new interval. With an unchanged interval and an active timeout, `kickoffUpdates()` resends the cached pair, when available, and leaves the timeout in place. With no active timeout, it starts a probe.

### Acquisition and delivery

- Each acquisition requests coordinates first. Latitude and longitude differences must each be less than `0.02` degrees to reuse the coordinate anchor and cached name. Weather is fetched even when coordinates are unchanged.
- Geolocation and network requests use a 15-second timeout. Geolocation may reuse a position up to 10 minutes old.
- Climate and reverse geocoding run concurrently. `Promise.all()` waits for both results before building a payload.
- A location name that differs from the cached lookup adds `MAYBE_CURRENT_LOCATION`. Movement beyond the coordinate tolerance clears that lookup and forces a new name request, so the location field is sent even if the new name matches the previous name.
- A change in any climate value adds all three climate fields: `TEMPERATURE`, `WEATHER_CONDITION`, and `IS_DAY`. Unchanged fields are omitted; unchanged results produce no send.
- Successful delivery updates `lastKnownClimate` and `lastKnownLookup` together. Failed delivery does not advance either cache, so the next acquisition can send the changes again.
- Geolocation failure, a rejected request, or a missing result clears the coordinates and caches. PKJS sends the full unavailable payload: temperature `0`, condition `-1`, `IS_DAY=0`, and an empty location string.
- PKJS owns coordinated acquisition and changed-field delivery. C accepts weather and location independently; a weather update still requires all three climate fields.

## AppMessage And Message-Key Contract

`package.json` assigns explicit numeric IDs in its `messageKeys` object.

Current manifest-backed keys:

| #   | Key                      | Generated numeric ID | Role                            |
| --- | ------------------------ | -------------------- | ------------------------------- |
| 1   | `PING`                   | `10000`              | PKJS connectivity probe         |
| 2   | `TEMP_UNIT`              | `10001`              | Runtime setting                 |
| 3   | `TEMPERATURE`            | `10002`              | Weather data                    |
| 4   | `WEATHER_CONDITION`      | `10003`              | Weather data                    |
| 5   | `IS_DAY`                 | `10004`              | Weather data                    |
| 6   | `WEATHER_UPDATE_MINUTES` | `10005`              | Runtime setting                 |
| 7   | `DISPLAY_MODE`           | `10006`              | Runtime setting                 |
| 8   | `HR_SAMPLE_MINUTES`      | `10007`              | Runtime setting on `PBL_HEALTH` |
| 9   | `STEPS_GOAL`             | `10008`              | Runtime setting on `PBL_HEALTH` |
| 10  | `STEPS_GOAL_PRESET`      | `10009`              | Clay-only input                 |
| 11  | `STEPS_GOAL_CUSTOM`      | `10010`              | Clay-only input                 |
| 12  | `MAYBE_CURRENT_LOCATION` | `10011`              | Optional location data          |
| 13  | `SYNC`                   | `10012`              | Startup synchronization in both directions |
| 14  | `BATTERY_ORIENTATION`    | `10013`              | Runtime setting                 |

**Notes**

- Numeric IDs come from explicit manifest values. Object order does not assign IDs.
- Generated artifacts may list names alphabetically.
- Manual QA commands, harness helpers, PKJS, and C must follow the generated numeric mapping.

**Rules**

- Preserve assigned IDs when adding keys; assign each new key an unused ID.
- Recheck generated key output after any `messageKeys` change.
- Update JS, C parsing, inbox sizing, harness helpers, manual QA commands, and docs in the same change.
- Do not assume generated file order is the same as numeric order.

### QA-Only One-Shot Keys

The current one-shot health keys are runtime-only constants:

| Key                                   | Numeric ID | Purpose                 |
| ------------------------------------- | ---------- | ----------------------- |
| `WATCHFACE_ONESHOT_MESSAGE_KEY_BPM`   | `10020`    | One-shot BPM override   |
| `WATCHFACE_ONESHOT_MESSAGE_KEY_STEPS` | `10021`    | One-shot steps override |

These keys provide transient QA overrides. Production PKJS does not send them.

[Validation](Validation.md) owns the operational command examples for these keys. This document references them only to keep the settings, data, and QA payloads separate.

## Watch Runtime And Persistence Flow

### Startup

```text
main_window_load(window)
  -> settings_load(&s_settings)
       -> apply defaults
       -> read persisted data if present
       -> sanitize values
  -> watchface_create(window, &s_settings)
       -> module_create(params, relevant_loaded_setting)
  -> open AppMessage
```

### Settings receipt

```text
inbox_received_handler(iter, context)
  -> retain the previous heart-rate cadence on PBL_HEALTH
  -> respond to SYNC when present, then continue parsing
  -> parse settings tuples
  -> parse weather and location tuples
  -> parse one-shot health tuples on PBL_HEALTH
  -> watchface_apply_received_data(&data, settings)
  -> save settings if persisted fields changed
  -> update HealthService sample period if HR cadence changed
```

### Runtime mutation

Sanitize settings on initial load. Reject invalid live updates and retain the current valid value.

```text
watchface_apply_received_data()
  -> validate setting values
  -> mutate WatchfaceSettings
  -> request repaint or targeted refresh
```

Refresh effects:

| Setting | Runtime action |
| --- | --- |
| `TEMP_UNIT` | `WATCHFACE_UPDATE_CLIMATE` |
| `DISPLAY_MODE` | `WATCHFACE_REPAINT` |
| `BATTERY_ORIENTATION` | `WATCHFACE_UPDATE_BATTERY_ORIENTATION` |
| `WEATHER_UPDATE_MINUTES` | Persisted in C; PKJS applies its local value through `kickoffUpdates()` |
| `HR_SAMPLE_MINUTES` | Persisted in C; HealthService sample period updated in `ataglance_comms_handler.c` |
| `STEPS_GOAL` | `WATCHFACE_UPDATE_HEALTH` |

### Persistence

Normal settings updates write the full `WatchfaceSettings` struct to Pebble storage key `3` only when a validated persisted field changes.

Migration changes the persisted format and key. After reading a complete legacy record from key `2`, the loader immediately writes its replacement to key `3`. It deletes key `2` only after the full replacement write succeeds. A failed or partial write preserves the readable legacy record for the next load. Preference values can remain unchanged while their persisted representation changes.

The loader sanitizes loaded values. Stored weather intervals are clamped to 15–60 and rounded down to a multiple of 15; invalid live updates retain the current value.

## Change Checklist

For a settings change, reconcile the affected owners and validate the complete path:

- Update `package.json` only when the AppMessage key set changes.
- Regenerate message-key output after key changes and confirm IDs match manual QA commands and harness helpers.
- Check `src/pkjs/config.json` fields, defaults, options, and capability filters.
- Check `src/pkjs/index.js` normalization, canonical outbound keys, and startup synchronization.
- Update `src/modules/settings.h` and `src/modules/settings.c` for defaults, validation, persisted shape, or sanitization changes.
- Update `src/c/ataglance_message_parser.c` for tuple parsing.
- Update `src/c/ataglance_comms_handler.c` for inbox sizing, save triggers, or HealthService side effects.
- Update `src/c/ataglance.c` when startup loading or service subscriptions change.
- Update `src/modules/watchface_runtime_boundary.c` for runtime validation, mutation, repaint, or refresh behavior.
- Update [SettingsandConfiguration](SettingsandConfiguration.md).
- Update [Validation](Validation.md) only when validation flow changes.
- Update [UserInterface](UserInterface.md) only when visible settings evidence or visual consequences change.
- Verify defaults, invalid live values, stored-value sanitization, persistence after restart, and the intended refresh or repaint scope.
- Run the relevant build and validation path, or record the validation gap. Use [BuildandInstall](BuildandInstall.md) for commands and [Validation](Validation.md) for runtime checks.

## Read Next

- [RuntimeArchitecture](RuntimeArchitecture.md) for the runtime architecture.
- [UserInterface](UserInterface.md) for the visual consequences of settings.
- [BuildandInstall](BuildandInstall.md) describes build and install flows and associated commands.
- [Validation](Validation.md) for automated tests, emulator checks, and operator evidence.
