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

Some public module headers still expose or import symbols more broadly than the
steady-state architecture should require. `layout_design.h` is now accepted as
private design vocabulary for the architect/stylist boundary, but public header
exposure still needs a focused audit.

Next action: execute the header cleanup plan in `ARCHITECTURE_LEDGER.md`.

### Runtime helper boundaries need audit

Type: Issue
Status: Open

Some functions that help `watchface` manage runtime state still accept broad
watchface-owned types where narrower inputs would better preserve helper
boundaries. Example: stylist functions currently receive
`WatchfaceSurfaceStyle*`, while some operations may only need palette/font
fields or specific style members.

Surface-retention migration target:

| Module | Create needs | Refresh needs | Callback-retained context | Remove retained `WatchfaceSurface*` by |
| --- | --- | --- | --- | --- |
| `date` | `WatchfaceTextSubstratum*`, resolved `GFont` | current `ColorPalette*` | none | retaining only the stable `text->color_role` needed to resolve the current palette color |
| `time` | `WatchfaceTextSubstratum*`, resolved `GFont` | current `ColorPalette*`, `time_format` | none | retaining only the stable `text->color_role` needed to resolve the current palette color |
| `battery` | `WatchfaceBatteryStratum*` frames | current `ColorPalette*` | current palette pointer plus track/fill/bolt frames for Pebble update procs | copying the small `track`, `fill`, and `bolt` frame values into module-owned callback context |
| `climate` | `WatchfaceTextSubstratum*`, `WatchfaceIconSubstratum*`, resolved `GFont` | current `ColorPalette*`, `temp_unit` | current palette pointer for the climate icon update proc | retaining text layer, icon layer state, weather source state, and current palette pointer only |
| `bpm` | `WatchfaceTextSubstratum*`, `WatchfaceIconSubstratum*`, resolved `GFont` | current `ColorPalette*` | current palette pointer for the BPM icon update proc | retaining text color role, BPM source/debug state, and current palette pointer only |
| `steps` | `WatchfaceTextSubstratum*`, `WatchfaceIconSubstratum*`, resolved `GFont` | current `ColorPalette*` | current palette pointer, bitmap, and bitmap foreground color | retaining text color role, steps source/debug state, bitmap state, and current palette pointer only |

Future design notes:

- Create a font equivalent to `ColorPalette` so font-role resolution does not
  hang directly off `WatchfaceSurfaceStyle`.
- Audit `is_compact` usage and decide whether it belongs on
  `WatchfaceSurfaceStyle` or in a narrower layout/style/font contract.

Validation target: after the migration, `watchface.c` may still own and inspect
the whole `WatchfaceSurface`, but feature modules should no longer retain
`static const WatchfaceSurface* s_surface`.

Implementation note: date, time, battery, climate, BPM, and steps now receive
narrow create inputs and current palettes on refresh. The remaining helper audit
is broader than surface retention and covers style/font helper boundaries.

Next action: audit font/style helper ownership and verify with
`rg "static const WatchfaceSurface\\* s_surface" src/modules`.

### Weather glyph visual defects need a focused pass

Type: Issue
Status: Open

The screenshot grid `weather-condition-watchface-grid.png` exposed several
weather glyph defects across compact and full targets. These are product/visual
issues, not part of the runtime-boundary migration.

The larger concern is architectural, not only cosmetic: production weather
glyphs still use a separate frame-aware scaling vocabulary in
`climate_glyphs.c`, including local line, fill, rectangle, circle, and
`weather_subframe()` helpers. That vocabulary now sits beside renderer-owned
stroke and scaling helpers. Before production edits, audit whether each glyph
should keep a local contract, move toward shared renderer vocabulary, or define
separate compact and non-compact contracts.

Process:

1. Prototype and validate non-trivial weather glyph changes in `tools/glyph-lab`
   or an equivalent screenshot-led harness first.
2. Port validated glyphs into the main watchface tree one glyph at a time.
3. For each glyph, capture before/after behavior, document the geometry intent
   in code where it clarifies maintenance, request approval, then commit.

TODO:

- Clear/sun glyph should not be filled; the filled sun reads poorly on Chalk
  and Flint.
- Yellow sun should still be clear/outlined rather than a filled yellow disk.
- Weather blue needs higher contrast across light and dark modes. Cobalt Blue
  remains too low-contrast and needs a focused color decision.
- Weather condition `45` is visually broken on smaller watchfaces. The fog line
  scaling appears wrong, likely due to frame compaction.
- Fog icon lines need more spacing; compact targets may also need different
  stroke widths.
- Revisit the sun geometry. Recent radius and ray-length changes made the sun
  worse; recover an earlier, better-balanced visual contract.
- Weather condition `80` has the same compacted-line issue. Subframed line
  glyphs likely need shorter strokes and/or thinner stroke widths inside
  subframes.
