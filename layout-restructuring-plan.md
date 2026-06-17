# Layout Restructuring Plan

**Status: Closed.**

This document is historical. The restructuring goal has landed and the live
contract is now captured in `ARCHITECTURE_LEDGER.md`.

## Closed Outcome

The watchface now uses a prepared `WatchfaceSurface` contract:

```text
ataglance.c
  -> watchface_create(window, settings)
  -> layout_watchface_initialize(width, height, &surface)
  -> layout_update_watchface_style(&surface.style, display_mode)
  -> feature_module_create(root, &surface)
  -> watchface_refresh(...)
```

The old split between `layout.c`, `layout_rect.c`, `layout_round.c`, and
private shape headers has been consolidated. The live layout provider is
`src/modules/layout_architect.c`, with design constants and private
blueprint/calculated-layout structs in `src/modules/design.h`.

## Current Contract

- `watchface` owns the live `WatchfaceSurface` storage and lifetime.
- `layout_watchface_initialize()` clears and prepares the caller-owned
  surface from scratch.
- The architect owns geometry and compact/full classification.
- Compact/full is resolved once and stored on
  `WatchfaceSurfaceStyle.is_compact`.
- `layout_stylist.c` consumes that resolved state for palette, font, and
  custom-font decisions.
- Feature modules own Pebble layers, buffers, source state, update procs,
  refresh behavior, and destroy paths.
- `src/c/ataglance.c` owns Pebble lifecycle, service subscriptions, settings,
  AppMessage parsing, and dispatch to `watchface`.

## Historical Value

This plan should not be used as an implementation spec anymore. It remains in
the repo only to record why the surface/layout/module boundaries exist. Future
cleanup TODOs from the original plan have been moved into
`ARCHITECTURE_LEDGER.md`.
