# Settings and Configuration

This document covers settings, configuration transport, message-key mapping,
persistence, and validation for *At A Glance*.

Use this document to understand:

- the canonical setting catalog
- settings defaults
- valid ranges and enums
- Clay field mapping
- `messageKeys` linkage
- PKJS normalization rules
- persisted-vs-ephemeral distinction
- runtime side effects
- validation obligations when a setting changes

Key implementation details:

- Settings flow to and from the Clay configuration page through PKJS.
- `ataglance.c` is responsible for settings persistence and lifecycle management.
- Settings are transported using AppMessage, then parsed and interpreted in C.
- Display is updated with information as long as it is within range.

## Required Reading

- [ArchitectureLedger.md](ArchitectureLedger.md) for the runtime architecture.
- [UserInterface.md](UserInterface.md) for details on the full visual reference implementation.

## Settings Catalog

The runtime setting source of truth is `WatchfaceSettings` in
`src/modules/settings.h`. On health-capable builds, there are six persisted
runtime settings.

| Setting | C field | Canonical key | Default | Valid values | Persisted | Runtime effect |
| --- | --- | --- | --- | --- | --- | --- |
| 1. Time format | `time_format` | `TIME_FORMAT` | `0` / `TIME_FMT_24` | `0` 24-hour, `1` 12-hour | Yes | Refreshes time text |
| 2. Temperature unit | `temp_unit` | `TEMP_UNIT` | `0` / `TEMP_UNIT_F` | `0` Fahrenheit, `1` Celsius | Yes | Refreshes climate text |
| 3. Display mode | `display_mode` | `DISPLAY_MODE` | `0` / `DISPLAY_MODE_LIGHT_MONOCHROME` | `0`, `1` on B/W; `0`, `1`, `2`, `3` on color | Yes | Repaints watch face |
| 4. Weather cadence | `weather_update_minutes` | `WEATHER_UPDATE_MINUTES` | `15` | `15`, `30`, `45`, `60` | Yes | Updates PKJS weather schedule |
| 5. Heart-rate cadence | `hr_sample_minutes` | `HR_SAMPLE_MINUTES` | `15` | `10`, `15`, `30`, `60`, `120` | Yes on `PBL_HEALTH` | Updates HealthService sample period |
| 6. Steps goal | `steps_goal` | `STEPS_GOAL` | `10000` | `4000` through `32000` | Yes on `PBL_HEALTH` | Refreshes steps display |

Clay also exposes two settings-page inputs that are not canonical runtime settings:

| Clay input | Message key | Purpose | Crosses JS-to-C boundary |
| --- | --- | --- | --- |
| Steps goal preset | `STEPS_GOAL_PRESET` | Preset candidate for steps goal | No |
| Custom steps goal | `STEPS_GOAL_CUSTOM` | Optional custom override for steps goal | No |

PKJS collapses these two Clay inputs into the canonical `STEPS_GOAL` value
before sending settings to C.

The settings catalog does not include weather data or one-shot health override
keys. Those are AppMessage payloads, not persisted user settings.

## Source Of Truth Map

| Source | Owns |
| --- | --- |
| `package.json` | Pebble manifest `messageKeys` order and generated numeric IDs |
| `src/pkjs/config.json` | Clay settings page fields, labels, options, capability filters, and form defaults |
| `src/pkjs/index.js` | Clay result parsing, steps-goal normalization, weather-cadence scheduling, and settings send |
| `src/modules/settings.h` | Runtime setting enums, defaults, validation macros, and `WatchfaceSettings` shape |
| `src/modules/settings.c` | Defaults application, persisted read/write, and persisted-value sanitization |
| `src/c/ataglance.c` | AppMessage tuple parsing, settings-save trigger, and HealthService side effects |
| `src/modules/watchface_runtime_boundary.c` | Runtime setting validation, mutation, repaint/refresh decisions |
| `src/modules/watchface.c` | Settings consumption during repaint and module refresh |
| [docs/Settings_and_Configuration.md](Settings_and_Configuration.md) | Human-readable settings contract |

## Clay And PKJS Flow

Clay is configured in `src/pkjs/config.json` and opened from PKJS:

```text
Pebble showConfiguration event
  -> clay.generateUrl()
  -> user edits settings page
  -> webviewclosed event
  -> clay.getSettings(e.response)
  -> PKJS normalizes settings
  -> Pebble.sendAppMessage(dict)
```

The current settings page is shown here as evidence of the configuration
surface, not as the visual specification:

![At A Glance settings page](assets/screenshots/at-a-glance-settings.png)

### PKJS Normalization

PKJS sends most Clay-backed settings directly after Clay maps their keys.

Steps goal is different:

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

After computing `STEPS_GOAL`, PKJS deletes `STEPS_GOAL_PRESET` and
`STEPS_GOAL_CUSTOM` from the outbound dictionary. C should not budget AppMessage
inbox space for those two Clay-only inputs.

Weather cadence is also applied in PKJS. When Clay returns
`WEATHER_UPDATE_MINUTES`, PKJS updates the weather schedule and asks for a
weather refresh.

## AppMessage And Message-Key Contract

`package.json` owns the manifest-backed message-key list.

Current manifest-backed keys:

