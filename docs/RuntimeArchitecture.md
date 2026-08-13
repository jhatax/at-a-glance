# Runtime Architecture

This is the current source of truth for _At A Glance's_ runtime architecture: execution flow, ownership, boundaries, lifecycle, and source organization.

## Adjacent

- [ProductInvariants](ProductInvariants.md) states invariants to achieve goals across devices.

## Watch face conceptual layers

### Layers

Layers have been implemented to satisfy visual display and runtime-architecture invariants.

1. Pebble OS adapter: `ataglance.c` => main window & persisted settings lifecycle owner
2. Pebble events and updates adapter: `watchface_runtime_boundary.c` => translate system events into watch face vocabulary
3. Watch face display: `watchface.c` => visual owner and lifecycle manager of supporting modules

- `layout_architect.c` => Prepares the surface - `layout_stylist.c` => Associates styles and fonts with visual channels

4. Watch face tiles: `time, date, battery, climate, steps, bpm` => display information and state
   - `climate` also owns the optional centered location text and its source buffer.

### Core Concepts

- The watch face display is organized using tiles (strata). Each tile can include multiple visual channels (sub-strata: icon, text, progress-bar).
- Information updates are relayed from Pebble OS (subscriptions, user-settings changes, major events, emulator messages).
- Updates are scoped to new information only; multiple strata changes are batched.
- Use of repaint is limited to maximize battery life.

## Initialization and Runtime Flows

These **four flows are the same** for supported capabilities of all Pebble devices, thereby satisfying the **invariant of architectural parity** for and compatibility with all devices.

### 1. Initialization flow

```text
ataglance.c
  -> window_set_window_handlers(...)
  -> window_stack_push(window, true)
       -> main_window_load(window)
            -> settings_load(&settings)
            -> watchface_create(window, &settings)
                 -> window_get_root_layer(window)
                 -> layout_watchface_initialize(width, height, &surface)
                      -> memset(surface, 0, sizeof(*surface))
                      -> calculate active blueprint and final geometry
                      -> store compact/full on surface.style
                 -> layout_watchface_update_palette(&surface.style, display_mode)
                 -> window_set_background_color(...)
                 -> layout_watchface_initialize_fonts(...)
                 -> layout_watchface_load_custom_fonts(...)
                 -> feature_module_create(root, prepared surface strata...)
                 -> require date/time/battery/climate strata to succeed
                 -> watchface_refresh(WATCHFACE_UPDATE_ALL_STRATA)
            -> subscribe tick, battery, connection, and health services
            -> register AppMessage callbacks
            -> app_message_open(...)
                 -> retry open with AppTimer only if opening fails
```

### 2. Runtime event flow

```text
service callback in ataglance.c
  -> build WatchfaceEventData
  -> watchface_apply_received_data()

AppMessage inbox handler
  -> parse WatchfaceEventData
  -> ataglance_apply_received_data()
       -> watchface_apply_received_data()
       -> apply_setting_data(...)
            -> mutate settings for time format, temp unit, display mode, steps goal
            -> accumulate targeted refresh mask
            -> mark repaint only for a valid display-mode change
       -> apply_weather_or_location_data(...)
            -> push complete or unavailable climate state
            -> request climate refresh when any weather field was received
            -> copy the bounded location payload into climate source state
            -> request the location text refresh
       -> apply_subscribed_service_updates(...)
            -> translate tick, battery, and health callbacks into refresh masks
       -> apply_health_setting_data(...)
            -> mutate HR sample setting only
       -> apply_oneshot_health_data(...)
            -> queue one-shot BPM/steps overrides
            -> request health refresh
       -> if repaint
            -> watchface_repaint()
                 -> layout_watchface_update_palette(&surface.style, display_mode)
                 -> window_set_background_color(...)
                 -> watchface_refresh(WATCHFACE_UPDATE_ALL_STRATA)
       -> else if refresh mask != WATCHFACE_UPDATE_NONE
            -> watchface_refresh(mask)
                 -> refresh only the addressed strata that were created
```

