# Round Watchface Development

This document summarizes the round-display guidance from Pebble/Rebble
documentation for At A Glance round watchface development. It captures the
shape-specific guidance that still applies now that first-pass round support
exists on this branch.

Sources:

- [Round App UI](https://developer.rebble.io/guides/user-interfaces/round-app-ui/)
- [Round App Design](https://developer.repebble.com/guides/design-and-interaction/in-the-round/)
- [Building for Every Pebble](https://developer.repebble.com/guides/best-practices/building-for-every-pebble/)

## Development Guidance

- Round support is a shape-specific design problem, not a rectangle-scaling
  problem. Rectangular layouts fail on round displays because clipped corners
  remove usable horizontal space and expose off-center assumptions. At A Glance
  should use a separate round architect instead of adapting rectangular
  geometry.

- Use `PBL_RECT` and `PBL_ROUND` for shape-level layout dispatch. Use
  capability macros such as `PBL_COLOR`, `PBL_BW`, and `PBL_HEALTH` only for
  those capabilities. Prefer shape and capability checks over
  `PBL_PLATFORM_*` unless a true model-specific exception is proven.

- Derive layout from root or unobstructed bounds instead of hard-coded screen
  rectangles. `PBL_DISPLAY_WIDTH` and `PBL_DISPLAY_HEIGHT` are useful
  compile-time facts, but round layout should still consume calculated face
  dimensions from the normal layout flow.

- Round displays do not have one stable content width. Each horizontal row has
  a different available width based on its vertical position. Top context,
  time, rule, date, and bottom metrics each need row-specific safe spans.

- Keep readable content away from the outer edge margin. Pebble's round design
  guidance calls out a small edge margin, including a two-pixel border on
  Pebble Time Round. Avoid thin outer rings because small manufacturing
  variation can make them look visibly off-center.

- Centering matters more on round displays. Geometry that appears acceptable
  on rectangular displays can look wrong on `180x180` Chalk. Time, rule, and
  date should remain optically centered around the round face centerline.

- A different round UI is acceptable when the rectangular model does not fit.
  This supports prototyping stacked metric layouts instead of forcing
  rectangular two-column icon/text rows onto Chalk.

- Text flow and pagination are not primary tools for this watchface. At A
  Glance uses fixed, single-line glance fields, so the round layout should
  calculate safe fixed frames rather than relying on reflow to rescue clipped
  content.

- Interactive scrolling and paging patterns are mostly non-goals for this
  watchface. All primary facts should be visible without interaction.

- For custom framebuffer work on round displays, do not assume rectangular row
  memory. Use row-aware framebuffer APIs if future work needs direct
  framebuffer access. Current procedural layer drawing can remain higher
  level unless screenshots prove otherwise.

- Platform-specific resources can help when color, black-and-white, or
  shape-specific assets diverge. Since this project currently uses procedural
  glyphs, keep color and monochrome behavior capability-gated and avoid adding
  shape-specific resources unless visual validation proves they are needed.

- `Pebble.getActiveWatchInfo()` must be guarded before use in PebbleKit JS.
  For layout, C-side shape macros and calculated bounds should remain the
  source of truth; JS watch info should not become layout authority.

## At A Glance Application

- Chalk remains the hard target. It is the tightest round display and the
  best test of whether the visual model actually works.

- Calculate safe spans per row before assigning frames. Do not use a single
  global content width for the round face.

- Compare stacked and unstacked metric layouts before treating the current
  first-pass layout as final. The design choice should be made from measured
  safe spans and screenshots, not from rectangular intuition.

- Preserve the current glance hierarchy: top context, dominant centered time,
  centered rule, centered date, and bottom health context.

- The current committed round layout uses four metric slots around the
  centered time/rule/date core: steps top-left, battery top-right, climate
  bottom-left, and BPM bottom-right. Treat this as the current candidate
  layout, not as a proven final design.

- Keep text primary and icons secondary. Icons may be disabled when they do not
  fit cleanly, but text-bearing metrics should remain readable.

- Keep the round architect private and geometry-only. It should sit behind the
  surface/layout builder path, assign final substratum frames, and leave
  palette, fonts, AppMessage behavior, and module lifecycle untouched.

## Current Project State

- `layout_round.c` exists and provides the `PBL_ROUND` implementation of the
  shared architect contract.

- `layout_rect.c` provides the `PBL_RECT` implementation of the same architect
  contract. Shape-specific implementation remains private to layout/surface
  construction.

- `package.json` currently includes both `chalk` and `gabbro` targets on this
  branch. Geometry changes now require screenshot review on round displays,
  not just rectangular build validation.

- `WatchfaceSurface` remains the calculated UI contract. It carries final
  frames, style, the background rule rectangle, and fixed product strata; it
  must not grow private layout metrics or row-safe-span internals.

- Compact/full state is stored on `WatchfaceSurfaceStyle` for stylist
  consumption. The exact compact predicate and blueprint scaling behavior
  should be audited before further round refinements are treated as final.
