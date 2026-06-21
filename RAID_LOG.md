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
| `climate` | `WatchfaceTextSubstratum*`, `WatchfaceIconSubstratum*`, resolved `GFont` | current `ColorPalette*`, `temp_unit` | current copied `ClimatePalette` for the climate icon update proc | retaining text layer, icon layer state, weather source state, and current module palette copy only |
| `bpm` | `WatchfaceTextSubstratum*`, `WatchfaceIconSubstratum*`, resolved `GFont` | current `ColorPalette*` | current palette pointer for the BPM icon update proc | retaining text color role, BPM source/debug state, and current palette pointer only |
| `steps` | `WatchfaceTextSubstratum*`, `WatchfaceIconSubstratum*`, resolved `GFont` | current `ColorPalette*` | current palette pointer, bitmap, and bitmap foreground color | retaining text color role, steps source/debug state, bitmap state, and current palette pointer only |

Future design notes:

- Create a font equivalent to `ColorPalette` so font-role resolution does not
  hang directly off `WatchfaceSurfaceStyle`.
- Audit `is_compact` usage and decide whether it belongs on
  `WatchfaceSurfaceStyle` or in a narrower layout/style/font contract.
- Add a narrow text-palette boundary so modules that only need base text colors
  and mode do not receive the whole `ColorPalette`.

  Proposed shared shape:

  ```c
  typedef struct {
    bool is_light_mode;
    GColor primary_text;
    GColor unavailable_text;
  } WatchfaceTextPalette;
  ```

  Proposed flow:

  1. `layout_stylist.c` keeps owning the full base `ColorPalette` values:
     `is_light_mode`, `background`, `primary_text`, `unavailable_text`,
     `date_text`, and `time_text`.
  2. `watchface.c` derives a stack-local or copied `WatchfaceTextPalette` from
     the active base palette before calling module create/refresh functions.
  3. Feature modules that only need text colors and mode receive
     `const WatchfaceTextPalette*` instead of `const ColorPalette*`.
  4. Feature modules with module-specific color semantics define private module
     palettes, selected from `text_palette->is_light_mode`.
  5. `watchface_runtime_boundary.c` continues to compute update categories only;
     it should not construct palettes or pass colors. `watchface.c` remains the
     owner of visual dispatch and supplies the current text palette during
     `watchface_refresh()`.

  Future signatures should move toward:

  ```c
  void date_module_refresh(const WatchfaceTextPalette* text_palette);
  void time_module_refresh(
      const WatchfaceTextPalette* text_palette,
      uint8_t time_format);
  void climate_module_refresh(
      const WatchfaceTextPalette* text_palette,
      uint8_t temp_unit);
  void battery_module_refresh(const WatchfaceTextPalette* text_palette);
  void bpm_module_refresh(const WatchfaceTextPalette* text_palette);
  void steps_module_refresh(const WatchfaceTextPalette* text_palette);
  ```

  Create functions should be audited separately. Modules that need only mode and
  text colors should receive `WatchfaceTextPalette`; modules whose Pebble update
  procs need `background` or role-specific colors may still require either a
  copied background value or a different narrow palette struct.
- Drive naming consistency before or during this migration: base palette fields
  should use explicit role names such as `date_text` and `time_text`, not
  ambiguous `date` and `time`, because these are text colors rather than module
  palettes or full feature styles.
- Module-specific palette migration template:

  1. Keep the full base `ColorPalette` flowing through current module APIs until
     a separate API-narrowing slice introduces `WatchfaceTextPalette`.
  2. Define a module-owned palette type for module-specific semantic colors.
  3. Define `c_dark_*_palette` and `c_light_*_palette` as `static const` values,
     following the `layout_stylist.c` palette-definition pattern.
  4. Select the module palette from the incoming base palette's `is_light_mode`.
     Do not store `is_light_mode` inside the module palette.
  5. Keep source-state interpretation inside the module. Palette selection should
     choose colors only; it must not decide whether a battery is low, BPM is high,
     or weather is available.
  6. Do not duplicate base palette colors in static module palettes. If lower
     drawing code needs base colors, copy them from the active base palette
     during refresh into the module's current palette instance.
  7. Use one vocabulary consistently within the module; climate uses `unknown`
     because the glyph state represents unknown weather data. In climate, the
     current palette's `unknown` and `default_color` are copied from
     `palette->unavailable_text` and `palette->primary_text`, respectively.
  8. Store only the selected/current module palette needed by Pebble update procs.
  9. Keep B/W fallback decisions for module-owned colors in the static module
     palette constants with
     `PBL_IF_COLOR_ELSE`.
  10. After each module migrates, run `git diff --check`, `pebble build`, and stage
     only that module's reviewed files plus this RAID log if the process changes.

Accepted climate boundary:

- `climate.c` owns climate styling policy. It defines the light/dark
  `ClimatePalette` templates, selects the template from the active base
  `ColorPalette.is_light_mode`, and copies active base colors into the current
  module palette during refresh.
