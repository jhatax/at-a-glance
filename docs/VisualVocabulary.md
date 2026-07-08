# Visual Vocabulary

This document defines rules for how the watch face can satisfy legibility
and automatic perception invariants visually.

Rules and ideas should apply to shared watch face composition, channel
responsibilities, glyph selection, color selection, display-mode behavior,
and representing out-of-range information.

These rules must stay relevant across implementations.

**Key document relationships**

`Design` establishes the motivation, foundational concepts, and overarching usability outcomes
for the watch face.
|
V
`ProductInvariants` states **what** must be true to achieve usability outcomes across devices.
|
V
`VisualVocabulary`, i.e. `this`, defines the visual grammar that *can* satisfy visual invariants.

## Visual Channels & Responsibilities

1. Each visual channel has one primary visual expression.

- Text for exact values
- Icons for metric identity
- Lines for progress
- Color for state

2. Nothing on screen interprets the data when direct presentation is enough.

3. Text is the primary glance surface. Icons support recognition, but they do
   not carry the product alone.

## Immediate Identifiability

Immediate identifiability satisfies automatic perception.

- Text is primary and carries exact values.
- Icons identify information and support recognition.
- Numbers quantify values.
- Text-only metrics are acceptable when an icon cannot be created or does not
  fit the platform.
- Icon-only metrics are not acceptable.
- Every icon and glyph must work in black and white.
- Identifiability matters at watch scale, not only in enlarged mockups or
  editor views.

## Surface Composition

A shared watch face composition across all Pebbles satisfies
anchoring information to stable locations on screen.

```text
top steps layer with progress and state
dominant bold and centered time
centered battery track and plugged-in bolt
weather/date context
bottom heart-rate layer with state
```

- This stack is part of the product's visual DNA.
- All devices (different sizes and geometries) implement the same information
  hierarchy and stack.
- Text conveys information precisely and independent of supporting visual
  channels.
- Icons and progress bars support text in glanceability but can be hidden.
- Negative space is used to serve glanceability.

**Display hierarchy invariant** is the location and order in which time,
battery track, weather, and date are displayed.

## Glyph and Icon Selection

1. All icons and glyphs satisfy the principal design invariant of automatic
   recognition on all supported Pebble devices:

- Color, Monochrome
- Compact, Full
- Rectangular, Round

2. Use custom glyphs when the product needs stronger recognition
   than borrowed icon sets can provide.

3. Color glyphs to show progress only when glanceability is enhanced.

4. Product direction to satisfy recognition and maintain distinction:

- Metric glyphs are filled.
- Climate glyphs are outlines, with recorded exceptions for contrast / separation.

## Icon Recognizability

- Prefer fewer, stronger marks over dots, texture, wisps, and small accents.
- Make the primary metaphor large.
- Drop secondary accents when they force the primary shape to shrink.

## Silhouette

- The silhouette conveys meaning without internal detail inspection.
- Use solid filled silhouettes over hollow outlines.
- Use line art only when the line itself is the message.

### Climate glyphs

- Use silhouettes over filled glyphs, except when filling the glyph
  improves perception or creates visual contrast.
- Keep condition marks sparse and strong.
- Prefer condition-first icons over cloud-first icons.
- Use a cloud only when it helps read the condition faster.

## Stroke And Simplification

1. When using lines, use heavy strokes that survive watch scale.

2. Use a fixed design grid for procedural glyphs.

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
- The user must see state change, not layout wobble.

## Color As State Support

- Display mode is a top-level product decision that repaints the watch face.
- Color is a state cue.
- Color is **not** the only source of meaning.

## Display Modes And Associated Palettes

### Modes

- At least two modes for every supported Pebble.

### Palettes

- Organize color decisions using hierarchical palettes.
- Select colors in each palette as a whole; do not convey different
  states or information with the same color.
- Each palette uses consistent vocabulary for current metric state.
- State changes remain visible on monochrome devices or when richer
  palettes are unavailable.
- If palette resolution fails, the display must degrade to a simple
  black-and-white presentation rather than hiding data.

## Absence Language

1. Out-of-range data uses a unique color and icon indicator that doesn't create alarm or demand attention.
2. Do not use question marks, exclamation points, or pause bars (play/pause) for out-of-range data.
3. Prefer bold slashes that start slightly above the canceled glyph and end slightly below it.

## Metric Icons And Glyphs

Icons identify every metric: heart rate, walking steps, battery or power
state, and weather conditions.

- Text carries values; icons identify the metric.
- Progress and state cues support the metric, but they do not replace text.
- Metric marks should use familiar metaphors that read immediately at watch
  scale.
- Weather glyphs should be condition-first and use clouds only when they
  improve recognition.
- Battery and power state must remain identifiable without relying on color
  alone.
- Color may reinforce state, but shape, silhouette, and placement carry the
  primary recognition burden.

## Visual Validation

Visual validation is critical evidence for Product Invariants. A rule is not
confirmed until the product remains immediately identifiable, glanceable, and
semantically stable on the supported display capabilities it affects.

## Further Reading

- `UserInterface.md` for the full visual reference implementation.
