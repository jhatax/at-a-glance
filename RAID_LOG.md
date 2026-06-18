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
Status: Closed

Module create paths now gate success on required layers/resources before
retaining live surface state. Optional resources are explicitly non-fatal, and
partial optional resources are cleaned up before returning success.

Decision: required text/control resources determine module success; optional
icon resources may fail without failing the module when the text-bearing
stratum remains usable.

### Renderer allocation strategy in draw path

Type: Issue
Status: Closed

The shared filled-polygon renderer now uses a bounded local `GPoint` array
after input validation instead of heap-allocating scaled point storage in a
drawing path.

Decision: accept the small bounded stack reservation because Pebble heap
fragmentation is a larger draw-path risk than an explicit eight-point array.

### Weather responses can arrive out of order

Type: Risk
Status: Closed

PebbleKit JS starts weather fetches on ready and on an interval. Request ids
prevent stale callbacks from applying. Geolocation denial falls back to the
fixed OAK weather location, and network errors, timeouts, non-200 responses,
parse failures, and malformed current-weather payloads send unavailable weather
sentinels.

Decision: code-path robustness is the test for this issue. OAK remains the
fixed documented weather fallback.

### Round visual validation is not final

Type: Risk
Status: Closed

Round targets are present in `package.json`, and the screenshot-led visual pass
has been confirmed complete for the current milestone.

Decision: no active round visual validation blocker remains for this milestone.

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
