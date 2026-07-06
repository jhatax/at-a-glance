# Visual Vocabulary

This document defines how At A Glance expresses meaning visually.

It covers the shared watchface composition, channel responsibilities, glyph rules, display-mode behavior, and absence language. It does not own product commitments, runtime architecture, or current geometry tables.

## Required Reading

- ../README.md
- Design.md
- ProductInvariants.md

## Visual Channels & Responsibilities

1. Each visual channel should have one primary visual expression.

- Text for exact values
- Icons for metric identity
- Lines for progress
- Color for state

2. Nothing on screen should interpret the data when direct presentation is enough.

3. Text is the primary glance surface. Icons support recognition, but they do not carry the product alone.

## Information Display

- A text-only metric is acceptable when an icon cannot be created or does not fit the platform.
- An icon-only metric is not acceptable.

## Surface Composition

The shared watchface composition is:

```text
top steps layer with progress and state
dominant bold and centered time
centered battery track and plugged-in bolt
weather/date context
bottom heart-rate layer with state
```

- This stack is part of the product's visual DNA.
- Rectangular and round displays implement the same information hierarchy and stack.
- Placement is resolved from defined layout constants to reduce positioning ambiguity.
- Text has to convey information precisely and independent of supporting visual channels.
  - Icons and progress bars support text in glanceability but can be hidden.
- Negative space is used to serve glanceability.

**Display hierarchy invariant** is the location and order in which time, battery track, weather, and date are displayed:

## Glyph and Icon Selection

1. All icons and glyphs satisfy principal design invariant of automatic recognition on all supported Pebble devices:
- Color, Monochrome
- Compact, Full
- Rectangular, Round

2. Use custom glyphs when the product needs stronger recognition than borrowed icon sets can provide.

3. Color glyphs to show progress only when glanceability is enhanced.

## Icon Recognizability

- Design for one-second recognition.
- Prefer fewer, stronger marks over dots, texture, wisps, and small accents.
- Make the primary metaphor large.
- Drop secondary accents when they force the primary shape to shrink.

## Silhouette

- The silhouette should convey meaning without internal detail inspection.
- Prefer solid filled silhouettes over hollow outlines.
- Use line art only when the line itself is the message.

### Climate glyphs

- Prefer silhouttes over filled glyphs. Use filled glyphs to improve perception based on visual contrast.
- Keep condition marks sparse and strong.
- Prefer condition-first icons over cloud-first icons.
- Use a cloud only when it helps read the condition faster.

## Stroke And Simplification

1. When using lines, use heavy strokes that survive watch scale.
2. Use a fixed design grid for procedural glyphs. The current reference grid is `28x28`.
3. Keep visual footprint stable when state changes.
4. Scale glyphs and icons to fit compact grid sizes.

5. For layered same-color shapes:
- Draw the rear shape.
- Separate it with a background halo.
- Draw the foreground shape.

6. Use simplification to preserve recognition, not to strip away meaning.

## Stable Visual Footprint

- Keep glyphs stable inside a fixed frame.
- Keep value and state changes from shifting the visual footprint.
- Prefer visual continuity across display families.
- The user should see state change, not layout wobble.

## Color As State Support

- Display mode is a top-level product decision, not a post-process tint.
- Color is a state cue.
- Color is **not** the only source of meaning.

## Display Modes And Associated Palettes

### Modes

- At least two modes for every supported Pebble.

### Palettes

- Organize color decisions using hierarchical palettes.
 - One palette to display time, date, and metrics
 - Per-module palettes for state
 - Clear state change visibility
 - Consistent vocabulary per palette to indicate current state of all metrics
- Select colors in each palette as a whole; do not convey different states or information with the same color.
- Fallback rendering should remain legible when richer palettes are unavailable.
- If palette resolution fails, the display should degrade to a simple black-and-white presentation rather than hiding data.

## Absence Language

1. Out-of-range data uses a unique color and icon indicator.
2. Do not use question marks or exclamation point or pause bars (play/pause) for out-of-range data.
3. The mark should read as absent or out-of-range without feeling alarming.
4. Prefer bold slashes that start slightly above the canceled glyph and end slightly below it.

## Steps

Use a station-style footprint that reads as familiar and obvious at watch scale.

Direction:

- Filled footprints, with color fills to denote state (PBL_COLOR).
- Out-of-range steps fills the icon with out-of-range color plus the unavailable slash.

Do not encode step count progress with icon color or shape. The text value already carries the count.

## BPM

- Heart-rate monitor or ECG-style waveform icon, with color fills to denote state (PBL_COLOR).
- Out-of-range BPM uses the muted waveform plus the shared slash.

## Battery

- Battery uses a horizontal track rather than relying on text percentage in the primary layout.
- The outer track is a primary-color outline. The inner fill uses the module-owned battery state color.
- A plugged-in state must use a prominent and visible shape cue at watch scale.

## Weather

Weather uses a hybrid vocabulary.

- Black and white: bold line and outline first
- Color: fills and accents may be added when they improve recognition
- The same condition should keep the same metaphor across both capabilities

For weather, outline does not mean delicate. It means heavy sign strokes.

General rules:

- Use bold strokes for monochrome weather icons.
- Use fills or color accents only as secondary treatment on color displays.
- Keep condition marks sparse and strong.
- Prefer condition-first icons over cloud-first icons.
- Drop the cloud when it competes with the primary condition mark.
- Keep precipitation separate from the cloud baseline.
- Do not let rain or snow marks read as legs.

### Clear Sky

- Clear sky uses a simple disk, not a sun, moon, horizon, or text.

Rules:

- filled center circle
- 1px outer ring
- no rays
- no celestial detail

If the outer ring disappears on a monochrome target, the filled disk should still carry the meaning.

### Partly Cloudy

- Partly cloudy uses a subordinate sun (day) or clear orb (night) behind a reduced cloud.
- If the sun forces the cloud to become too small, show only the cloud.

### Fog

- Fog may omit the cloud.

Preferred direction:

- two or three strong horizontal bars
- bars heavy enough to survive black-and-white display
- add a cloud only if it does not reduce fog readability

## Review Workflow

Use `glyph-lab` for broad vocabulary work.

Recommended loop:

1. Define the glyph decision in prose.
2. Identify the real state variants used by the product.
3. Create or update lab variants only for those states.
4. Review color and black-and-white screenshots.
5. Review at least `aplite` and `emery` before production porting.
6. Pick one direction.
7. Port the selected drawing rules into production modules.
8. Validate in the real watchface layout.
