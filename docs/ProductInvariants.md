# Product Invariants

This document defines the product truths that should remain stable as the watchface evolves.

These are acceptance properties, not current implementation notes.

## Required Reading

- `../README.md`
- `Design.md`
- `VisualVocabulary.md`

## Dominant Time

- Time is always dominant.
- Time remains the dominant visual object.
- Other information may add context, but it must not compete with time for first attention.

## Recognizable Information Hierarchy

- Information hierarchy remains recognizable across display families.
- Rectangular and round targets should feel like the same product.
- The shared hierarchy is: top steps context, dominant centered time, centered battery track, weather/date context, and bottom heart-rate context.
- Geometry can differ by shape without changing the hierarchy.
- Layout changes may rebalance spacing, but they should not change which information reads first, second, and third.

## First-Class Device Support

- Every supported device deserves an equally intentional experience within the capabilities it supports.
- Intentional does not mean identical.
- Platform differences may change geometry, spacing, and visual treatment.
- Platform differences must not reduce the product to a careless port.
- The product should adapt to capability differences without treating any supported device as an afterthought.

## Identifiable Visual Elements

- Icons identify information.
- Numbers provide values.
- Text is primary; icons support recognition.
- A text-only metric is acceptable when an icon cannot be created or does not fit the platform.
- An icon-only metric is not acceptable.
- Every icon and glyph must work in black and white.
- Identifiability matters at watch scale, not only in enlarged mockups or editor views.

## Stable Semantic Meaning

Every visual channel has one responsibility.

- Text provides exact values.
- Icons identify the metric.
- Lines show progress.
- Color indicates state.
- Do not make one channel carry another channel's job when the product can avoid it.
- Do not encode step count progress with icon color or icon shape. The text value already carries the count.
- Users should not have to relearn the meaning of a channel from one metric to another.

## Stable Color Meaning

- Use color as a state cue, not as the only source of meaning.
- Dynamic metric colors may vary by module or state.
- Status meaning should stay consistent across the product.
- Color may add speed and confidence, but the display must remain interpretable when color is absent, muted, or unavailable.
- State meaning should survive across monochrome and color-capable devices.

## Glanceability

- Design for one-second recognition.
- Keep the product hierarchy stable across display families.
- Keep value and state changes from shifting the visual footprint unnecessarily.
- The display should reward quick reading, not inspection.
- A user should be able to understand the watchface at a glance without decoding it field by field.

## Calm Availability Language

- Unavailable data uses the accepted absence slash.
- The slash reads as absent or unavailable.
- The mark means absent, unavailable, or not currently usable.
- The mark should be visible without feeling alarming.
- Unavailable states should read as calm product behavior, not as an error condition demanding attention.
