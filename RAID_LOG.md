# RAID Log

Use this lightweight log alongside `ARCHITECTURE_LEDGER.md` for non-trivial
layout, platform, lifecycle, or API work. It should not replace the
architecture ledger or the user's notebook.

RAID means:

- Risk: a future problem that may occur.
- Assumption: something currently treated as true but not yet proven.
- Issue: an active problem already affecting the work. Active architecture
  issues usually belong in `ARCHITECTURE_LEDGER.md` or the user's notebook;
  only duplicate them here when they materially affect a current risk,
  assumption, or decision.
- Decision: an accepted direction or tradeoff.

Keep entries short and current. Prefer risks, assumptions, and decisions here;
use issue entries sparingly so this file does not become a competing ledger.
Close or update stale entries instead of letting them become hidden
architecture.

## Active Entries

### Compact rectangular layout may inherit Emery-native assumptions

Type: Risk
Status: Open

The original rectangular layout treated Emery as the design reference and used
scaling to derive smaller rectangular geometry. That can make compact targets
such as Aplite and Flint compile correctly while still looking non-native.

Next action: evaluate Aplite and Flint together with emulator screenshots,
then decide compact-specific icon size, row gaps, column gaps, font choices,
and text-layer dimensions as product decisions rather than passive scaling.

### Compact rectangular design authority

Type: Assumption
Status: Open

The compact rectangular design should be evaluated as its own product layout
class, not only as a scaled Emery derivative.

Next action: confirm whether Aplite, Flint, or a shared compact rectangular
policy is the design authority for the next layout slice.

### Reuse architecture, not geometry

Type: Decision
Status: Accepted

The code should reuse the architect/stylist/surface architecture across
platforms. It should not assume that product geometry, icon dimensions, text
dimensions, or spacing can be reused by scaling from another device class.

### Glyph lab is the validation gate for glyph work

Type: Decision
Status: Accepted

Validate non-trivial glyph changes in `tools/glyph-lab` before porting them
into the main watchface tree. Use the lab to test size classes and visual
contracts first, then port one glyph slice at a time.

### Unavailable slash uses a fixed 3px design-space stroke

Type: Decision
Status: Accepted

The unavailable slash is a shared glyph contract, not a per-module styling
choice. Keep the diagonal at `5,5 -> 24,24` with design-space stroke width
`3`, centralized in `substratum_renderer`.

### Shared glyph primitives belong in substratum_renderer

Type: Decision
Status: Accepted

When multiple modules reuse the same glyph primitive math, move that math into
`substratum_renderer` instead of maintaining parallel local helpers. This now
includes unavailable slash rendering, scaled polygon outlines, and
frame-aware line/circle/corner-derived fill helpers.

### Weather subframe call sites may hide overlap or clipping drift

Type: Risk
Status: Open

`weather_subframe()` remains local by design, but its call sites can hide
composition problems when sub-frames are tuned by eye rather than derived
from a stable glyph contract. Partly cloudy is the current concrete example.

Next action: audit every `weather_subframe()` call site with emulator
screenshots and confirm that nested cloud/sun/rain/snow frames are deliberate
for each weather glyph.
