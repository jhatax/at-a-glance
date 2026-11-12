# Runtime Architecture

This document maps _At A Glance's_ runtime execution, ownership, boundaries, and lifecycle. Live source defines current behavior.

## Adjacent

- [ProductInvariants](ProductInvariants.md) defines the product properties the runtime must preserve across devices.

## Watch face conceptual layers

| Responsibility | Owner | Role |
| --- | --- | --- |
| Pebble lifecycle | `ataglance.c` | Own the window, load settings, register callbacks, subscribe to services, and cancel refresh/retry timers on unload. |
| Communications | `ataglance_comms_handler.c` | Open AppMessage, handle inbox/outbox callbacks, schedule refresh requests and retries, and apply persistence and service side effects after runtime validation. |
| Message parsing | `ataglance_message_parser.c` | Convert tuples into typed event data and receipt/parse masks. |
| Runtime decisions | `watchface_runtime_boundary.c` | Validate and apply settings, build domain updates, and choose repaint or targeted refresh. |
| Visual coordination | `watchface.c` | Own the live surface, manage feature lifecycles, and dispatch visual updates. |
| Geometry | `layout_architect.c` | Calculate the prepared surface from platform layout constants. |
| Style | `layout_stylist.c` | Resolve palettes and fonts for the selected display mode and layout. |
| Feature rendering | `time`, `date`, `battery`, `climate`, `steps`, and `bpm` modules | Own feature layers, source state, and rendering. Climate also owns location text. |

PKJS handles configuration and retrieves climate/location data when C requests it. C owns the refresh schedule.

### Core Concepts

- The display uses tiles (strata). Each tile can contain icon, text, and progress channels (substrata).
- Service callbacks and AppMessage handlers pass event data through one runtime ingress: `watchface_apply_received_data()`.
- The runtime combines affected strata into one refresh mask.
- Display-mode changes repaint the watch face. New data targets the affected strata. Repaints preserve source state when no replacement data arrives.

## Initialization and Runtime Flows

All supported devices follow these flows. Capability guards select the services and optional features available on each platform.

### 1. Initialization flow

```text
ataglance.c
  -> window_create()
  -> window_set_window_handlers(...)
  -> window_stack_push(window, true)
       -> main_window_load(window)
            -> settings_load(&settings)
            -> watchface_create(window, &settings)
                 -> window_get_root_layer(window)
                 -> layout_watchface_prepare(width, height, is_battery_vertical, &surface)
                      -> memset(surface, 0, sizeof(*surface))
                      -> calculate frames from platform and full/compact layout constants
                 -> layout_watchface_update_palette(&surface.style, display_mode)
                 -> window_set_background_color(...)
                 -> layout_watchface_initialize_fonts(...)
                 -> layout_watchface_load_custom_fonts(...)
                 -> feature_module_create(root, prepared surface strata...)
                 -> require date, time, battery, climate, and horizontal-rule strata to succeed
                 -> watchface_refresh(WATCHFACE_UPDATE_ALL_STRATA, &data)
                      -> data is a zeroed event; weather and location use initial source state
            -> register AppMessage callbacks
            -> app_message_open(...)
                 -> retry open with AppTimer only if opening fails
            -> subscribe tick, battery, connection, and health services
            -> apply the current Bluetooth connection state
```

If required strata fail to initialize, `main_window_load()` removes the window. Window unload calls `watchface_destroy()` to release partial module state.

### 2. Runtime event flow

