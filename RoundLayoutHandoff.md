# Round Layout Implementation Handoff

This handoff is for the agent who will continue round-display layout work
from the current `WatchfaceSurface` architecture.

It is not approval to enable `chalk` or `gabbro` immediately. It is not
approval to redesign rectangle, change AppMessage behavior, change palette,
or move module lifecycle ownership.

## Required Reading

Read these first, in order:

1. `ARCHITECTURE_LEDGER.md`
2. `RoundWatchfacePlan.md`
3. `layout-restructuring-plan.md`
4. `agents.md`

If any inherited guidance in `agents.md` cannot be read, stop and say so
before editing.

## Operating Protocol

Use the project cycle without shortcuts:

1. Audit current symbols, includes, call sites, platform guards, and docs.
2. State exact patch shape before editing.
3. Keep one coherent slice at a time.
4. Preserve rectangular behavior unless the user explicitly approves a visual
   rectangle change.
5. Validate after each source slice with `git diff --check` and
   `pebble build`.
6. Use emulator screenshots before approving any round geometry.
7. Reconcile against the ledger before claiming the slice is complete.

If a broken pattern is found, search sibling modules and analogous call sites
before finishing the slice. Fix matching cases only when the ownership
boundary and risk profile match.

## Current Architecture

The relevant current flow is:

```c
watchface_create()
  -> layout_watchface_initialize(face_width,
                                 face_height,
                                 &s_surface)
       -> memset(surface, 0, sizeof(*surface))
       -> apply active shape geometry
  -> layout_update_watchface_style(&surface->style, display_mode)
```

Round uses the same public entry point:

```c
memset(surface, 0, sizeof(*surface));
/* apply active shape geometry */
layout_update_watchface_style(&surface->style, display_mode);
```

`layout.h` is the only public surface-construction boundary. Feature modules
and `watchface.c` must not include private architect or stylist headers.

## Current Round Slice State

The scaffolding slice has already landed:

- `src/modules/layout_round.c` exists
- round and rectangular geometry share the public
  `layout_watchface_initialize()` contract
- `package.json` targets remain rectangular-only until round geometry is
  reviewed and accepted
- rectangular targets continue to build

Do not pass or return `WatchfaceSurface` by value.

## Round Architect Responsibility

`layout_round.c` calculates geometry only. It fills final frames and flags on
the existing surface:

- `surface->background`
- `surface->date.text`
- `surface->time.text`
- `surface->battery.icon`
- `surface->battery.text`
- `surface->climate.icon`
- `surface->climate.text`
- `surface->steps.icon`
- `surface->steps.text`
- `surface->bpm.icon`
- `surface->bpm.text`

It may use private file-local metric structs and helpers, such as
`LayoutRoundMetrics`, row safe spans, and integer square-root helpers.

It must not:

- create Pebble `Layer` or `TextLayer` objects
- set update procs
- call feature modules
- decide runtime data availability
- change AppMessage behavior
- own palette or font resolution
- change custom font lifecycle
- add generic dynamic strata arrays
- expose intermediate geometry through `watchface_components.h`

## Shape Macros

Use Pebble shape macros for architect dispatch:

- `PBL_RECT` for rectangular geometry
- `PBL_ROUND` for round geometry

Use capability macros only for their actual capability:

- `PBL_COLOR` / `PBL_BW` for color behavior
- `PBL_HEALTH` for health behavior

Avoid `PBL_PLATFORM_CHALK` and `PBL_PLATFORM_GABBRO` for general geometry.
Use them only if a model-specific exception is proven by screenshots and
cannot be represented through face dimensions or round-safe spans.

## Geometry Principles

Round layout needs row-specific safe spans. Do not scale the rectangle
layout wholesale.

Use integer math only:

