# Watchface Runtime Boundary Migration Handoff

Archived planning artifact. This document captured the scaffolding for the
runtime-boundary migration, but it is no longer the active authority. Use
`ARCHITECTURE_LEDGER.md`, `RAID_LOG.md`, module headers, and live source for
current architecture decisions and invariants.

## Summary

This is an architecture boundary migration, not a narrow FR-104 weather patch.
The goal is to move watchface runtime interpretation out of `ataglance.c` and
behind one watchface event boundary.

Architecture invariants:
1. `ataglance.c` remains the Pebble app container: lifecycle, services,
AppMessage tuple lookup, transport parsing, settings persistence, and HR
service period application.
2. `watchface.c` becomes the runtime engine: it interprets received
typed data, validates domain/settings values, mutates settings, applies module
source state, computes refresh/repaint behavior, and performs visual updates
internally.
3. There should be no heap allocation, no generic dispatch table, no `void*`, no callback lookup
framework.
4. Do NOT CODIFY hidden runtime policy in parser helpers.

## Architecture Decisions, roles and responsibilities based on the RACI matrix

- `ataglance.c`: Responsible for adapting external inputs into
  `WatchfaceEventData`:
  - `dict_find()`
  - tuple type/int conversion
  - set received and parsed masks
  - assign raw parsed `int` values
  - log transport parse failures
  - convert Pebble service callbacks into event bits

- `ataglance.c` must not:
  - validate settings values
  - compute weather completeness
  - mutate weather/debug/module state
  - build `WatchfaceUpdateMask`
  - decide repaint vs refresh
  - call scalar weather/debug watchface setters

- `watchface_runtime_boundary.c` owns watchface event interpretation:
  - settings validation and mutation
  - weather atomicity
  - malformed/partial weather fallback
  - debug health application
  - Pebble service event refresh mapping
  - runtime update mask computation
  - render refresh/repaint decisions

- `watchface.c` remains the runtime surface/lifecycle orchestrator:
  - create/destroy order
  - live `WatchfaceSurface` ownership
  - font/palette application
  - module refresh/repaint dispatch

- `ataglance.c` still owns:
  - `settings_save(&s_settings)`
  - `health_service_set_heart_rate_sample_period(...)`
  - service subscription lifecycle
- `ataglance.c` determines whether to save/apply HR period by comparing
  `WatchfaceSettings` before and after `watchface_apply_received_data()`, not by
  interpreting received fields.
- All watchface updates from `ataglance.c` enter through
  `watchface_apply_received_data()`. `ataglance.c` knows that a watchface
  exists; it does not know which strata or feature modules should refresh.


## Public API And Data Shapes

Add the event-data ingress types to `watchface.h`.

Use two masks:

```c
typedef enum {
  WATCHFACE_DATA_NONE = 0,
  WATCHFACE_DATA_TIME_FORMAT = 1 << 0,
  WATCHFACE_DATA_TEMP_UNIT = 1 << 1,
  WATCHFACE_DATA_TEMPERATURE = 1 << 2,
  WATCHFACE_DATA_WEATHER_CONDITION = 1 << 3,
  WATCHFACE_DATA_IS_DAY = 1 << 4,
  WATCHFACE_DATA_HR_SAMPLE_MINUTES = 1 << 5,
  WATCHFACE_DATA_DISPLAY_MODE = 1 << 6,
#if defined(PBL_HEALTH) && ATAGLANCE_DEBUG
  WATCHFACE_DATA_DEBUG_BPM = 1 << 7,
  WATCHFACE_DATA_DEBUG_STEPS = 1 << 8,
#endif
} WatchfaceDataMask;
```

For AppMessage fields, `received` means tuple presence and `parsed` means
transport parse success. For Pebble service events, `received` and `parsed`
both mark event delivery. Neither means domain validity.

```c
typedef struct {
  WatchfaceDataMask received;
  WatchfaceDataMask parsed;

  int time_format;
  int temp_unit;
  int temperature_celsius_tenths;
  int weather_condition;
  int is_day;
  int hr_sample_minutes;
  int display_mode;

#if defined(PBL_HEALTH) && ATAGLANCE_DEBUG
  int debug_bpm;
  int debug_steps;
#endif
} WatchfaceEventData;
```

Use `int` for incoming values. Runtime narrows to `uint8_t` only after
validation.

Add the public runtime ingress:

```c
void watchface_apply_received_data(
    const WatchfaceEventData* data,
    WatchfaceSettings* settings);
```

Remove these public APIs from `watchface.h` after migration:

```c
void watchface_set_temperature(int celsius_tenths);
void watchface_set_weather_condition(int weather_condition);
void watchface_set_is_day(bool is_day);
```

Remove public debug setters from `watchface.h` after debug data moves through
`WatchfaceEventData`:

```c
void watchface_debug_set_bpm(int bpm);
void watchface_debug_set_steps(int steps);
```

Keep `watchface_debug_clear_health()` private to `watchface.c` if still needed
during destroy, or make it `static`.

## AppMessage Parsing Flow