- `climate.c` retains the active copied `ClimatePalette` because Pebble layer
  update procs need callback-visible colors after refresh returns.
- `climate_glyphs.c` is a rendering helper only. It receives a fully-resolved
  `ClimatePalette*` and must not select light/dark mode, inspect
  `ColorPalette`, or derive module styling policy.
- The module updates the active climate palette only when the selected template
  or injected base colors differ from the currently retained copy.

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

Meta-entry:

1. Review `weather_subframe()` usage before tuning individual glyphs.
2. Review each weather glyph against compact and reference/full display classes.
3. Decision: follow the `layout_architect` pattern and split compact glyphs from
   reference glyphs into distinct paths. The goal is to simplify glyph code by
   avoiding excessive `substratum_scale_*` compensation inside one shared path.

Unavailable glyph decision:

- Device visual testing changed the unavailable weather slash contract. Contrast
  now comes from the longer segmented slash geometry, so the slash does not need
  to use `primary_text`. The weather unavailable slash should use
  `unavailable_text` to keep the unavailable icon semantically consistent while
  preserving legibility.

Weather blue decision:

- Device visual testing rejected `GColorCobaltBlue` as too low-contrast for
  weather glyphs and rejected `GColorElectricBlue` as too luminescent on white
  backgrounds. Cloud, fog, drizzle, rain, and shower glyphs now use module-owned
  climate palette colors: Electric Blue in dark mode and Blue in light mode,
  with normal monochrome fallbacks. Snow, snow showers, and sleet stay on
  `GColorPictonBlue` because that family already reads as distinct cold-weather
  vocabulary.

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
- Weather blue was moved into the module-owned climate palette after device
  testing. Cloud/fog/rain glyph families use Electric Blue in dark mode and Blue
  in light mode. Snow/sleet remains Picton Blue.
- Climate glyph rendering now receives only `ClimatePalette`. The current
  climate palette copies `background`, `default_color`, and `unknown` from the
  active base palette so glyph drawing no longer depends on the broader base
  `ColorPalette` and base text colors do not drift.
- Glyph-lab will be updated at the end of the next glyph pass. The next coding
  round will proceed glyph by glyph rather than syncing the whole lab upfront.

Next action: use `tools/glyph-lab` or an equivalent screenshot-led glyph pass
to tune these weather glyphs before porting changes back to the watchface.

### Documentation source-of-truth drift

Type: Issue
Status: Open

Some older planning and review documents still contain stale open items or
pre-migration architecture language. `RAID_LOG.md` and `ARCHITECTURE_LEDGER.md`
are the current authorities; older ledgers and handoff files should not reopen
already-closed work unless a fresh source audit proves the issue still exists.

Drifted sources to reconcile:

- `FINAL_REVIEW_LEDGER.md` still lists FR-301 runtime update mask cleanup and
  FR-302 `ataglance.h` retirement as open. Both are closed in live code and in
  this RAID log.
- `FINAL_REVIEW_LEDGER.md` still carries FR-104 pre-migration weather language.
  The live contract is now atomic `ClimateUpdate`: runtime owns transport
  completeness, climate owns domain validity and stale-state clearing.
- `ARCHITECTURE_LEDGER.md` still has stale cleanup backlog lines for removed
  background stratum/update artifacts, `DESIGN_COMPACT_RIGHT_TEXT_X`, and
  `HELPER_SCALE_ROUND` not being fully parenthesized.
- `AGENTS.md` still describes the older AppMessage flow where `ataglance.c`
  builds a local `WatchfaceUpdateMask`, calls narrow setters, and coalesces
  `watchface_refresh()`. The current runtime-boundary contract is that
  `ataglance.c` parses transport tuples, calls the watchface runtime boundary,
  and does not decide repaint/refresh for AppMessage handling.
- `layout-architect-role-flow.md` contains an old helper-macro TODO and API
  examples that should be reconciled against the current layout API before being
  treated as guidance.
- Dirty-tree document deletions and archived planning files need one final docs
  closeout pass so the repository has one clear live documentation set before
  publication.

Consolidated current status:

- `HELPER_CLAMP_MIN` is intentionally retained as a small helper utility.
- `HELPER_SCALE_ROUND` is fully parenthesized in live source.
- `helper_swap_colors_in_bitmap()` is intentionally retained as a bitmap/PNG
  utility.
- Background stratum fields, background palette fields, and
  `WATCHFACE_UPDATE_BACKGROUND` have been removed.
- `WatchfaceRuntimeUpdateMask` has been removed.
- `src/c/ataglance.h` has been removed.
- `src/scratch.txt` has been removed.
- `design-preview.html` is deleted in the current dirty tree but not yet
  committed; publication cleanup should verify whether to keep the deletion.
- Weather glyph issues remain active and should be handled through the glyph-lab
  validation gate, not through stale final-review entries.

Next action: update or archive the drifted documents in one docs-only closeout
slice after current code/resource edits settle.

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
