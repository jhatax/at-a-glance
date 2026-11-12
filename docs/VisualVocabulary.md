# Visual Vocabulary

This document defines the visual rules that preserve legibility and automatic perception across supported Pebble displays. It assigns meaning to text, icons, lines, and color, and governs composition, glyphs, palettes, and unavailable states.

## Adjacent

- [ProductInvariants](ProductInvariants.md) defines the product properties this visual grammar must satisfy.

## Read Next

- [UserInterface](UserInterface.md) records the current visual realization.

## Visual Channels & Responsibilities

| Channel | Responsibility |
| --- | --- |
| Text and numbers | Present precise information and exact values. |
| Icons | Identify metrics and weather conditions. |
| Lines and tracks | Show progress or level. |
| Color | Reinforce state. |

Present data directly. Keep each channel's meaning stable as values, states, and display modes change.

## Immediate Identifiability

- Text is the primary glance surface for time, date, weather, and health values.
- Text-only metrics are acceptable when an icon cannot be created or does not fit the platform.
- Weather, steps, and heart rate retain text when their icons are absent. Battery level uses a progress track and a charging mark.
- Every icon and glyph must work in black and white.
- Validate recognition at watch scale.

## Surface Composition

Keep the information hierarchy recognizable across display sizes and shapes. Time remains dominant, with weather and date providing context. Anchor each metric to a stable position and use negative space to separate it from neighboring information.

Battery progress may occupy the central stack or the right margin. Supporting icons and optional health progress may be absent; required information remains legible.

## Glyph and Icon Selection

Use familiar metric and condition metaphors that remain recognizable in color, monochrome, compact, full, rectangular, and round presentations. Use custom glyphs when existing marks do not provide clear recognition.

- Use filled metric marks.
- Climate glyphs use outlines and sparse condition marks. Fill shapes where it improves contrast or separates overlapping forms.
- Battery and power state remain identifiable through track geometry and the charging bolt; color supports recognition.

## Icon Recognizability

- Prefer fewer, stronger marks over dots, texture, wisps, and small accents.
- Make the primary metaphor large.
- Drop secondary accents when they force the primary shape to shrink.

## Silhouette

- Make the outer shape recognizable before adding internal detail.
- Prefer solid silhouettes for metric marks.
- Use line art for condition marks and other information carried by the line itself.

### Climate glyphs

- Keep condition marks sparse and strong.
- Prefer condition-first icons over cloud-first icons.
- Use a cloud only when it helps read the condition faster.

## Stroke And Simplification

- Use strokes that remain visible at watch scale.
- Use a fixed design grid for procedural glyphs and simplify small frames where needed.
- Preserve the distinguishing shape when removing detail.

For overlapping same-color shapes, draw the rear shape, separate it with a background halo, then draw the foreground shape.

## Stable Visual Footprint

- Keep glyphs stable inside a fixed frame.
- Prefer visual continuity across display families.
- Value and state changes preserve alignment and surrounding spacing.

## Color As State Support

- Display mode is a top-level product decision that repaints the watch face.
- Shape, text, progress, and placement carry meaning when color is absent or muted.

## Display Modes And Associated Palettes

### Modes

- Provide light and dark modes on every supported Pebble.

### Palettes

- Organize color decisions using hierarchical palettes.
- Select colors in each palette as a whole. Keep state colors consistent within each metric.
- State changes remain visible on monochrome devices or when richer palettes are unavailable.
- Use a legible monochrome palette when palette selection needs a fallback.

## Absence Language

- Unavailable numeric data uses `---` and a slashed icon when present. Color reinforces absence where the palette supports it.
- Use the shared absence slash for out-of-range states. Question marks, exclamation points, and playback symbols have different meanings.
- Keep the slash bold and long enough to remain recognizable across the glyph.

## Visual Validation

Validate changed glyphs, palettes, and layouts on the affected display capabilities. Inspect recognition, legibility, stable placement, and state meaning at watch scale. [Validation](Validation.md) owns the evidence requirements.