In `inbox_received_callback()`:

1. Create stack-local data packet:

```c
WatchfaceEventData data = {0};
WatchfaceSettings previous_settings = s_settings;
```

2. Parse by category with direct typed parser functions, not callback tables:
   - `parse_settings_data(iter, &data)`
   - `parse_weather_data(iter, &data)`
   - `parse_health_settings_data(iter, &data)` under `PBL_HEALTH`
   - `parse_debug_health_data(iter, &data)` under `PBL_HEALTH && ATAGLANCE_DEBUG`

3. Parser behavior:
   - If `dict_find()` returns a tuple, set the relevant `data.received` bit.
   - If the tuple transport-parses to `int`, set the relevant `data.parsed` bit
     and assign the raw `int`.
   - If tuple exists but fails transport parsing, log a warning and do not set
     `parsed`.
   - Do not call settings validators.
   - Do not check weather completeness.
   - Do not set runtime/render masks.
   - Do not mutate `s_settings`.

4. Call runtime:

```c
watchface_apply_received_data(&data, &s_settings);
```

5. After runtime returns, `ataglance.c` compares `previous_settings` to
   `s_settings`:
   - if any persisted setting changed, call `settings_save(&s_settings)`;
   - under `PBL_HEALTH`, if `hr_sample_minutes` changed, call
     `apply_hr_sample_period()`.

No repaint/refresh call occurs in `ataglance.c` after event delivery.

## Runtime Interpretation Flow

Inside `watchface_apply_received_data()` in `watchface_runtime_boundary.c`:

- Return immediately if `data == NULL` or `settings == NULL`.
- Create private local masks:
  - `WatchfaceRuntimeUpdateMask runtime_updates = WATCHFACE_RUNTIME_UPDATE_NONE;`
  - `WatchfaceUpdateMask refresh = WATCHFACE_UPDATE_NONE;`
  - `bool repaint = false;`
- `WatchfaceRuntimeUpdateMask` should be private to
  `watchface_runtime_boundary.c`.

Settings handling:

- If `parsed` includes `TIME_FORMAT` and value passes `TIME_FORMAT_VALID`,
  compare to `settings->time_format`.
  - If changed, mutate settings and set `refresh |= WATCHFACE_UPDATE_TIME`.
- If `parsed` includes `TEMP_UNIT` and value passes `TEMP_UNIT_VALID`, compare
  to `settings->temp_unit`.
  - If changed, mutate settings and set `refresh |= WATCHFACE_UPDATE_CLIMATE`.
- If `parsed` includes `DISPLAY_MODE` and value passes `DISPLAY_MODE_VALID`,
  compare to `settings->display_mode`.
  - If changed, mutate settings and set `repaint = true`.
- Under `PBL_HEALTH`, if `parsed` includes `HR_SAMPLE_MINUTES` and value passes
  `HR_SAMPLE_MINUTES_VALID`, compare to `settings->hr_sample_minutes`.
  - If changed, mutate settings.
  - Do not call Pebble health service here.

Invalid settings values:

- Log a warning from `watchface.c`.
- Do not mutate settings.
- Do not refresh/repaint for that invalid field.

Weather handling:

- Compute weather input state from masks:
  - `weather_received`: any of `TEMPERATURE`, `WEATHER_CONDITION`, `IS_DAY` is
    in `received`.
  - `weather_parsed_complete`: all three are in `parsed`.
- If no weather fields received: no climate source change.
- If any weather field received but not all three parsed: send an incomplete
  `ClimateUpdate` packet and set `refresh |= WATCHFACE_UPDATE_CLIMATE`.
- If all three parsed:
  - call `climate_module_set_weather(&update)` with `is_complete = true`;
  - climate owns domain validation, applying valid weather, and clearing stale
    weather for incomplete or invalid packets;
  - set `refresh |= WATCHFACE_UPDATE_CLIMATE`.
- Runtime does not validate `is_day` as a climate-domain value. Climate owns
  `is_day` validity and fallback.

Debug health:

- Under `PBL_HEALTH && ATAGLANCE_DEBUG`, if debug BPM or debug steps are parsed:
  - apply them inside `watchface.c` to BPM/steps debug module APIs;
  - set `refresh |= WATCHFACE_UPDATE_HEALTH`.
- Do not expose debug health setters publicly in `watchface.h`.

Visual application:

- If `repaint` is true, call `watchface_repaint()` internally.
- Else if `refresh != WATCHFACE_UPDATE_NONE`, call `watchface_refresh(refresh)`
  internally.
- `ataglance.c` must not receive or inspect this refresh decision.

## Climate API Migration

Replace scalar weather setters with atomic climate APIs:

```c
typedef struct {
  bool is_complete;
  int celsius_tenths;
  int weather_condition;
  int is_day;
} ClimateUpdate;

void climate_module_set_weather(ClimateUpdate* update);
```

`climate.c` responsibilities:

- owns weather source-state fields;
- receives atomic climate update packets from the runtime boundary;
- treats incomplete packets as unavailable weather;
- validates temperature range and condition range;
- validates `is_day`;
- owns sentinel/fallback values;
- sets unavailable state atomically;
- preserves existing rendering behavior: unavailable temperature text and
  unknown/unavailable glyph.