```text
service callback in ataglance.c
  -> build WatchfaceEventData
  -> watchface_apply_received_data()

AppMessage inbox handler in ataglance_comms_handler.c
  -> parse WatchfaceEventData through ataglance_message_parser.c
  -> watchface_apply_received_data(&data, settings)
       -> apply_settings_data(...)
            -> mutate settings for time format, temp unit, display mode, battery orientation, weather cadence, HR cadence, and steps goal
            -> accumulate targeted refresh mask
            -> request repaint when the accepted display mode changes
       -> apply_weather_data(...)
            -> mark current_climate.received when any weather tuple arrives
            -> mark current_climate.complete when all three weather fields parse
            -> request climate refresh when any weather field was received
       -> apply_location_data(...)
            -> request the location text refresh
       -> apply_subscribed_service_updates(...)
            -> translate tick, battery, connection, and health callbacks into refresh masks
       -> apply_oneshot_health_data(...)
            -> queue one-shot BPM/steps overrides
            -> request health refresh
       -> if battery orientation changed
            -> watchface_maybe_relayout(&refresh)
                 -> recalculate battery, charging-bolt, and Bluetooth frames
                 -> request battery, Bluetooth-layout, and horizontal-rule updates
       -> if repaint
            -> watchface_repaint(&data)
                 -> layout_watchface_update_palette(&surface.style, display_mode)
                 -> window_set_background_color(...)
                 -> watchface_refresh(WATCHFACE_UPDATE_ALL_STRATA, &data)
       -> else if refresh mask != WATCHFACE_UPDATE_NONE
            -> watchface_refresh(mask, &data)
                 -> refresh only the addressed strata that were created
```

### 3. Settings persistence and side effects

```text
AppMessage inbox handler
  -> request a refresh when JS_READY is present, then continue parsing
  -> parse settings, weather, location, and one-shot health tuples
  -> watchface_apply_received_data(&data, settings)
  -> if any persisted setting changed
       -> settings_save(settings)
       -> if weather_update_minutes changed, request a refresh and schedule the new interval
  -> if hr_sample_minutes changed
       -> health_service_set_heart_rate_sample_period(...)
```

### 4. Weather-cadence scheduling

```text
src/pkjs/index.js ready
  -> enqueue JS_READY=1 through the settings/control queue
  -> wait for REFRESH from C

AppMessage inbox handler in ataglance_comms_handler.c
  -> receive JS_READY after watchface creation and AppMessage opening
  -> request_refresh(context)
       -> check the phone app connection before each attempt
       -> if disconnected, schedule a retry without opening the outbox
       -> if connected, send REFRESH=1 to PKJS
       -> allow six total attempts, including disconnected checks and failed sends
       -> after delivery succeeds or attempts are exhausted, schedule the next C cycle
  -> continue parsing other tuples

src/pkjs/index.js appmessage handler
  -> receive REFRESH=1 from C
  -> requestUpdate()
       -> queue one geolocation and combined climate/location request
       -> send valid or unavailable data to C

src/pkjs/index.js webviewclosed handler
  -> Clay saves the submitted settings in localStorage
  -> normalize and enqueue settings

C receives a changed WEATHER_UPDATE_MINUTES
  -> persist the accepted value
  -> request a refresh immediately and schedule the new interval

C cadence timer expires
  -> request_refresh(context)
```

C schedules the next cycle without waiting for climate/location data. A failed JS fetch or response delivery does not cancel the C cadence. PKJS has no periodic refresh timer.

[SettingsandConfiguration](SettingsandConfiguration.md) owns the readiness, refresh-request, retry, and cadence contract.

### 5. App teardown

`ataglance.c` owns the refresh and send-retry timer handles. The communications handler schedules these timers through pointers to those handles. Window unload cancels both timers before destroying the watchface.

```text
main_window_unload(window)
  -> unsubscribe tick and battery services
  -> deregister AppMessage callbacks and context
  -> unsubscribe connection service
  -> if health events were subscribed
       -> unsubscribe health events
       -> clear the subscription flag
  -> cancel active refresh and send-retry timers
  -> set both timer handles to NULL
  -> watchface_destroy()
       -> destroy date, time, battery, and climate modules
       -> destroy Bluetooth and horizontal-rule resources
       -> destroy health modules that were created
       -> unload custom fonts after dependent layers are destroyed
       -> clear retained pointers, lifecycle flags, and the prepared surface

deinit()
  -> window_destroy(s_window)
  -> set s_window to NULL
```

Window unload releases watchface resources; `deinit()` releases the window itself. Changed settings are saved when applied, so teardown does not write settings again.

## Prepared Surface

The architect prepares a `WatchfaceSurface` from platform layout constants. `layout_blueprints.h` selects one of two size classes at compile time:

1. `full`: `gabbro`, `emery`.
2. `compact`: `aplite`, `flint`, `diorite`, `chalk`, `basalt`.

The surface is prepared as follows:

1. Layout initialization clears and prepares the caller-owned surface from scratch.
2. The architect calculates frames using platform and full/compact layout constants.
3. The stylist resolves palettes and fonts.
4. The watchface passes prepared frames, substrata, and fonts to feature modules for layer creation.

[UserInterface](UserInterface.md) records current placement, palettes, typography, and screenshots.

## Architect

- Owns geometry and compact/full classification.
- Resolves placement from the active platform's layout blueprint.

## Stylist

- `layout_watchface_update_palette()` resolves the palette; font initialization and loading select fonts by role.
- Owns display-mode styling decisions.
- Supplies the palette and fonts that modules use to resolve feature-specific styling.

## Modules

- Modules own their layers, update procs, source state, feature-specific colors, and destruction.
- `create()` APIs receive only the prepared substrata, frames, and fonts they need.
- `refresh()` APIs receive the current palette and narrow runtime payloads where needed.
- The watchface retains the full surface and style; modules consume their own geometry and palette inputs.
- Modules may expose a source-state setter when runtime ingress needs to update state before refresh.

Required strata are time, battery, date, climate, and the horizontal rule. All other strata are optional.

- Time and date require their text layers.
- Climate requires temperature and location text layers; its condition icon is optional.
- Battery requires its track and procedurally drawn charging-bolt layers.
- Horizontal rule requires its dedicated layer.
- Optional icons never gate required text updates.
- If a required stratum cannot be created, initialization fails and window unload releases partial state.

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
  -> service adapter builds event data, or the message parser parses tuples
  -> watchface_runtime_boundary.c validates and interprets runtime facts
  -> optional module source-state setter
       -> module stores source state only
  -> watchface_refresh(mask, &data)
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
- Climate refresh accepts `ClimateUpdate` and location refresh accepts `LocationUpdate`. Each refresh applies received data to module-owned source state before rendering. Repaints and unit changes preserve source state when no new data arrived.
- QA-only one-shot health setters queue a single source-state override that is consumed by the next health refresh and cleared during module or watch face teardown.

### Reference Example: Steps Module

`steps.c` demonstrates text, bitmap icons, progress rendering, HealthService reads, a settings-driven goal, and a QA-only one-shot override.

**Steps creation flow**

```text
watchface_create()
  -> steps_module_create(root, text substratum, icon substratum, progress frame, font)
       -> require root, text, progress frame, and font
       -> create required steps text layer
       -> initialize goal, threshold, icon color cache, and one-shot state
       -> if icon substratum exists
            -> load RESOURCE_ID_WALK bitmap
            -> create optional icon layer with steps_icon_update_proc
            -> destroy bitmap if optional icon layer creation fails
       -> if no icon frame was supplied
            -> narrow progress frame to text width
       -> create optional progress layer from the adjusted frame
       -> return true once required text layer exists
```

**Steps refresh flow**

```text
watchface_refresh(WATCHFACE_UPDATE_HEALTH, &data)
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
  -> ataglance_comms_handler.c parses WATCHFACE_ONESHOT_MESSAGE_KEY_STEPS
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
       -> clear text buffer, palette, colors, goal, and one-shot state
```

The steps module demonstrates the intended boundary:

- The runtime boundary requests health refreshes; `watchface.c` dispatches them to the affected modules.
- `settings.steps_goal` is passed as a narrow runtime input.
- `steps.c` owns HealthService interpretation, threshold calculation, progress fill math, bitmap recoloring, unavailable rendering, and one-shot consumption.
- `steps.c` receives its own geometry, palette, and steps goal.

## Runtime resolution of palettes and icon colors

- The selected display mode determines the palette.
- A display-mode change resolves the new palette and refreshes all created strata.
- Bitmap recoloring requires palettized PNG resources.

## Capability Guards

