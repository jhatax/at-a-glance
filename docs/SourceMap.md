# Source Map

This document is the current source of truth for At A Glance source-code
navigation. It maps implementation files, shared headers, QA tooling, and their
relationships. Use the dedicated product, architecture, and build documents for
runtime behavior, product rules, visual rules, and build/validation policy.

## Required Reading

- [ArchitectureLedger.md](ArchitectureLedger.md) for the runtime architecture.
- [UserInterface.md](UserInterface.md) for the full visual reference implementation.

## Feature, QA, and Docs Organization

Four major folders:

1. ./src: Watch face features, modules, surfaces, and adapters
2. ./resources: Fonts, icons, and images
3. ./qa: Build and test harness modules
4. ./docs: Watch face documentation

### Feature Source

All paths relative to repository home.

```text
resources/
src/c/
src/modules/
src/pkjs/
package.json
wscript
```

### QA Source

```text
qa/
qa/data
qa/lib
qa/stages
qa/tests
```

### Documentation

```text
README.md
docs/
qa/README.md
```

## Relationship Map

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
|               - geolocation and Oakland fallback
|               - weather refresh requests, appmessage packaging
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
- settings load/save                               - update masks
- service subscriptions                            - runtime-visible transport keys
- AppMessage parsing
- WatchfaceEventData dispatch
        |
        v
watchface_runtime_boundary.c ─────────────────────> settings.h
- runtime event & message adapter                  - settings defaults
- settings mutation                                - persisted settings shape
- weather ingress application                      - validation vocabulary
- one-shot health ingress application
- repaint-versus-refresh decision
        |
        v
watchface.c ──────────────────────────────────────> watchface_components.h
- WatchfaceSurface owner                           - display primitives
- layout/style delegation                          - substrata · strata
- module manager                                   - WatchfaceSurface vocabulary
- event and visual dispatcher                      - large-display vocabulary
- create/destroy order
- repaint/refresh routing
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
        |
        v
feature modules ─────────────────────────────────> module headers
     1. date.c                                     date.h
     2. time.c                                     time.h
     3. battery.c                                  battery.h
     4. climate.c                                  climate.h
        - climate_glyphs.c                         - climate_glyphs.h
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
- tuple parsing                                    - shared utility macros
- bitmap palette helpers
────────────────────────────────────────────────────────────────────────────
```

### Header groups

- Runtime contract: `watchface.h`, `settings.h`
- Watchface vocabulary: `watchface_components.h`, `watchface_layout.h`, `layout_surface.h`, `layout_style.h`
- Diagnostics: `watchface_debug.h`
- Module contracts: `date.h`, `time.h`, `battery.h`, `climate.h`, `climate_glyphs.h`, `steps.h`, `bpm.h`
- Shared helpers: `substratum_renderer.h`, `helper.h`

**Notes**
1. Public and shared headers expose only the concepts cross-module callers need.
2. Feature module headers must not include `watchface_layout.h`.

### Core runtime relationship

```text
ataglance.c adapts Pebble callbacks and AppMessage tuples into WatchfaceEventData
  -> watchface_runtime_boundary.c decides what changed
  -> watchface.c owns the live surface and dispatches create/repaint/refresh
  -> layout delegates prepare geometry and style
  -> feature modules own their Pebble layers and source state
```

## Debug Source Placement

`ATAGLANCE_DEBUG` source placement:

- `src/modules/watchface_debug.h` **owns debug-gate normalization only**, with `wscript`
  as the build-time enable point.
- `wscript` owns the optional build-time define.

## Future Reading

- [Build_and_Validation.md](Build_and_Validation.md) for build activation and emulator validation.
- [Contributing.md](Contributing.md) for contributor workflow, validation, logging, and review discipline.
- [../qa/README.md](../qa/README.md) for Build & QA harness source navigation.