Delete or make private after migration:

- `climate_module_set_temperature`
- `climate_module_set_condition`
- `climate_module_set_is_day`

## Message Loss And Stale State Rules

- No AppMessage: no runtime state change.
- Complete valid weather message: apply fresh weather atomically.
- Partial weather message: clear stale weather by setting unavailable weather
  atomically.
- Malformed weather tuple present: clear stale weather by setting unavailable
  weather atomically.
- Complete weather message with invalid domain value: clear stale weather
  through climate fallback.
- Partial settings message: apply only valid parsed settings fields
  independently.
- Malformed setting tuple: ignore that field and log warning.
- Display mode changes repaint internally in runtime.
- Temp unit changes refresh climate internally in runtime.
- Time format changes refresh time internally in runtime.
- HR sample setting changes mutate settings; `ataglance.c` observes changed
  settings and applies the service period after runtime returns.

## Files Expected To Change

Primary implementation files:

- `src/modules/watchface.h`
- `src/modules/watchface_runtime_boundary.c`
- `src/modules/watchface.c`
- `src/modules/climate.h`
- `src/modules/climate.c`
- `src/c/ataglance.c`

Documentation/tracker files:

- `FINAL_REVIEW_LEDGER.md`
- `ARCHITECTURE_LEDGER.md`

Do not include unrelated cleanup in this migration. The helper macro fix, Clay
copy fix, and ledger closure updates should remain separate unless already
staged intentionally.

## Validation Plan

Static checks:

- `rg "watchface_set_temperature|watchface_set_weather_condition|watchface_set_is_day" src`
  - should return no public/live callers after migration.
- `rg "watchface_debug_set_bpm|watchface_debug_set_steps" src`
  - should not appear in public headers.
- `rg "WATCHFACE_UPDATE_.*=" src/c/ataglance.c`
  - AppMessage path should not build render masks.
- `rg "watchface_repaint\\(|watchface_refresh\\(" src/c/ataglance.c`
  - AppMessage callback should not call these after migration; service callbacks
    may still call `watchface_refresh()`.
- `git diff --check`
- `pebble build`

Manual emulator checks:

- Install on Emery.
- Send complete weather AppMessage and confirm climate text/icon update.
- Send only `TEMPERATURE` and confirm weather becomes unavailable, not mixed with
  stale condition/day.
- Send malformed or invalid weather values and confirm weather becomes
  unavailable.
- Send `TIME_FORMAT` and confirm time refreshes.
- Send `TEMP_UNIT` and confirm climate refreshes.
- Send `DISPLAY_MODE` and confirm runtime repaints internally.
- Send `HR_SAMPLE_MINUTES` on health-capable target and confirm settings persist
  and HR sample period is applied by `ataglance.c`.
- With `ATAGLANCE_DEBUG=1`, send debug BPM/steps and confirm health strata update
  without public debug setter leakage.

## Key Invariants And Guardrails For The Coding Agent

- This is an architecture boundary migration, not a tactical FR-104 patch.
- `ataglance.c` is the app container and transport parser; it is not the runtime.
- `watchface_runtime_boundary.c` implements the watchface event runtime
  boundary.
- `watchface.c` remains the surface/lifecycle orchestrator and must not become
  the implementation bucket for boundary interpretation.
- `watchface_runtime_boundary.c` must not access `watchface.c` statics.
- Runtime-boundary helpers should receive the exact state they need to inspect
  or mutate.
- For AppMessage fields, `WatchfaceEventData.received` means tuple presence.
- For AppMessage fields, `WatchfaceEventData.parsed` means transport parse
  success.
- For Pebble service events, both masks mark event delivery.
- Neither `received` nor `parsed` means domain validity.
- Runtime validation and grouping happen in `watchface_runtime_boundary.c`.
- Weather is atomic: any received-but-incomplete or malformed weather packet
  clears stale weather.
- `ataglance.c` must not compute `WatchfaceRuntimeUpdateMask` or
  `WatchfaceUpdateMask`.
- `ataglance.c` must not decide repaint vs refresh for watchface events.
- `ataglance.c` may persist settings only by comparing pre/post
  `WatchfaceSettings`.
- `ataglance.c` may apply HR service period only by comparing pre/post
  `hr_sample_minutes`.
- `settings_save()` stays outside `watchface.c`.
- Pebble health service calls stay outside `watchface.c`.
- No heap allocation.
- No `void*`.
- No function-pointer dispatch table.
- No generic runtime dictionary/bag.
- Keep parsing direct, typed, and grouped by message category.
- Keep all health code behind `PBL_HEALTH`.
- Keep all debug health transport/application behind
  `PBL_HEALTH && ATAGLANCE_DEBUG`.
- Remove public scalar weather setters after migration to avoid a second update
  path.
- Do not change AppMessage keys, persisted settings layout, Clay schema, visual
  layout, fonts, colors, or platform list in this slice.
