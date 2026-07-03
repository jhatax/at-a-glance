# At A Glance Design

At A Glance is a glance-first Pebble watchface built to make complex information simple to perceive automatically, with minimal cognitive load.

## Product Intent

At A Glance presents information with minimal interpretation.

The interface should present information directly, without explanatory friction.

Complex information should be simple to perceive.

Nothing on screen should explain the data when the data can be shown clearly on its own.

## Design Influences

Inspired by transit signage and cockpit instrumentation.

These influences show up in clear hierarchy, strong alignment, restrained color, and direct labeling instead of decoration.

## Glance-First Philosophy

Maximize information capture while minimizing the effort and attention required to understand it.

Design for automatic perception before deliberate reading.

Reward a quick glance. A user should not need to stop and decode the layout.

The design uses the principle of irreducible simplicity: reduce each element until any further reduction would compromise function, recognition, or glanceability.

## Restraint And Negative Space

Negative space is part of the design.

The display should feel ordered, not crowded.

Visual restraint is part of the product, not leftover space.

The goal is to keep the hierarchy clear and the screen calm, even when several metrics are present.

## Intentional Experience Across Devices

Every supported device deserves an equally intentional experience within the capabilities it supports.

Intentional does not mean identical.

Differences in shape, color capability, and screen class may change the layout, spacing, and treatment.

Those differences should not make any supported platform feel like a scaled copy or an afterthought.

## Shared Visual DNA

Rectangular and round targets should feel like the same product.

Rectangular and round displays implement the same information hierarchy and
stack. Placement is resolved from defined layout constants.

The shared stack, dominant time, centered battery treatment, weather/date row, and stable hierarchy should preserve that family resemblance across platforms.

Shared visual DNA is about recognition, not about reusing the same coordinates on every device.

## Design Invariant

Automatic perception of information is the singular design invariant for this watch face.

- A user should be able to understand the display quickly, with minimal effort and minimal interpretation.
- If a change improves implementation neatness but makes the display slower to read, it fails the design invariant.

## Further Reading

- [Product Invariants](Product-Invariants.md) describes the functional and semantic rules that preserve the product identity across devices, layouts, and feature configurations.