- Treat platform and runtime capabilities as conditional. Gate color, health, shape-specific layout, loaded resources, and phone-provided data through the Pebble SDK macros, manifest capabilities, or runtime validation before relying on them.
- Prefer `PBL_COLOR`, `PBL_BW`, `PBL_HEALTH`, `PBL_RECT`, `PBL_ROUND`, and `PBL_API_EXISTS()`.
- Do not assume that a supported target has every capability that a neighboring target has.

## AppMessage Runtime Coverage

`ataglance_comms_handler.c` uses the message parser to build `WatchfaceEventData`, then passes it to `watchface_apply_received_data()`. The runtime validates and applies the data before the communications handler saves changed settings or updates service cadence.

### Inbound AppMessage coverage

- **Settings tuples**: time format, temperature unit, display mode, battery orientation, weather cadence, heart-rate cadence, and steps goal.
- **Weather tuples**: temperature, weather condition, and `is_day`.
- **Location tuple**: current-location text from PKJS; an empty value renders `---`.
- **QA-only one-shot health tuples**: heart-rate and steps overrides.

### AppMessage sends

- PKJS announces readiness with `JS_READY=1`. C requests climate/location with `REFRESH=1` after the watchface has been created. C owns cadence and bounded request retries.
- PKJS uses two independent serialized queues: settings/control and combined weather/location.
- Each queue retries a failed `Pebble.sendAppMessage()` up to five times, with delays of one through five seconds.
- Each queue advances independently, so climate retries do not hold settings/control messages in the same queue.

[SettingsandConfiguration](SettingsandConfiguration.md) owns Clay field mapping, generated message-key numbering, PKJS normalization, and persistence rules.

### Climate-specific transport behavior

- `weather_location.js` owns geolocation, Open-Meteo weather requests, and OpenStreetMap reverse geocoding for each `REFRESH=1` request. Weather is fetched for every request; the reverse-geocoded name may be reused while coordinates remain within the location tolerance.
- Weather and reverse-geocoding requests run concurrently. `Promise.all()` waits for both, updates the climate and lookup caches together, and sends one payload containing `TEMPERATURE`, `WEATHER_CONDITION`, `IS_DAY`, and `MAYBE_CURRENT_LOCATION`. Individual request callbacks do not update these caches.
- If geolocation fails, either request rejects, or either resolved result is missing, PKJS clears its coordinates and climate/location caches. It sends one unavailable payload: temperature `0`, condition `-1`, `is_day` `0`, and an empty location string. Network requests and geolocation use a 15-second timeout; geolocation may reuse a position up to 10 minutes old.
- Refresh attempts run through a serialized promise chain. Reverse-geocoding responses for superseded coordinates are rejected. A late response from a failed attempt cannot publish a cached pair.
- PKJS owns combined climate/location delivery. C parses weather and location independently and accepts partial messages.
- `climate.c` owns the location buffer and text layer. A received empty or malformed location renders `---`; a repaint without a location tuple preserves the buffer.
- Temperature is sent to the watch in Celsius tenths and rendered according to settings.
- `weather_code` and `is_day` are sent to C for glyph selection.
- Weather tuple flow:
  1. A received tuple triggers a climate refresh.
  2. The runtime marks the climate update complete only when temperature, condition, and `is_day` all parse.
  3. `climate.c` validates the parsed domain values.
  4. Incomplete or invalid packets clear prior weather state and render the unavailable vocabulary.
  5. A refresh without new weather data preserves the prior weather values.

## Lifecycle Priorities

1. Evaluate architecture changes using embedded platform affordances and limits.
2. Build for security, usability, performance, code readability, and future maintenance.
3. Validate before confirming feature completion.
4. Use heap allocation only when required, pair allocations with cleanup, and clear released pointers.
5. Do not leave stale pointers, partial layer ownership, unmatched resource lifetimes, buffer overflows, or hidden failure branches.

## Read Next

- [WatchfaceImplementationFlow](WatchfaceImplementationFlow.md) for implementation flow, source responsibilities, and interconnections.
- [SettingsandConfiguration](SettingsandConfiguration.md) for the settings catalog, Clay mapping, message-key contract, persistence, and validation obligations.
- [Validation](Validation.md) for validation contract, evidence, and release profiles.
