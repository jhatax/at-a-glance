# Current Handoff

**Status: Closed.**

This handoff is no longer active. The live architecture contract, invariants,
and cleanup backlog are in `ARCHITECTURE_LEDGER.md`.

Current source facts:

- `src/c/ataglance.c` owns Pebble lifecycle, services, settings, AppMessage
  parsing, and dispatch to `watchface`.
- `src/modules/watchface.c` owns the live `WatchfaceSurface` and module
  lifecycle coordination.
- `src/modules/layout_architect.c` prepares calculated geometry.
- `src/modules/layout_stylist.c` applies palette, font, and custom-font
  decisions.
- `src/modules/design.h` is currently a private implementation header for
  design constants, blueprints, and calculated layout structs.
- `package.json` currently includes rectangular and round targets.

Do not use older handoff details as current instructions without verifying
against live source and the ledger.
