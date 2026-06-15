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

### Dirty visual and renderer changes need design review

Type: Risk
Status: Open

The current working tree changes rectangular geometry, date/climate alignment,
and shared text-layer background behavior. It builds, but it mixes product
layout movement with a renderer-wide visual rule.

Next action: review the prior and new flows before committing. Either approve
the visual rule intentionally or split renderer behavior from rectangular
layout work.

### Create failure paths may retain stale module state

Type: Issue
Status: Open

Some module create paths assign retained surface or palette pointers before all
required resources exist, and one path allocates a bitmap before required text
layer creation. If creation then fails, watchface may not record the stratum
as created and therefore will not call that module's destroy path.

Next action: make create paths transaction-like. On failure, destroy resources
created in that function and clear retained pointers before returning false.

### Watchface refresh should gate every created stratum

Type: Issue
Status: Open

`watchface_refresh_strata()` gates optional background, climate, BPM, and steps
refreshes by `s_strata_created_mask`, but required date, time, and battery
refreshes are called whenever their update bit is set. Required modules are
expected to exist after successful create, but failure and teardown paths are
safer if the mask is used consistently.

Next action: gate date, time, and battery refreshes by creation mask in the
same focused lifecycle cleanup slice as create failure hardening.

### Renderer allocation strategy in draw path

Type: Issue
Status: Open

The shared filled-polygon renderer allocates scaled point storage in a drawing
path even though the point count is bounded. Its `gpath_create()` failure path
also returns without freeing that allocation.

Next action: avoid per-frame heap allocation without wasting stack. Use exact
primitive-specific storage, fixed static storage, or a bolt-specific draw path;
ensure every acquired resource is released on all return paths.

### Revisit rectangular architect header after Round

Type: Assumption
Status: Open

`layout_rect.h` currently contains rectangular constants, blueprint objects,
and calculated layout structs. Keeping that split is acceptable while the code
moves toward a consolidated architect file, because putting everything in
`layout_rect.c` made the active layout harder to read.

Next action: revisit after Round lands. Decide whether rectangular and round
architect details should live in shape headers or be rationalized into one
architect implementation.

### Gabbro debug override can become accidental product geometry

Type: Risk
Status: Open

Round layout has a `DEBUG_ATAGLANCE && PBL_PLATFORM_GABBRO` geometry override,
while debug is currently globally enabled in `ataglance.h`. If Gabbro enters
`targetPlatforms` before debug ownership is cleaned up, hard-coded debug
geometry can silently become the active product layout.

Next action: move debug enablement out of product constants before enabling
round targets, and require explicit review for any platform debug override.

### DEBUG_ATAGLANCE gate style needs full audit

Type: Issue
Status: Open

Debug code was previously gated by presence checks such as
`#ifdef DEBUG_ATAGLANCE`, so defining `DEBUG_ATAGLANCE 0` still compiled debug
paths. Live code is moving to value checks, but every debug-gated branch should
be reviewed before this is considered closed.

Next action: audit all `DEBUG_ATAGLANCE` gates and standardize on value checks
such as `#if DEBUG_ATAGLANCE` or `#if DEBUG_ATAGLANCE == 1`.

### Weather responses can arrive out of order

Type: Risk
Status: Open

PebbleKit JS starts weather fetches on ready and on an interval. Network or
geolocation delays can allow older requests to complete after newer ones and
overwrite fresher weather data.

Next action: add a small request sequence or in-flight policy so weather
updates are applied deliberately.

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

### Battery text drops the percent sign

Type: Decision
Status: Accepted

Battery text no longer shows `%`. This is a product decision to reduce visual
noise and align compact battery text width with BPM text width, since the
left-most digit is `1`.

### Keep the main/watchface boundary narrow

Type: Decision
Status: Accepted

`main.c` parses AppMessage transport tuples and converts them to typed runtime
facts before calling narrow watchface setters. `watchface` should not parse
raw dictionaries or take a shared runtime-update package unless message volume
or atomic multi-field coherence makes the current setter path meaningfully
worse.

### Weather subframe call sites may hide overlap or clipping drift

Type: Risk
Status: Open

`weather_subframe()` remains local by design, but its call sites can hide
composition problems when sub-frames are tuned by eye rather than derived
from a stable glyph contract. Partly cloudy is the current concrete example.

Next action: audit every `weather_subframe()` call site with emulator
screenshots and confirm that nested cloud/sun/rain/snow frames are deliberate
for each weather glyph.
