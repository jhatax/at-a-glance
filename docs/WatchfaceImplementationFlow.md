# Watchface Implementation Flow

This document maps watch face implementation files, shared headers, and their relationships. Live source defines current behavior. Use the product, architecture, and build documents for their contracts.

## Adjacent

- [RuntimeArchitecture](RuntimeArchitecture.md) for runtime ownership and boundaries.
- [UserInterface](UserInterface.md) for the implemented visual reference.
- [SettingsandConfiguration](SettingsandConfiguration.md) for settings transport and persistence.

## Read Next

- [Contributing](Contributing.md) for the contributor workflow around implementation changes.

## Feature and Documentation Organization

Three major folders:

1. ./src: Watch face features, modules, surfaces, and adapters
2. ./resources: Fonts, icons, and images
3. ./docs: Watch face documentation

All paths in sub-sections are relative to repository home.

### Feature Source

```text
resources/
src/c/
src/modules/
src/pkjs/
package.json
wscript
```

### Documentation

```text
README.md
docs/
```

## Watch Face Product Implementation Map

```text
────────────────────────────────────────────────────────────────────────────
Implementation / ownership                          Contracts / vocabulary
──────────────────────────                          ──────────────────────
package.json
        | - manifest - platforms - capabilities
        V - message keys - resources
+───────+───────+
|               |
|               src/pkjs/index.js ──────────────────> package.json message keys
|               - Clay bootstrap                    - generated message key symbols
|               - saved Clay preferences and settings/control sends
|               src/pkjs/weather_location.js
|               - geolocation, parallel weather/location requests, combined payload
|               src/pkjs/app_message_queue.js
|               - independent control and climate queues with bounded retries
V               ────────────────────────────────────────────────────────────────────
wscript
- source globs -  JS bundle
- optional ATAGLANCE_DEBUG hook ──────────────────> watchface_debug.h
        |                                           - debug-gate normalization
        v <────────────    Pebble OS   ────────────>
        |
src/c/ataglance.c ────────────────────────────────> watchface.h
- Pebble app lifecycle adapter                     - public runtime ingress
- window ownership                                 - WatchfaceEventData
- settings load                                    - update masks
- service subscriptions                            - runtime-visible transport keys
- AppMessage callback registration
- WatchfaceEventData dispatch
        |
        +-> ataglance_pebble_message_handler.c
        |   - inbox/outbox callbacks and transport opening
        |   - changed-settings persistence and HR cadence side effects
        |       -> ataglance_pebble_message_parser.c
        |          - tuple parsing and received/parsed masks
        |
        v
watchface_runtime_boundary.c ─────────────────────> settings.h
- runtime event & message adapter                  - settings defaults
- settings mutation                                - persisted settings shape
- weather and location ingress application          - validation vocabulary
- one-shot health ingress application
- repaint-versus-refresh decision
        |
        v
watchface.c ──────────────────────────────────────> watchface_components.h
- WatchfaceSurface owner                           - display primitives
- layout/style delegation                          - substrata · strata
- module manager                                   - WatchfaceSurface vocabulary
- root-owned Bluetooth and horizontal-rule layers
- event and visual dispatcher                      - large-display vocabulary
- create/destroy order
- repaint/refresh routing
- battery-orientation relayout dispatch
        |
        +─────────────────────────────────────────> watchface_layout.h
        |                                           - public layout facade
        +─────────────────────────────────────────> layout_surface.h
        |                                           - geometry/design vocabulary
        |                                           - calculated layout structures
        +─────────────────────────────────────────> layout_style.h
        |                                           - visual vocabulary - fontbooks
        |                                           - color & text roles - palettes
        +───────────────+──────────────────────────+
                    |
                    v watch face visual delegates
        +───────────+──────────────────────────────+
        |                                          |
        v                                          v
layout_architect.c                         layout_stylist.c
- placement provider                       - palette resolution
- blueprint selection                      - font-role selection
- compact/full classification              - custom-font load/unload
- prepared-surface assembly                - display-mode styling
- orientation-specific battery placement
        |
        v
feature modules ─────────────────────────────────> module headers
     1. date.c                                     date.h
     2. time.c                                     time.h
     3. battery.c                                  battery.h
        - orientation-aware track rendering
     4. climate.c                                  climate.h
        - climate_glyphs.c                         - climate_glyphs.h
        - location text and source buffer
     5. steps.c                                    steps.h
     6. bpm.c                                      bpm.h
        +─────────────────────────────────────────+
        |
        v
substratum_renderer.c ───────────────────────────> substratum_renderer.h
- shared text/icon layer setup                     - renderer helper contract
- text updates                                     - color-role lookup
- glyph primitives                                 - small rendering helpers
        |
        v
helper.c ────────────────────────────────────────> helper.h
- bitmap palette helpers                           - shared utility macros
helper_tuple_parsing.c
- typed tuple conversion
helper_computations.c
- shared computation helpers
────────────────────────────────────────────────────────────────────────────
```

### Header groups

- Runtime contract: `watchface.h`, `settings.h`
- Watchface vocabulary: `watchface_components.h`, `watchface_layout.h`, `layout_surface.h`, `layout_style.h`
- Diagnostics: `watchface_debug.h`
- Module contracts: `date.h`, `time.h`, `battery.h`, `climate.h`, `climate_glyphs.h`, `steps.h`, `bpm.h`
- Private message adapter: `ataglance_messages_adapter.h`, `ataglance_message_parser.h`
- Shared helpers: `substratum_renderer.h`, `helper.h`, `helper_tuple_parsing.h`, `helper_computations.h`

**Notes**

1. Public and shared headers expose only the concepts cross-module callers need.
2. Feature module headers must not include `watchface_layout.h`.

### Core runtime relationship

```text
ataglance.c builds service events; the private message adapter parses AppMessage tuples
  -> watchface_runtime_boundary.c decides what changed
  -> watchface.c owns the live surface and dispatches create/repaint/refresh
  -> layout delegates prepare geometry and style
  -> feature modules own their Pebble layers and source state
```

## Debug Source Placement

Setting `ATAGLANCE_DEBUG` to 1/0 and flipping/unflipping the comment in `wscript` enables and disables `DEBUG` mode for the watch face:

- `src/modules/watchface_debug.h` **owns debug-gate normalization only**, with `wscript` as the build-time enable point.
- `wscript` owns the optional build-time define.