```c
diameter = min(face_width, face_height);
radius = diameter / 2;
center_x = face_width / 2;
center_y = face_height / 2;
safe_radius = radius - edge_margin;

dy = max(abs(row_y - center_y), abs((row_y + row_h - 1) - center_y));
half_width = isqrt((safe_radius * safe_radius) - (dy * dy));
safe_x = center_x - half_width;
safe_w = half_width * 2;
```

Use the farthest vertical edge of each row band when calculating `dy`.
This protects the full text or icon frame, not just the row centerline.

Use the active round blueprint's content margin as the edge margin. Do not add
a separate optical inset unless screenshots later prove a distinct round-only
correction is needed.

Every known substratum should receive its own final `GRect`. Do not make
callers reconstruct positions from shared row metrics.

## Product Layout Direction

The round layout should feel related to the current rectangular face:

```text
Top context: battery and climate
Dominant centered time
Centered rule
Centered date
Bottom health context: steps and bpm
```

Chalk (`180x180`) is the hard target. Start text-first and compact:

- time remains centered and dominant
- rule remains centered and short enough for its safe span
- date remains centered
- top context may need stacking or compact placement
- bottom health may need text-first or stacked icon/text treatment
- icons are optional when they do not fit cleanly

Gabbro (`260x260`) is full round, but still uses the same safe-span model.
Do not scale rectangle just because Gabbro has more room.

## Style And Typography Boundary

`layout_stylist.c` owns style decisions:

- palette selection
- font-role mapping
- custom time font resource IDs

The active shape architect owns compact/full classification because it owns
geometry. It calculates compact/full once from its blueprint and face
dimensions, stores compact/full on `WatchfaceSurfaceStyle`, and the stylist
consumes that stored state. With the current rectangular blueprint, compact
means face width below `200` or face height below `228`.

Do not move font decisions into `time.c` or `layout_round.c`. The round
architect assigns geometry; the stylist resolves typography.

## Module Lifecycle Boundary

Feature modules continue to own their Pebble layers, buffers, update procs,
refresh behavior, source state, and destroy paths.

Do not move layer creation into layout or watchface. The architect calculates
substratum data; modules create/update/destroy layers from those substrata.

Creation semantics remain:

- required text layer creation determines success for text-bearing modules
- optional icons may fail without failing the module
- modules assign retained `s_surface` only after required layer creation
  succeeds
- custom fonts/resources are released on create failure and destroy

## Watchface And Main Boundary

`watchface` owns the live `WatchfaceSurface` and module creation order.
`main.c` knows update categories and calls watchface APIs only.

Round support must not make `main.c` aware of:

- layout shape details
- feature modules
- strata masks
- palette/style internals
- individual Pebble layers

`watchface_refresh()` remains the runtime dispatch boundary.

## Validation Sequence

For the scaffolding slice:

1. Run `git diff --check`.
2. Run `pebble build`.
3. Confirm rectangular targets still build.
4. Review diff for header leakage and behavior drift.

For first round geometry:

1. Enable `chalk` only after the next round geometry slice is ready.
2. Build and install Chalk.
3. Capture screenshots in dark and light mode if practical.
4. Check clipping for time, date, rule, top context, and bottom health.
5. Only then consider enabling `gabbro`.

For any visual approval:

- inspect screenshots, not only calculated frames
- check color and black-and-white behavior
- test unavailable health/weather states
- test time extremes such as `23:59` and `12:59`
- test long date extremes such as `WED · 30 SEP`

## Explicit Non-Goals For The First Round Slice

- no rectangle redesign
- no palette changes
- no custom font changes
- no AppMessage key changes
- no debug-health behavior changes
- no module lifecycle refactor
- no feature-module shape branching unless proven necessary
- no generic row engine or dynamic strata array
- no public exposure of round metrics

## Commit Hygiene

Commit round support in focused slices:

1. round geometry refinement for Chalk
2. target enablement and screenshot-reviewed adjustments
3. Gabbro adjustments if needed
4. docs updates after the visuals are proven

Stage only intended files. Do not commit screenshots, build artifacts, or
scratch files unless the user explicitly asks.
