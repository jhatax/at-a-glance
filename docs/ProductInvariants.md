# Product Invariants

This document defines the product truths that should remain stable as the watchface evolves.

These are acceptance properties, not current implementation notes.

## Required Reading

- `../README.md`
- `Design.md`
- `VisualVocabulary.md`

## Glanceability

- A user should be able to understand the watchface within a glance (sub-second attention) without decoding information field by field.
- Keep the product hierarchy stable across display families.
- Keep value and state changes from shifting the visual footprint unnecessarily.

## Dominant Time

- Time remains the dominant visual object.
- Other information may add context, but it must not compete with time for first attention.

## Recognizable Information Hierarchy

- Information hierarchy remains recognizable across display families.
- Rectangular and round targets should feel like the same product.
- The invariant information hierarchy is: top steps context, centered time, centered battery track, weather/date context, and bottom heart-rate context (time, battery, weather, date supported on all Pebbles).
- Rectangular and round displays implement the same information hierarchy and stack.
- Layout changes may rebalance spacing, but they should not change which information reads first, second, and third.

## First-Class Device Support

- Every supported device deserves an equally intentional experience within the capabilities it supports.
- Intentional does not mean identical.
- Platform differences must not reduce the product to a careless port.
- The product should adapt to capability differences without treating any supported device as an afterthought.

## Identifiable Visual Elements

- Icons identify information.
- Numbers quantify values.
- Text is primary; icons support recognition.
- A text-only metric is acceptable when an icon cannot be created or does not fit the platform.
- An icon-only metric is not acceptable.
- Every icon and glyph must work in black and white.
- Identifiability matters at watch scale, not only in enlarged mockups or editor views.

## Stable Semantic Meaning

1. Every visual channel has one responsibility:

- Text = Exact values.
- Icons = Metric identification.
- Bars = Progress.
- Color = State.

2. Color an icon or glyph vs. text to maximize glanceability.

3. Users should not have to relearn the meaning of a channel from one metric to another.

## Stable Color Meaning

- Use color as a state cue, not as the only source of meaning.
- Dynamic metric colors may vary by module or state.
- Status meaning should stay consistent across the product.
- Color may add speed and confidence, but the display must remain interpretable when color is absent, muted, or unavailable.
- State meaning should survive across monochrome and color-capable devices.

## Calm Availability Language

- Unavailable data uses the accepted absence slash.
- The slash reads as absent or unavailable.
- The mark means absent, unavailable, or not currently usable.
- The mark should be visible without feeling alarming.
- Unavailable states should read as calm product behavior, not as an error condition demanding attention.
