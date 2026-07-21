# Product Invariants

This document defines the product truths that must remain stable as the watch face evolves.

These are acceptance properties that every implementation must satisfy. They
are written to be simple to understand and verify.

## Adjacent

- [Design](Design.md) establishes product motivation and usability outcomes.
- [VisualVocabulary](VisualVocabulary.md) defines the visual grammar for these invariants.

## Read Next

- [RuntimeArchitecture](RuntimeArchitecture.md) describes the runtime structure that satisfies these invariants.
- [UserInterface](UserInterface.md) records the current visual realization.

## Glanceability & Automatic Perception

- The watch face must be immediately legible within a glance (sub-second attention span).
- The information must be understandable without decoding it field by field.
- The product hierarchy is stable across display families.
- Values and state changes do not shift the visual footprint.
- The user must gain perceptual fluency with the display over time.

## First-Class Device Support

- Every supported device receives an equally intentional experience within supported capabilities.

## Recognizable Information Hierarchy

- Information hierarchy remains recognizable across display families.
- Time, battery, weather, and date form the always-present core.
- Layout changes may rebalance spacing, but they must not change which information reads first, second, and third.

## Dominant Information

- **Time** remains the dominant visual object.
- Other information may add context without competing with time for attention.

## Identifiable Visual Elements

- Every visual element must be immediately identifiable at watch scale.

## Stable Semantic Meaning

- Every visual channel has one responsibility.
- Color, icon, glyph, and text choices must maximize glanceability within their assigned channel responsibilities.
- Users must retain the meaning of each channel within the same glance surface.

## Stable Color Meaning

- When different colors are used for the same metric, color should merely convey state change without alarm or seeking attention.
- Status meaning must stay consistent across the product.
- Color may add speed, confidence, and state meaning, but the display must remain interpretable when color is absent, muted, or unavailable.

## Out of Range Perception

- Out of range information is recognizable immediately without causing alarm or demanding attention.
