# Product Invariants

This document defines the product truths that must remain stable as the
watchface evolves.

These are acceptance properties that must be satisfied by any implementation,
so they have been written to be simple to understand and verifiable.

It is the task of implementation documentation to cover decisions made to
satisfy these properties.

**Key document relationships**

`Design` establishes the motivation, foundational concepts, and overarching usability outcomes
for *At A Glance*, the entire watch face.
|
V
`ProductInvariants`, i.e. `this`, states **what** must be true to achieve usability outcomes across devices.
|
V
`VisualVocabulary` defines the visual grammar that *can* satisfy visual invariants.

## Glanceability & Automatic Perception

- The watchface must be immediately legible within a glance (sub-second
  attention span).
- The information should be understandable without the need to decode
  field by field.
- The product hierarchy is stable across display families.
- Values and state changes do not shift the visual footprint.
- The user should gain perceptual fluency with the display over time.

## First-Class Device Support

- Every supported device receives an equally intentional experience within
  supported capabilities.

## Recognizable Information Hierarchy

- Information hierarchy remains recognizable across display families.
- Time, battery, weather, and date form the always-present core.
- Layout changes may rebalance spacing, but they must not change which
  information reads first, second, and third.

## Dominant Information

- **Time** remains the dominant visual object.
- Other information may add context, but it must not compete with time for first attention.

## Identifiable Visual Elements

- Every visual element must be immediately identifiable at watch scale.

## Stable Semantic Meaning

1. Every visual channel has one responsibility:

- Text = Exact values.
- Icons = Metric identification.
- Bars = Progress.
- Color = State.

2. Color an icon or glyph vs. text to maximize glanceability.

3. Users must not have to relearn the meaning of a channel within the same glance surface.

## Stable Color Meaning

- When different colors are used for the same metric, color should merely
  convey state change without alarm or seeking attention.
- Status meaning must stay consistent across the product.
- Color may add speed, confidence, and state meaning, but the display
  must remain interpretable when color is absent, muted, or unavailable.

## Out of Range Perception

- Out of range information is recognizable immediately without causing alarm or
  demanding attention.