### 3. Settings persistence and side effects

```text
AppMessage inbox handler
  -> detect optional JS_READY synchronization sentinel
  -> send persisted WEATHER_UPDATE_MINUTES when JS_READY is received
  -> parse settings, weather, location, health settings, and one-shot health tuples
  -> ataglance_apply_received_data(&data)
       -> watchface_apply_received_data(&data, &settings)
  -> if any persisted setting changed
       -> settings_save(&settings)
  -> if hr_sample_minutes changed
       -> apply_hr_sample_period()
```

### 4. Weather-cadence synchronization:

```text
src/pkjs/index.js ready
  -> start weather/location refresh at the 15-minute default
  -> enqueue JS_READY through the control AppMessage lane

AppMessage inbox handler in ataglance.c
  -> detect JS_READY
  -> enqueue persisted WEATHER_UPDATE_MINUTES through the C outbox

src/pkjs/index.js appmessage handler
  -> receive the persisted cadence
  -> applyWeatherUpdateMinutes(minutes)
  -> reschedule weather/location polling from the watch-loaded value
```

## Prepared Surface

The watch face is organized around a prepared `WatchfaceSurface` using `blueprints` customized for watch face geometry and shape. Two device categories have been defined:

1. `full` [`gabbro`, `emery`]
2. `compact` [`aplite`, `flint`, `diorite`, `chalk`, `basalt`]

The surface is prepared as follows:

1. Layout initialization clears and prepares the caller-owned surface from scratch.
2. Layout placement is calculated based on platform-specific constants.
3. Feature modules consume prepared substrata, frames, fonts, palettes, and layout policy.
4. A renderer supplied by the watch face places components on screen based on layout policy.

Current visual placement, palette, and typography are evidenced as screenshots in [UserInterface](UserInterface.md).

## Architect

- Owns geometry and compact/full classification.
- Resolves placement from defined layout `blueprint` for the active platform.

## Stylist

- `layout_watchface_update_palette()` resolves palette and font selection.
- Owns display-mode styling decisions.
- Delegates strata / information styling to modules.

## Watchface Runtime

- `ataglance.c` owns Pebble lifecycle, services, persistent settings, AppMessage parsing, and transport dispatch.
- `watchface_runtime_boundary.c` interprets runtime events, mutates settings and domain state, and decides repaint versus refresh.
- `watchface.c` owns the live `WatchfaceSurface`, feature-module lifecycle, and visual dispatch.
- `watchface_apply_received_data()` is the single runtime ingress for watchface updates.

## Modules

- Own Pebble layer lifecycle and source state.
- `create()` APIs receive only the prepared substrata, frames, and fonts they need.
- `refresh()` APIs receive the current palette and narrow runtime payloads where needed.
- Do not retain `WatchfaceSurface*` and global style.

Two classes of strata have been defined: required strata are [`time`, `battery`, `date`, `climate`], and all other strata are optional. Required strata have been selected based on capabilities available on all devices.

Additionally,

- **only text elements** are considered as required within each strata.
- Placement of text is not impacted if icons cannot be created, are disabled, or cannot be displayed.
- If `Required` strata cannot be created, watch face initialization fails and control is returned to the Pebble OS.

This categorization satisfies the visual invariant of information hierarchy and display consistency for all supported devices.

## Reference Architecture Of A Module

Feature modules follow the same lifecycle shape, though their rendering details can differ based on what they display.

### Module creation flow

```text
watchface_create()
  -> module_create(root, prepared substrata, font or narrow geometry)
       -> validate required inputs
       -> create required Pebble layers and resources
       -> configure update procs, fonts, frames, alignment, and static layer state
       -> add created layers to the root layer
       -> retain only module-owned handles and source state
       -> return success only after required layer/resource creation succeeds
```

### Runtime data flow

```text
service callback or AppMessage callback
  -> ataglance.c parses raw Pebble input
  -> watchface_runtime_boundary.c validates and interprets runtime facts
  -> optional module source-state setter
       -> module stores source state only
  -> watchface_refresh(mask)
       -> module_refresh(active palette, narrow runtime inputs)
            -> read current source state or Pebble service state
            -> derive render text, icon, progress, and live colors locally
            -> update only module-owned layers
```

