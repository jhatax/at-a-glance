# Round Watchface Reference

**Status: Current reference.**

This document collapses the former round plan, handoff, and development notes.
It records round-display lessons that still matter after the first
implementation pass. The live architecture contract and active TODO backlog
remain in `ARCHITECTURE_LEDGER.md`.

## Current Source State

- `src/modules/layout_architect.c` is the live geometry provider.
- `src/modules/layout_design.h` holds private design constants, blueprints, and
  calculated layout structs.
- `package.json` currently includes rectangular and round targets.
- `WatchfaceSurface` remains the calculated UI contract and must not expose
  private layout metrics.
- Compact/full is stored on `WatchfaceSurfaceStyle` by the architect and
  consumed by the stylist.
- Feature modules remain Pebble layer owners.

## Durable Round Principles

- Round support is a shape-specific product design problem, not a rectangular
  scaling problem.
- The code should reuse the surface/architect/stylist/module architecture
  across displays without assuming shared geometry.
- Round displays do not have one stable content width. Available horizontal
  span changes by vertical position.
- Calculate from real face dimensions and root bounds.
- Account for circular clipping by vertical position.
- Chalk `180x180` remains the hard round target.
- Gabbro `260x260` should reuse the same safe-span reasoning as an expansion
  case, not a scaled rectangular layout.
- Center alignment matters more on round displays, especially for time,
  battery track, date, and the bottom metric context.
- Text remains the primary glance surface. Icons are support and may be
  disabled when they do not fit cleanly.
- Do not rely on text flow or scrolling; all primary facts should be visible
  at rest.
- Round visual changes require emulator screenshot review before they are
  considered final.

## Source Guidance Used

- [Round App UI](https://developer.repebble.com/guides/user-interfaces/round-app-ui/)
- [Round App Design](https://developer.repebble.com/guides/design-and-interaction/in-the-round/)
- [Building for Every Pebble](https://developer.repebble.com/guides/best-practices/building-for-every-pebble/)
- [Drawing Text](https://developer.rebble.io/docs/c/Graphics/Drawing_Text/)
- [Round 2 SDK note](https://repebble.com/blog/cloudpebble-returns-plus-new-pure-javascript-and-round-2-sdk)

## Current Product Direction

The current source favors a shared visual stack across rectangular and round
targets:

```text
top metric context
dominant centered time
centered battery track and bolt
weather/date context
bottom metric context
```

This is the shared visual DNA between display families. The implementation
must still be judged by screenshots on real emulator targets because a shared
sequence does not prove that every target has enough optical room.

## Remaining Round Work -- Complete

## Historical Notes

Earlier versions of this document mentioned `layout_rect.c`, `layout_round.c`,
and separate private shape headers. Those files no longer represent the live
implementation. The current layout provider is `src/modules/layout_architect.c`.

Former files collapsed into this reference:

- `RoundLayoutHandoff.md`
- `roundwatchfacedevelopment.md`
