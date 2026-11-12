# Settings and Configuration

This document defines settings, defaults, valid values, message keys, persistence, and runtime effects for _At A Glance_. It traces a preference from Clay through PKJS and AppMessage to the watch.

`ataglance.c` loads persisted settings before creating the watchface. The runtime validates incoming settings; the communications handler saves accepted changes and applies service side effects.

## Adjacent

- [ProductInvariants](ProductInvariants.md) for the product properties settings must preserve.
- [VisualVocabulary](VisualVocabulary.md) for the visual rules affected by display settings.

## Settings Catalog

The runtime setting source of truth is `WatchfaceSettings` in `src/modules/settings.h`. On health-capable builds, there are seven persisted runtime settings.

| Setting | C field | Canonical key | Default | Valid values | Persisted | Runtime effect |
| --- | --- | --- | --- | --- | --- | --- |
| 1. Time format | `time_format` | `TIME_FORMAT` | `0` / `TIME_FMT_24` | `0` 24-hour, `1` 12-hour | Yes | Refreshes time text |
| 2. Temperature unit | `temp_unit` | `TEMP_UNIT` | `0` / `TEMP_UNIT_F` | `0` Fahrenheit, `1` Celsius | Yes | Refreshes climate text |
| 3. Display mode | `display_mode` | `DISPLAY_MODE` | `0` / `DISPLAY_MODE_LIGHT_MONOCHROME` | `0`, `1` on B/W; `0`, `1`, `2`, `3` on color | Yes | Repaints watch face |
| 4. Weather cadence | `weather_update_minutes` | `WEATHER_UPDATE_MINUTES` | `15` | `15`, `30`, `45`, `60` | Yes | C persists the interval and schedules refresh requests |
| 5. Heart-rate cadence | `hr_sample_minutes` | `HR_SAMPLE_MINUTES` | `15` | C accepts integers `10` through `120`; Clay offers `10`, `15`, `30`, `60`, `120` | Yes on `PBL_HEALTH` | Updates HealthService sample period |
| 6. Steps goal | `steps_goal` | `STEPS_GOAL` | `10000` | `4000` through `32000` | Yes on `PBL_HEALTH` | Refreshes steps display |
| 7. Battery orientation | `battery_orientation` | `BATTERY_ORIENTATION` | `0` horizontal on rectangular displays; `1` vertical on round displays | `0` horizontal, `1` vertical | Yes | Relayouts battery presentation |

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
| `package.json` | Pebble manifest `messageKeys` order and generated numeric IDs |
| `src/pkjs/config.json` | Clay settings page fields, labels, options, capability filters, and form defaults |
| `src/pkjs/index.js` | Clay result parsing, settings normalization, readiness announcement, and refresh-request handling |
| `src/pkjs/weather_location.js` | One climate/location request per REFRESH, location reuse, and combined payload creation |
| `src/pkjs/app_message_queue.js` | Serialized sends and bounded retries for each queue |
| `src/modules/settings.h` | Runtime setting enums, defaults, validation macros, and `WatchfaceSettings` shape |
| `src/modules/settings.c` | Defaults application, persisted read/write, and persisted-value sanitization |
| `src/c/ataglance.c` | Settings loading, service subscriptions, and AppMessage callback registration |
| `src/c/ataglance_message_parser.c` | Tuple parsing and receipt/parse masks |
| `src/c/ataglance_comms_handler.c` | Inbox sizing/opening, refresh scheduling and retries, settings-save trigger, and HealthService cadence updates |
| `src/modules/watchface_runtime_boundary.c` | Runtime setting validation, mutation, repaint/refresh decisions |
| `src/modules/watchface.c` | Settings consumption during repaint and module refresh |
| [SettingsandConfiguration](SettingsandConfiguration.md) | Human-readable settings contract |

## Clay And PKJS Flow

Clay is configured in `src/pkjs/config.json` and opened from PKJS:

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

PKJS removes `STEPS_GOAL_PRESET` and `STEPS_GOAL_CUSTOM` after computing `STEPS_GOAL`. Only the canonical goal crosses the boundary, so inbox sizing excludes those two Clay inputs.

**2. Weather Update Minutes & Scheduling**

C owns the weather schedule and uses its persisted `WEATHER_UPDATE_MINUTES` value. Clay offers `15`, `30`, `45`, and `60` minutes. PKJS parses the submitted value, defaults unparseable values to 15, and clamps the result to 15–60. C accepts only the four supported intervals and retains its current value when a live update is invalid.

Startup and refresh requests use separate keys:

- On `ready`, PKJS sends `JS_READY=1` through the settings/control queue and waits for a request.
- C creates the watchface before registering callbacks and opening AppMessage. On receipt of `JS_READY`, C starts a refresh attempt and continues parsing other tuples in the message.
- Each `REFRESH=1` queues one climate/location request in JS. Weather is fetched again; a reverse-geocoded location may be reused while coordinates remain within the location tolerance.
- C checks the phone app connection before each attempt. While disconnected, it skips the outbox and schedules a retry. These checks count toward the same six-attempt limit as failed sends.
- Retry delays run from one through five seconds. Successful delivery resets the retry state and schedules the next C cycle. Exhausted attempts also schedule the next cycle, which checks the connection again.
- C schedules the next request independently of whether JS retrieves or delivers climate/location successfully. PKJS has no periodic refresh timer.
- An accepted interval change starts a refresh immediately and replaces the C cadence timer. An unchanged interval leaves the schedule alone.

Climate and reverse-geocoding requests run concurrently. PKJS updates `lastKnownClimate` and `lastKnownLookup` together only after both promises resolve. A missing result clears the cached fields and sends unavailable data. Geolocation failure or a rejected request also clears the caches and sends unavailable data. The combined payload contract belongs to PKJS; C continues to accept weather and location tuples independently.

## AppMessage And Message-Key Contract

`package.json` owns the manifest-backed message-key list.

Current manifest-backed keys:

| #   | Key                      | Generated numeric ID | Role                                |
| --- | ------------------------ | -------------------- | ----------------------------------- |
| 1   | `TIME_FORMAT`            | `10000`              | Runtime setting                     |
| 2   | `TEMP_UNIT`              | `10001`              | Runtime setting                     |
| 3   | `TEMPERATURE`            | `10002`              | Weather data                        |
| 4   | `WEATHER_CONDITION`      | `10003`              | Weather data                        |
| 5   | `IS_DAY`                 | `10004`              | Weather data                        |
| 6   | `WEATHER_UPDATE_MINUTES` | `10005`              | Runtime setting                     |
| 7   | `DISPLAY_MODE`           | `10006`              | Runtime setting                     |
| 8   | `HR_SAMPLE_MINUTES`      | `10007`              | Runtime setting on `PBL_HEALTH`     |
| 9   | `STEPS_GOAL`             | `10008`              | Runtime setting on `PBL_HEALTH`     |
| 10  | `STEPS_GOAL_PRESET`      | `10009`              | Clay-only input                     |
| 11  | `STEPS_GOAL_CUSTOM`      | `10010`              | Clay-only input                     |
| 12  | `MAYBE_CURRENT_LOCATION` | `10011`              | Optional location data              |
| 13  | `JS_READY`               | `10012`              | PKJS readiness announcement         |
| 14  | `BATTERY_ORIENTATION`    | `10013`              | Runtime setting                     |
| 15  | `REFRESH`                | `10014`              | C request for climate/location      |

**Notes**

- Pebble assigns manifest-backed numeric message IDs from the `package.json` `messageKeys` order.
- Generated artifacts may list names alphabetically; numeric IDs still follow manifest order.
- Manual QA commands, harness helpers, PKJS, and C must follow the generated numeric mapping.

**Rules**

- Add new manifest-backed keys at the end unless a deliberate renumbering is being performed.
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
  -> request a refresh when JS_READY is present, then continue parsing
  -> parse settings tuples
  -> parse weather and location tuples
  -> parse one-shot health tuples on PBL_HEALTH
  -> watchface_apply_received_data(&data, settings)
  -> save settings if persisted fields changed
  -> request a refresh and reschedule if weather cadence changed
  -> update HealthService sample period if HR cadence changed
```

## Runtime mutation

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
| `TIME_FORMAT` | `WATCHFACE_UPDATE_TIME` |
| `TEMP_UNIT` | `WATCHFACE_UPDATE_CLIMATE` |
| `DISPLAY_MODE` | `WATCHFACE_REPAINT` |
| `BATTERY_ORIENTATION` | `WATCHFACE_UPDATE_BATTERY_ORIENTATION` |
| `WEATHER_UPDATE_MINUTES` | Persisted in C; requests a refresh and schedules the new interval |
| `HR_SAMPLE_MINUTES` | Persisted in C; HealthService sample period updated in `ataglance_comms_handler.c` |
| `STEPS_GOAL` | `WATCHFACE_UPDATE_HEALTH` |

Persistence uses Pebble persistent storage key `2` and writes the full `WatchfaceSettings` struct only when a validated persisted field changes. Stored settings are sanitized on load.

## Change Checklist

For a settings change, reconcile the affected owners and validate the complete path:

- Update `package.json` only when the AppMessage key set changes.
- Regenerate message-key output after key changes and confirm IDs match manual QA commands and harness helpers.
- Check `src/pkjs/config.json` fields, defaults, options, and capability filters.
- Check `src/pkjs/index.js` normalization, canonical outbound keys, and refresh-request handling.
- Update `src/modules/settings.h` and `src/modules/settings.c` for defaults, validation, persisted shape, or sanitization changes.
- Update `src/c/ataglance_message_parser.c` for tuple parsing.
- Update `src/c/ataglance_comms_handler.c` for inbox sizing, save triggers, or cadence side effects.
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
- [Validation](Validation.md) for emulator validation and QA evidence.