### Module destroy flow

```text
watchface_destroy()
  -> module_destroy()
       -> destroy module-owned layers and resources
       -> clear retained handles
       -> clear queued one-shot or source state when needed
```

### Current module variants

- Text-only modules such as date and time own a text layer and refresh text from time/date state plus the active palette.
- Service-backed modules such as battery, BPM, and steps read Pebble service state during refresh, then map that source state to text, progress, icon, or live colors.
- Transport-backed modules such as climate receive weather and optional location source state through setters, then render temperature, glyph, and location text during refresh.
- QA-only one-shot health setters queue a single source-state override that is consumed by the next health refresh and cleared during module or watch face teardown.

### Module boundaries

- Modules own their layers, update procs, source state, dynamic color policy, and destroy path.
- Modules consume prepared substrata; they do not recalculate layout policy.
- Modules consume the active palette; they do not choose display mode.
- Modules accept narrow runtime inputs; they do not accept `WatchfaceSurface`.
- Modules may expose a setter only when runtime ingress needs to update module-owned source state before refresh.

### Reference Example: Steps Module

`steps.c` is the most complex current feature-module example because it owns text, bitmap icon rendering, progress rendering, HealthService reads, a settings-driven goal, and a QA-only one-shot override.

**Steps creation flow**

```text
watchface_create()
  -> steps_module_create(root, text substratum, icon substratum, progress frame, font)
       -> require root, text, progress frame, and font
       -> create required steps text layer
       -> initialize source state, goal, threshold, icon color cache, and one-shot state
       -> if icon substratum exists
            -> load RESOURCE_ID_WALK bitmap
            -> create optional icon layer with steps_icon_update_proc
            -> destroy bitmap if optional icon layer creation fails
       -> create optional progress layer from prepared progress frame
       -> if icon is absent
            -> narrow progress frame to text width
       -> return true once required text layer exists
```

**Steps refresh flow**

```text
watchface_refresh(WATCHFACE_UPDATE_HEALTH)
  -> steps_module_refresh(active palette, settings.steps_goal)
       -> store current steps goal
       -> calculate approaching-goal threshold at 70 percent
       -> update module palette from the active display palette
       -> update_steps()
            -> read today's HealthService step sum when available
            -> if one-shot steps override is queued
                 -> replace HealthService value for this refresh only
                 -> clear queued one-shot value
            -> apply_steps_value(steps, availability)
                 -> store source state and availability
                 -> calculate live steps color
                 -> render text or unavailable token
                 -> mark icon layer dirty
                 -> mark progress layer dirty
```

**Steps render callbacks**

```text
steps_icon_update_proc()
  -> recolor WALK bitmap when live steps color changes
  -> draw bitmap
  -> draw unavailable slash when steps are unavailable

steps_progress_update_proc()
  -> draw background-filled track with live outline
  -> fill completed width from steps / goal
  -> clear track to background when steps are unavailable
```

**Steps one-shot flow (via AppMessage)**

```text
pebble send-app-message ... 10021=<steps>
  -> ataglance.c parses WATCHFACE_ONESHOT_MESSAGE_KEY_STEPS
  -> watchface_runtime_boundary.c calls steps_module_oneshot_set_steps()
  -> next steps_module_refresh()
       -> consumes queued value
       -> clears queued value
       -> renders exactly one health refresh from the override
```

**Steps destroy flow**

```text
watchface_destroy()
  -> steps_module_destroy()
       -> destroy text layer
       -> destroy WALK bitmap
       -> destroy icon layer
       -> destroy progress layer
       -> clear text buffer, source state, palette, colors, goal, and one-shot state
```

The steps module demonstrates the intended boundary:

