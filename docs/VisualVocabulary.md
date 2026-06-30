# Visual Vocabulary

This document defines how At A Glance expresses meaning visually.

It covers the shared watchface composition, channel responsibilities, glyph rules, display-mode behavior, and absence language. It does not own product commitments, runtime architecture, or current geometry tables.

## Required Reading

- README.md
- Design.md
- ProductInvariants.md

## Visual Channels & Responsibilities

Each visual channel should have one primary visual expression.

- Text for exact values
- Icons for metric identity
- Lines for progress
- Color for state

Nothing on screen should interpret the data when direct presentation is enough.

Text is the primary glance surface. Icons support recognition, but they do not carry the product alone.

## Why Custom Glyphs

The goal is not decorative detail. The goal is glanceable, compact, confident glyphs that work across monochrome and color modes.

Use custom glyphs when the product needs stronger recognition than borrowed icon sets can provide.

A text-only metric is acceptable when an icon cannot be created or does not fit the platform.

An icon-only metric is not acceptable.

Create review variants in `glyph-lab` before porting a glyph into production code.

## Icon Recognizability

Design for one-second recognition.

Prefer fewer, stronger marks over dots, texture, wisps, and small accents.

Make the primary metaphor large.

Drop secondary accents when they force the primary shape to shrink.

Every glyph must work in black and white.

A glyph that works only when enlarged, or only in color, is not good enough.

## Silhouette

Prefer solid filled silhouettes over hollow outlines.

Use line art only when the line itself is the message.

Keep condition marks sparse and strong.

Prefer condition-first icons over cloud-first icons.

Use a cloud only when it helps the condition read faster.

The silhouette should read before its internal detail does.

## Stroke And Simplification

When using lines, use heavy strokes that survive watch scale.

Use a fixed design grid for procedural glyphs. The current reference grid is `28x28`.

Keep the visual footprint stable when state changes.

For layered same-color shapes:

1. Draw the rear shape.
2. Separate it with a background halo.
3. Draw the foreground shape.

Use simplification to preserve recognition, not to strip away meaning.

## Stable Visual Footprint

Keep glyphs stable inside a fixed frame.

Keep value and state changes from shifting the visual footprint.

Prefer visual continuity across display families even when geometry changes.

The user should see state change, not layout wobble.

## Color As State Support

Color is a state cue.

It is not the only source of meaning.

Display mode is a product decision, not a post-process tint.

Fallback rendering should remain legible when richer palettes are unavailable.

If palette resolution fails, the display should degrade to a simple black-and-white presentation rather than hiding data.

## Surface Composition

The shared watchface composition is:

```text
top heart-rate context
dominant centered time
centered battery track and charging bolt
weather/date context
bottom steps context
```

This stack is part of the product's visual DNA. Rectangular and round displays may use different geometry, but they should still read as the same watchface.

Visual guidance:

- Keep time visually dominant.
- Keep the battery track directly under time.
- Keep weather and date on one context row.
- Keep BPM on the top context row and steps on the bottom context row on health platforms.
- Keep health metrics on centerline vocabulary rather than far-corner placement.
- Preserve negative space instead of filling every pixel.

## Display Modes And Palette Fallback

The live display-mode families are:

1. `Black on White`
2. `White on Black`
3. `Clear as Celeste`
4. `Night in Oxford`

On color-capable watches, all four modes are available. The first two are monochrome-style presentations rendered on color hardware. The latter two are the color-background palettes.

Base text, date, time, and unavailable colors come from the selected display mode. Module-owned colors then layer on top for battery, BPM, steps, and climate state.

Showing legible data in black and white matters more than preserving a richer accent palette.

## Absence Language

Unavailable data uses the accepted absence slash:

```text
\
```

Do not use question marks for unavailable data.

Do not use exclamation points for unavailable data.

Keep pause bars as rejected history.

The mark should read as absent or unavailable without feeling alarming.

Meaning examples:

- waveform + slash
- footprint + slash
- cloud + slash

Use a bold slash. Start slightly above the canceled glyph and end slightly below it.

## Steps

Use a station-style footprint that reads as familiar and obvious at watch scale.

Direction:

- one large filled ball-of-foot circle
- one background-color separator cut
- one smaller filled heel circle
- no toe dots
- no shoe detail

Do not encode step count progress with icon color or shape. The text value already carries the count.

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

Clear sky uses a simple disk, not a sun, moon, horizon, or text.

Rules:

- filled center circle
- 1px outer ring
- no rays
- no celestial detail

If the outer ring disappears on a monochrome target, the filled disk should still carry the meaning.

### Partly Cloudy

Partly cloudy uses a subordinate sun behind a dominant reduced cloud.

If the sun forces the cloud to become too small, show only the cloud.

### Fog

Fog may omit the cloud.

Preferred direction:

- two or three strong horizontal bars
- bars heavy enough to survive black-and-white display
- add a cloud only if it does not reduce fog readability

## BPM

BPM uses a heart-rate monitor or ECG-style waveform icon, not a heart.

Preserve BPM state color on color displays.

Unavailable BPM uses the muted waveform plus the shared slash.

## Battery

Battery uses a horizontal track rather than relying on text percentage in the primary layout.

The outer track is a primary-color outline. The inner fill uses the module-owned battery state color.

Charging must use a visible shape cue, not color alone.

The charging bolt should be prominent enough to read at watch scale.

## Round Displays

Round displays preserve the same text-first hierarchy while using round-aware geometry.

Use the same surface stack before inventing a separate round-only product.

Confirm readable chord width at each y-position.

Disable optional icons before allowing text to clip.

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
