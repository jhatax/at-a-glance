# RAID Log

Use this lightweight log alongside `ARCHITECTURE_LEDGER.md` for non-trivial
layout, platform, lifecycle, or API work. It should not replace the ledger.

RAID means:

- Risk: a future problem that may occur.
- Assumption: something currently treated as true but not yet proven.
- Issue: an active problem already affecting the work.
- Decision: an accepted direction or tradeoff.

## Active Entries

### Create failure paths may retain stale module state

Type: Issue
Status: Open

Some module create paths assign retained surface or palette pointers before all
optional resources are handled. Failure behavior should be made explicitly
transaction-like across modules.

Next action: make create paths destroy resources created in the same function
and clear retained pointers before returning false.

### Renderer allocation strategy in draw path

Type: Issue
Status: Open

The shared filled-polygon renderer allocates scaled point storage in a drawing
path even though the point count is bounded.

Next action: avoid per-frame heap allocation without wasting stack. Use exact
primitive-specific storage, fixed static storage, or a bolt-specific draw path.

### Weather responses can arrive out of order

Type: Risk
Status: Open

PebbleKit JS starts weather fetches on ready and on an interval. The current
request id prevents stale callbacks from applying, but this should be validated
on device before publication.

Next action: test repeated weather refreshes and unreachable-network behavior.

### Round visual validation is not final

Type: Risk
Status: Open

Round targets are present in `package.json`, but the round visual result still
needs a final screenshot-led pass on Chalk and Gabbro before publication.

Next action: capture dark/light screenshots on Chalk and Gabbro and check
time, battery, weather/date, and health text at likely extremes.

### Header cleanup is deferred

Type: Issue
Status: Open

`ataglance.h`, `design.h`, and several public module headers still expose or
import symbols more broadly than the steady-state architecture should require.

Next action: execute the header cleanup plan in `ARCHITECTURE_LEDGER.md`.

## Accepted Decisions

### Reuse architecture, not geometry

Type: Decision
Status: Accepted

The code should reuse the architect/stylist/surface architecture across
platforms. It should not assume that product geometry, icon dimensions, text
dimensions, or spacing can be reused by scaling from another device class.

### One prepared surface contract

Type: Decision
Status: Accepted

`watchface` owns `WatchfaceSurface` storage. The architect prepares that
surface from scratch and stores final geometry plus compact/full state.
Feature modules create layers from the prepared surface and retain it for
their lifetime.

### Shared visual DNA across display families

Type: Decision
Status: Accepted

Rectangular and round targets should feel like the same product: top metric
context, dominant centered time, centered battery track, weather/date context,
and bottom metric context. Geometry can differ by shape.

### Glyph lab is the validation gate for glyph work

Type: Decision
Status: Accepted

Validate non-trivial glyph changes in `tools/glyph-lab` before porting them
into the main watchface tree.

### Battery text drops the percent sign

Type: Decision
Status: Accepted

Battery text no longer shows `%` where battery text is used. The current
primary layout uses a battery track instead of battery text.

### Keep the ataglance/watchface boundary narrow

Type: Decision
Status: Accepted

`ataglance.c` parses AppMessage transport tuples and converts them to typed
runtime facts before calling narrow watchface setters. `watchface` should not
parse raw dictionaries or take a shared runtime-update package unless message
volume or atomic multi-field coherence makes the current setter path
meaningfully worse.