- `watchface.c` decides when health refreshes happen.
- `settings.steps_goal` is passed as a narrow runtime input.
- `steps.c` owns HealthService interpretation, threshold calculation, progress fill math, bitmap recoloring, unavailable rendering, and one-shot consumption.
- `steps.c` is only aware of its own placement settings and palette.

## Runtime resolution of palettes and icon colors

- Palette selection is driven by user-preferences.
- When the palette is changed, the watch face is repainted in layers, back-to-front.
- Bitmap palette mutation -- to convey status -- requires palettized PNG resources.

## Capability Guards

- Treat platform and runtime capabilities as conditional. Gate color, health, shape-specific layout, loaded resources, and phone-provided data through the Pebble SDK macros, manifest capabilities, or runtime validation before relying on them.
- Prefer `PBL_COLOR`, `PBL_BW`, `PBL_HEALTH`, `PBL_RECT`, `PBL_ROUND`, and `PBL_API_EXISTS()`.
- Do not assume that a supported target has every capability that a neighboring target has.

## AppMessage Runtime Coverage

`ataglance.c` is the Pebble container and service adapter. The private C message adapter owns AppMessage parsing, sizing, opening, callbacks, and the watch-to-phone cadence synchronization send. Responsibilities are split as follows:

1. the message parser translates raw tuples into `WatchfaceEventData`;
2. the message handler owns AppMessage transport and delegates parsed data;
3. `ataglance.c` owns lifecycle, persistence triggers, and service callbacks;
4. `watchface_apply_received_data()` owns runtime interpretation.

### Inbound AppMessage coverage

- **Settings tuples**: time format, temperature unit, display mode, weather cadence, heart-rate cadence, and steps goal.
- **Weather tuples**: temperature, weather condition, and `is_day`.
- **Location tuple**: optional current-location text from PKJS; an empty value clears the location text.
- **QA-only one-shot health tuples**: heart-rate and steps overrides.

### Outbound AppMessage coverage

- When PKJS sends `JS_READY`, the watch sends the persisted `WEATHER_UPDATE_MINUTES` value back through the C outbox so the phone-side weather polling schedule resumes from watch storage.
- PKJS uses independent serialized lanes:
  - Weather
  - Location
  - Control
- Each lane retries failed `Pebble.sendAppMessage()` calls with bounded timer backoff.
- Lane separation prevents a stalled weather or location send from blocking settings and control traffic.

Settings, Clay field mapping, generated message-key numbering, PKJS normalization, and persistence rules are owned by [SettingsandConfiguration](SettingsandConfiguration.md). This section owns only the architecture-level transport route.

### Climate-specific transport behavior

- The phone companion requests current weather from Open-Meteo on the configured cadence. If geolocation or either network request fails, PKJS logs the failure and leaves the watch's current weather/location state unchanged.
- PKJS sends the resolved location text through `MAYBE_CURRENT_LOCATION`; the runtime passes it to `climate.c`, which owns the location buffer and text layer.
- Temperature is sent to the watch in Celsius tenths and rendered according to settings.
- `weather_code` and `is_day` are sent to C for glyph selection.
- Weather tuple flow:
  1. A received tuple triggers a climate refresh.
  2. The runtime applies complete climate state only when temperature, condition, and `is_day` are all parsed.
  3. `climate.c` validates the parsed domain values.
  4. Incomplete or invalid packets clear prior weather state and render the unavailable vocabulary.

## Lifecycle Priorities

1. Evaluate architecture changes using embedded platform affordances and limits.
2. Build for security, usability, performance, code readability, and future maintenance.
3. Validate before confirming feature completion.
4. Heap usage should be exceptional; release memory and initialize to NULL.
5. Do not leave stale pointers, partial layer ownership, unmatched resource lifetimes, buffer overflows, or hidden failure branches.

## Read Next

- [WatchfaceImplementationFlow](WatchfaceImplementationFlow.md) for implementation flow, source responsibilities, and interconnections.
- [SettingsandConfiguration](SettingsandConfiguration.md) for the settings catalog, Clay mapping, message-key contract, persistence, and validation obligations
- [Validation](Validation.md) for validation contract, evidence, and release profiles.