| # | Key | Generated numeric ID | Role |
| --- | --- | --- | --- |
| 1 | `TIME_FORMAT` | `10000` | Runtime setting |
| 2 | `TEMP_UNIT` | `10001` | Runtime setting |
| 3 | `TEMPERATURE` | `10002` | Weather data |
| 4 | `WEATHER_CONDITION` | `10003` | Weather data |
| 5 | `IS_DAY` | `10004` | Weather data |
| 6 | `WEATHER_UPDATE_MINUTES` | `10005` | Runtime setting |
| 7 | `DISPLAY_MODE` | `10006` | Runtime setting |
| 8 | `HR_SAMPLE_MINUTES` | `10007` | Runtime setting on `PBL_HEALTH` |
| 9 | `STEPS_GOAL` | `10008` | Runtime setting on `PBL_HEALTH` |
| 10 | `STEPS_GOAL_PRESET` | `10009` | Clay-only input |
| 11 | `STEPS_GOAL_CUSTOM` | `10010` | Clay-only input |

Pebble assigns manifest-backed numeric message IDs from the `package.json`
`messageKeys` order.

Generated C and JS artifacts may list keys in sorted-name order. That listing
order is not the numeric assignment rule. Manual QA commands, harness helpers,
PKJS, and C must follow the generated numeric mapping.

Rules:

- Add new manifest-backed keys at the end unless a deliberate renumbering is
  being performed.
- Recheck generated key output after any `messageKeys` change.
- Update JS, C parsing, inbox sizing, harness helpers, manual QA commands, and
  docs in the same change.
- Do not assume generated file order is the same as numeric order.

### QA-Only One-Shot Keys

The current one-shot health keys are runtime-only constants:

| Key | Numeric ID | Purpose |
| --- | --- | --- |
| `WATCHFACE_ONESHOT_MESSAGE_KEY_BPM` | `10020` | One-shot BPM override |
| `WATCHFACE_ONESHOT_MESSAGE_KEY_STEPS` | `10021` | One-shot steps override |

These are not Clay settings, not manifest-backed keys, and not persisted. They
exist for focused QA and are consumed once by the health refresh path.

[Validation.md](Validation.md) owns the operational command examples for these keys.
This document references them only to keep the settings, data, and QA payloads
separate.

## Watch Runtime And Persistence Flow

### Startup

```text
init()
  -> settings_load(&s_settings)
       -> apply defaults
       -> read persisted data if present
       -> sanitize values
  -> watchface_create(window, &s_settings)
       -> module_create(params, relevant_loaded_setting)
  -> open AppMessage
  -> send loaded WEATHER_UPDATE_MINUTES to PKJS
```

### Settings receipt

```text
inbox_received_callback(iter)
  -> copy previous settings
  -> parse settings tuples
  -> parse weather tuples
  -> parse health settings tuples on PBL_HEALTH
  -> watchface_apply_received_data(&data, &s_settings)
  -> save settings if persisted fields changed
  -> update HealthService sample period if HR cadence changed
```

## Runtime mutation

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
| `WEATHER_UPDATE_MINUTES` | Persisted in C; schedule update is managed in PKJS |
| `HR_SAMPLE_MINUTES` | Persisted in C; HealthService sample period updated in `ataglance.c` |
| `STEPS_GOAL` | `WATCHFACE_UPDATE_HEALTH` |

Persistence uses Pebble persistent storage key `2` and writes the full
`WatchfaceSettings` struct. Stored settings are sanitized on load.

## Validation Requirements

When a setting changes, validate the whole path that changed.

Minimum checks:

- `package.json` `messageKeys` order still matches expected numeric IDs.
- `src/pkjs/config.json` exposes the intended Clay field and capability filter.
- `src/pkjs/index.js` sends the intended canonical key.
- `src/c/ataglance.c` parses the tuple and budgets the inbox size correctly.
- `src/modules/settings.h` defines the default and valid range.
- `src/modules/settings.c` sanitizes persisted data correctly.
- `src/modules/watchface_runtime_boundary.c` validates and mutates the setting.
- The runtime refresh or repaint effect matches the setting's intended scope.
- Manual QA commands and harness helpers use the current numeric mapping.

Read [Build.md](Build.md) for build activation and [Validation.md](Validation.md)
for emulator validation.

## Change Checklist

For any settings change:

- Update `package.json` only when the AppMessage key set changes.
- Regenerate and inspect generated message-key output after key changes.
- Update `src/pkjs/config.json` for settings-page shape changes.
- Update `src/pkjs/index.js` for Clay normalization or scheduling changes.
- Update `src/modules/settings.h` and `src/modules/settings.c` for defaults,
  validation, persisted shape, or sanitization changes.
- Update `src/c/ataglance.c` for tuple parsing, inbox sizing, save triggers, or
  side effects.
- Update `src/modules/watchface_runtime_boundary.c` for runtime validation,
  mutation, repaint, or refresh behavior.
- Update [docs/Settings_and_Configuration.md](Settings_and_Configuration.md).
- Update [Validation.md](Validation.md) only when validation flow changes.
- Update [UserInterface.md](UserInterface.md) only when visible settings evidence or visual
  consequences change.
- Run the relevant build and validation path, or record the validation gap.

## Further Reading

- [Build.md](Build.md) for build activation.
- [Validation.md](Validation.md) for emulator validation and QA evidence.
- [Contributing.md](Contributing.md) for contributor workflow, validation, and review discipline.
