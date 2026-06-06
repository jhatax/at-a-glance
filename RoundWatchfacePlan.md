# Round Watchface Plan

This is the planning artifact for adding round-display support. It is
not approval to add `chalk` or `gabbro` yet.

## Existing Plan Found

The existing project notes say:

- Plan round layout separately before adding `chalk` or `gabbro`.
- Research Pebble/Rebble round-display guidance and community examples.
- Account for row-specific clipping on circular screens.
- Keep row-specific content widths separate.
- Do not scale the rectangular layout and call it round support.

The current code has the right starting point: `layout_calculate()`
fills a caller-owned `WatchfaceLayout`. `WatchfaceLayout` already has
separate width fields for date, time, health, and bottom content. Today
those fields all receive the same rectangular content width.

## Source Findings

- Pebble's round UI guidance says rectangular layouts fail on round
  screens because corners are clipped and because centered geometry
  becomes more important on the `180x180` Chalk display.
- Pebble's general multi-platform guidance says each horizontal line of
  text has a different available width on round displays.
- Pebble's round design guidance recommends a two-pixel edge margin and
  warns against thin rings at the outer edge.
- Pebble text APIs support text flow and paging, but this watchface has
  single-line fixed values. We should calculate safe frames instead of
  relying on text flow to rescue poor placement.
- The rePebble Round 2 SDK note says `gabbro` is the Pebble Round 2
  platform and its display is `260x260`, compared with `180x180` for
  Chalk.
- Official example work such as Time Dots and Concentricity is
  deliberately round-native. It does not preserve a rectangular row
  layout.

Sources:

- <https://developer.repebble.com/guides/user-interfaces/round-app-ui/>
- <https://developer.repebble.com/guides/design-and-interaction/in-the-round/>
- <https://developer.repebble.com/guides/best-practices/building-for-every-pebble/>
- <https://developer.rebble.io/docs/c/Graphics/Drawing_Text/>
- <https://repebble.com/blog/cloudpebble-returns-plus-new-pure-javascript-and-round-2-sdk>
- <https://github.com/pebble-examples/time-dots/>

## Current Layout Constraints

Current rectangular layout:

- Uses `content_x = 2`, which is acceptable for rectangular Emery but
  unsafe as a universal round inset.
- Centers the horizontal rule at `face_height / 2`.
- Places date, time, health, and bottom rows using rectangular
  assumptions.
- Gives every content-width field the same width.
- Keeps text left-aligned everywhere.
- Keeps each complication as an icon-plus-value horizontal pair.

This cannot be carried onto Chalk unchanged. The bottom row is the hard
failure: a two-column icon-plus-value row near the lower curve does not
have enough horizontal width on a `180x180` circular screen.

The current module boundary makes the right product rule clearer:
`layout.c` owns geometry, `watchface_composer.c` creates modules from
that geometry, and each metric module owns its text and optional icon
layers. Round support should preserve that boundary. The layout must
describe the frames and optional icon policy; the composer should not
invent geometry, and individual modules should not decide global round
placement.

## Round Geometry Rule

Round layout needs row-band safe spans, not one global content width.

For a square circular display:

- `diameter = min(face_width, face_height)`
- `radius = diameter / 2`
- `center_x = face_width / 2`
- `center_y = face_height / 2`
- `safe_radius = radius - edge_margin`
- for a row band `(y, h)`, use the farthest vertical edge from center:
  `dy = max(abs(y - center_y), abs((y + h - 1) - center_y))`
- `half_width = isqrt((safe_radius * safe_radius) - (dy * dy))`
- `safe_x = center_x - half_width`
- `safe_w = half_width * 2`

Add a small optical inset after this calculation for text and glyphs.
This keeps the top date, the lower complication grid, and the rule from
touching the circular edge.

Implementation note: use integer math. Do not introduce floating point.

## Recommended Product Direction

Use a round-native centered layout as the starting hypothesis:

1. Date centered near the upper third.
2. Time centered in the largest middle band.
3. A short horizontal rule centered on the safe span at its y-position.
4. Compact complication rows below the rule.

The complication layout is the important design question. The first
Chalk prototype should preserve all four text data points with two
compact horizontal rows:

- row 1: BPM and steps
- row 2: weather/temperature and battery

The health row may keep small icons because `62` and `4218` are not
self-labeling. The bottom row should be text-first on Chalk: temperature
and battery percentage already carry `°F`/`°C` and `%`, so their icons
are optional and should be omitted if they force crowding near the lower
curve.

The rejected option is a vertical `2x2` icon-over-value grid, which is
too tall near the bottom curve with the current value font.

For Gabbro, reuse the same layout model and safe-span calculation.
Because Gabbro is `260x260`, it can allow more breathing room, but it
should not be a scaled rectangular layout.

## Chalk Geometry Audit

Chalk is the hard target because it is `180x180`.

The current rectangular value font uses an 18px text layer. If a round
complication cell uses a 20px icon, a 1px gap, and an 18px value layer,
the minimum vertical stack is:

```text
20 icon + 1 gap + 18 value = 39px
```

Two stacked complication rows therefore need 78px before row gaps.
Below a centered rule, that pushes the lower row near the bottom curve.
At those y-positions, the round safe span becomes too narrow for two
useful columns.

The same problem appears horizontally. The lower row can narrow to
roughly a two-cell width where `90°F` and `100%` can fit as text, but
full icon-plus-value pairs have almost no slack.

Conclusion: do not add a dormant `PBL_ROUND` branch until we choose one
of these tradeoffs:

- reduce or change the round value font
- show fewer than four text values on Chalk
- omit optional icons where text is already self-labeling
- use a single-column complication stack
- use a `2x2` grid only on Gabbro and a different Chalk layout
- move some complications above or around the rule instead of below it

## Prototype Finding

The first production round layout should not use icon-over-value cells
on Chalk. It should use compact horizontal rows with text as the
must-initialize surface.

Recommended Chalk model:

- date centered near `y=18`
- time centered near `y=52`
- rule raised to about `y=105`
- health row at about `y=110`
- bottom row at about `y=132`
- complication rows use 20px text height
- health icons shrink to about 16px
- bottom-row icons are omitted on Chalk unless a prototype proves there
  is comfortable slack
- row content is centered inside the round safe span

The important safe-span estimates for Chalk:

```text
row              band       safe width
health row       y=110 h=20  about 148px
bottom row       y=132 h=20  about 118px
```

Approximate required widths:

```text
BPM:      16 icon + 1 gap + 28 value = 45px
steps:    16 icon + 1 gap + 40 value = 57px
health:   45 + 4 column gap + 57 = 106px

temp:     40 value
battery:  34 value
bottom:   40 + 20 column gap + 34 = 94px
```

This gives the bottom row roughly 24px of slack on Chalk, which is much
more realistic than the full icon-plus-value candidate. It preserves the
current text font and all four data values while accepting the product
hierarchy: text is essential, icons are support.

For Gabbro, start with the same model before inventing a separate one.
The larger safe spans should allow the bottom weather and battery icons
to return, likely at a larger round-specific icon size, while keeping
the row order and text-first contract identical to Chalk.

## Module-Aware Layout Contract

The current composer and module split changes the implementation shape:

- `layout.c` should calculate round frames and optional icon policy.
- `watchface_composer.c` should pass those frames to the modules and
  keep enforcing must-initialize text creation.
- Date, time, battery, weather, and health modules should keep owning
  their layer creation, update procs, buffers, and destroy paths.
- Optional icon suppression must be explicit. Do not rely on failed
  `layer_create()` calls or zero-sized accidental frames as control
  flow.
- If `WatchfaceLayout` needs round-specific icon visibility flags, add
  named fields rather than a generic cell array.
- Text alignment may need to become layout-provided metadata. Round date
  and time should be centered; rectangular alignment should remain
  unchanged.

## Implementation Plan

Phase 1: decide the round information model.

- Chalk should show all four text values at once.
- Decide whether Chalk may use smaller or different value fonts.
- Decide whether Gabbro re-enables bottom-row icons within the same
  two-row model.
- Do not write a dormant round branch until this decision is made.

Phase 2: geometry only.

- Keep `package.json` target platforms unchanged.
- Add a private round branch inside `layout_calculate()`.
- Add small private helpers in `layout.c` for round safe spans and
  integer square root.
- Keep `layout_calculate()` writing into caller-owned `WatchfaceLayout`.
- Do not pass `WatchfaceLayout` or other project structs by value.

Phase 3: layout data shape.

- Extend `WatchfaceLayout` only if needed for round-specific alignment,
  icon visibility, or cell frames.
- Prefer explicit fields over an opaque generic array.
- Keep existing rectangular fields stable for current platforms.
- If round needs text alignment changes, make that explicit instead of
  hiding it inside magic coordinates.

Phase 4: round rendering behavior.

- Center date and time on round displays.
- Keep rectangular left alignment unchanged.
- For Chalk, keep health icons small and omit bottom-row icons unless
  screenshots prove the row has comfortable slack.
- For Gabbro, evaluate restoring all four icons within the same two-row
  text-first model.
- Keep icon frames stable within each cell.
- Use current glyph modules; do not redesign glyphs as part of round
  layout.

Phase 5: platform enablement.

- Add `chalk` only after the round branch builds and screenshots are
  acceptable.
- Add `gabbro` after verifying the local SDK supports it in this repo.
- Do not reorder AppMessage keys.
- Update `AGENTS.md`, `README.md`, and manual validation commands after
  target platforms change.

Phase 6: validation.

- Build before every emulator pass.
- Test current rectangular platforms still build.
- Capture Chalk screenshots for dark and light modes.
- Capture Gabbro screenshots if the local SDK exposes the platform.
- Test likely text extremes:
  `WED · 30 SEP`, `12:59`, `100`, `99999`, `---F`, and `100%`.
- Test missing health/weather values.
- Test color and black-and-white behavior where platform support allows.

Use `pebble kill --force` for emulator recovery.

## Open Decisions

1. Confirm that Chalk may use 16px health icons and omit bottom-row
   icons.
2. Decide whether round date/time remain the same fonts or receive
   round-specific font choices after screenshots.
3. Decide whether Gabbro uses the same compact rows with all icons
   restored or a roomier round-specific grid.
4. Verify whether this workspace's SDK can build and emulate `gabbro`.
5. Decide whether the first patch targets Chalk only, then Gabbro, or
   both round platforms together after SDK verification.

## Recommended First Patch

The first patch should be a round-decision/prototype patch, not
production layout code:

- create a small frame-calculation prototype or temporary layout lab
- model Chalk `180x180` and Gabbro `260x260`
- test the recommended Chalk layout: centered date/time, small health
  icons, bottom text-only row
- test the Gabbro variant that restores bottom weather/battery icons
- prove text extremes before touching production `layout.c`

After that decision is reviewed, the first production code patch should
add round-safe-span helpers in `src/modules/layout.c`, branch
`layout_calculate()` on `PBL_ROUND`, and keep target platforms
unchanged.
