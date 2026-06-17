# At A Glance Design

At A Glance is a glance-first Pebble watchface influenced by transit signage:
clear hierarchy, strong alignment, restrained color, and no decorative
clutter.

## Product Stack

The selected layout stack is shared across rectangular and round displays:

```text
top metric context
dominant centered time
centered battery track and charging bolt
weather/date context
bottom metric context
```

This is a product sequence, not a promise that every display reuses the same
coordinates. Geometry remains the architect's job.

## Visual Priorities

1. Time is the dominant object.
2. Battery is a compact horizontal status band under time.
3. Weather and date form one contextual row.
4. Health metrics sit on the display centerline vocabulary instead of being
   forced into corners.
5. Text is primary. Icons support recognition.
6. Negative space is part of the design.

## Current Constants

The current implementation stores design constants in `src/modules/design.h`
and shared component inputs in `src/modules/watchface_components.h`.

Important product choices:

- Reference face: `200x228`
- Reference icon grid: `28x28`
- Rectangular margin: `6`
- Round margin: current implementation value in `src/modules/design.h`
- Icon/text gap: `2`
- Time top: percentage of display height from the active blueprint
- Battery band width: percentage of display width

Do not treat these values as generic scaling rules. Compact and round layouts
may need their own product choices.

## Typography

The stylist owns font selection in `src/modules/layout_stylist.c`.

- Compact displays use smaller system fonts.
- Full displays use larger system fonts and the custom time font where
  selected.
- The architect decides compact/full once and stores the result on
  `WatchfaceSurfaceStyle.is_compact`; the stylist consumes that state.

## Color

The live palette is documented in `README.md` and implemented in
`src/modules/layout_stylist.c`.

Dynamic metric colors remain module-owned:

- BPM owns BPM zone colors.
- Battery owns battery state colors.
- Climate owns weather condition glyph colors.

## Validation

Every visual change should be reviewed on emulator screenshots, not only by
calculated frames. Minimum release-candidate visual checks:

- `aplite`
- `flint`
- `emery`
- `chalk`
- `gabbro`
- dark and light mode
- unavailable weather
- unavailable health
- `12:59`, `23:59`, and a long date such as `WED 30 SEP`