- Partly-clear/cloud composition needs legibility work. The cloud should be
  filled with the background color, and the sun behind it should remain clear so
  the icon reads on light backgrounds.
- Partly-cloudy cannot be discerned in black-and-white dark or light modes.
- Drizzle stroke width is too heavy on compact targets.
- Light sleet fails glanceability on all platforms and modes; redraw or replace
  the glyph.
- Snowflake needs to be redrawn for compact targets.
- Light showers on compact targets are too compressed.
- Light snow showers are poor on all platforms.

Observed good states:

- Light heavy showers, cloudy, and unavailable weather glyphs are good on all
  platforms.

Open design question:

- Consider separate compact and non-compact weather icon contracts instead of
  scaling one glyph set across all display classes.
- Decide whether `weather_subframe()` remains part of the production glyph
  contract or whether subframed glyphs need explicit compact/full geometry.

Completed code slices:

- Fog bar spacing was widened in `tools/glyph-lab/src/c/glyph_lab_glyphs.c` and
  `src/modules/climate_glyphs.c` from y positions `10/15/20` to `6/14/22`.
  Visual screenshot acceptance is still pending.

Next action: use `tools/glyph-lab` or an equivalent screenshot-led glyph pass
to tune these weather glyphs before porting changes back to the watchface.

### Background stratum cleanup

Type: Issue
Status: Closed

The stale background stratum was removed after the background layer went away.
This included deleting `WatchfaceBackgroundStratum`, removing
`WatchfaceSurface.background`, removing unused background palette fields, and
removing `WATCHFACE_UPDATE_BACKGROUND`.

Decision: background color remains window/style state unless a real background
module is restored.

### Runtime update mask cleanup

Type: Issue
Status: Closed

The unused private `WatchfaceRuntimeUpdateMask` was removed from
`watchface_runtime_boundary.c`. Runtime-boundary handling no longer accumulates
a mask that is discarded.

Decision: `ataglance.c` does not receive a runtime-update result. It parses
transport data, calls the watchface runtime boundary, then observes pre/post
settings only for persistence and HR sample-period application.

### Battery rule vocabulary cleanup

Type: Issue
Status: Closed

Battery internals now use `track` vocabulary for the horizontal battery track
instead of stale `rule` names.

Decision: this was naming cleanup only; behavior and drawing policy did not
change.

### Layout architect comment cleanup

Type: Issue
Status: Closed

Stale commented-out calculations were removed from `layout_architect.c`, local
indentation was normalized, and the round design margin was raised to prevent
Chalk icon clipping.

Decision: keep layout comments focused on current geometry contracts rather
than preserving old calculation experiments inline.

### Runtime review findings resolved

Type: Issue
Status: Closed

The current runtime-boundary review found and resolved several defects before
commit:

- Incomplete or malformed weather packets no longer become valid zeroed
  weather. Runtime now sends an atomic `ClimateUpdate` packet and sets
  `is_complete` only when all weather tuples parse; climate owns applying valid
  weather or clearing stale weather.
- The weather packet contract was renamed from parser-centric `data_parsed` to
  runtime-owned `is_complete`, and the boundary is documented in `climate.h` and
  `ARCHITECTURE_LEDGER.md`.
- The private `climate_is_day_is_valid()` predicate was restored to `static`
  linkage.
- Battery track fill again uses the computed battery color, preserving
  low/medium/charging color policy and removing the dead `fill_color`
  calculation.
- Steps bitmap recoloring now combines real-device glanceability with cheap
  repaint behavior: desired foreground is `gcolor_legible_over()` the active
  palette background, while mutation tracks the bitmap's current foreground
  color and only replaces when it changes.
- Steps availability now treats `0` as unavailable in both production and debug
  paths. Product decision: zero steps should render as the cockpit-style `---`
  panel rather than as a useful metric value.
- BPM waveform stroke width is intentionally fixed at 2px after real Flint
  hardware testing showed the scaled procedural glyph was not sufficiently
  visible. This visual change is accepted as product legibility work within the
  current staged slice.
- BPM icon direction remains open: prefer a real BPM icon asset over the current
  rendered procedural glyph when a suitable asset is found.
- The stale `watchface-runtime-boundary.md` handoff plan was archived under
  `archive/` so the live authority remains `ARCHITECTURE_LEDGER.md`, this RAID
  log, module headers, and source.

Decision: runtime supplies atomic update packets and transport completeness;
feature modules own domain validity, source-state fallback, and render-specific
legibility choices.

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
`watchface.c` passes only the narrow substrata, frames, fonts, palette, and
runtime scalar values each feature module needs. Feature modules must not retain
`WatchfaceSurface*`; Pebble update procs may retain only minimal callback
context such as the active palette pointer and copied frame values.

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
