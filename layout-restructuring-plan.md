# Layout Restructuring Plan

This plan captures the intended cleanup after the working
`WatchfaceSurface` refactor. The current implementation works, but `layout.c`
now owns too many adjacent responsibilities in one file. The goal is to
realign the architecture without changing visible watchface behavior.

This is a behavior-preserving restructuring plan, not round support.

## Current Architecture

`layout.h` is the public layout contract. It defines:

- `ColorPalette`
- font and color roles
- text, icon, and background substrata
- fixed watchface strata
- `WatchfaceSurface`
- the public layout APIs used by watchface and feature modules

`layout_calculate_surface()` currently fills the surface data:

- face dimensions
- background and rule geometry
- shared content metrics
- date and time text substrata
- BPM, steps, battery, and climate icon/text substrata
- palette pointer and resolved fonts

Feature modules create Pebble runtime objects from their own stratum data:

- `background` creates the background `Layer`
- `date` creates the date `TextLayer`
- `time` creates the time `TextLayer`
- `bpm` creates BPM text and icon layers
- `steps` creates steps text and icon layers
- `battery` creates battery text and icon layers
- `climate` creates temperature text and weather icon layers

`watchface` is the runtime clearing house. It owns:

- the live `WatchfaceSurface`
- module creation order
- module refresh order
- module destroy order
- service and AppMessage event routing

This ownership model is correct. The cleanup should preserve it.

## Problem

`layout.c` currently mixes several concerns:

- palette constants and display-mode palette selection
- compact/full font role resolution
- rectangular geometry calculation
- public text-layer update helper
- public icon coordinate scaling helpers
- shape dispatch for future round work

`watchface.c` is mostly doing the right clearing-house job, but it is noisy
because it is the one place where all modules are created, refreshed,
destroyed, and routed. That explicitness is useful and should not be hidden
behind a generic registry unless there is a stronger reason later.

## Target Shape

Keep `layout.h` as the only public layout header.

Split layout implementation into private layout files:

- `layout.c`
  - public facade for the layout module
  - owns `layout_calculate_surface()`
  - owns `layout_update_surface_style()`
  - owns public text-layer and icon-scaling helpers unless a later audit
    proves they belong elsewhere
  - includes private layout implementation headers

- `layout_stylist.c/.h`
  - private to the layout module
  - owns `ColorPalette` constants
  - owns display-mode palette selection
  - owns compact/full classification
  - owns font-key mapping for `WatchfaceFontRole`
  - fills `WatchfaceSurfaceStyle`

- `layout_rect.c/.h`
  - private to the layout module
  - compiled for rectangular geometry
  - owns rectangular coordinate calculation
  - fills every rectangular stratum and substratum frame
  - fills rectangular background/rule geometry

- `layout_round.c/.h`
  - future private layout architect
  - not added until round geometry is deliberately planned
  - must not be introduced as speculative support in this cleanup slice

Only `layout.c` should include private layout headers. Feature modules and
`watchface.c` should continue to include only `layout.h`.

## Strata And Substrata Rule

Layout creates calculated descriptions, not Pebble runtime layers.

The architects fill fields inside `WatchfaceSurface`:

- `surface->background`
- `surface->date.text`
- `surface->time.text`
- `surface->bpm.icon`
- `surface->bpm.text`
- `surface->steps.icon`
- `surface->steps.text`
- `surface->battery.icon`
- `surface->battery.text`
- `surface->climate.icon`
- `surface->climate.text`

Feature modules remain responsible for calling:

- `text_layer_create()`
- `layer_create()`
- `layer_set_update_proc()`
- `layer_add_child()`
- matching destroy functions

Do not move Pebble layer creation into layout.

## Public API Rules

- Public APIs stay declared in `layout.h` and implemented as normal external
  functions.
- Private layout helper APIs may be declared in private headers, but those
  headers must only be included by layout implementation files.
- Do not make public APIs `static` or `inline`.
- File-local private helpers may remain `static`.
- Do not pass or return large watchface structs by value.
- Continue filling caller-owned `WatchfaceSurface*` storage.

## Watchface Boundary

`watchface` should remain explicit:

```c
layout_calculate_surface(..., &s_surface);
background_module_create(root, &s_surface);
date_module_create(root, &s_surface);
time_module_create(root, &s_surface);
battery_module_create(root, &s_surface);
climate_module_create(root, &s_surface, temp_unit);
bpm_module_create(root, &s_surface);
steps_module_create(root, &s_surface);
```

Do not replace this with a generic table-driven module registry in this
cleanup. The explicit order is readable, reviewable, and important for
partial-create destroy behavior.

Potential later cleanup, separate from the layout split:

- audit whether the extra unconditional `background_module_destroy()` at the
  end of `watchface_destroy()` is still needed
- audit repeated `watchface_update_style()` calls for event paths

These are not part of the first layout split unless explicitly approved.

## Implementation Sequence

1. Audit current symbols and call sites.
   Confirm every public layout function, every `layout.h` include, and every
   module call that depends on `WatchfaceSurface`.

2. Extract style only.
   Move palette constants, compact/full classification, font key mapping, and
   style fill behavior into `layout_stylist.c/.h`.

3. Build and diff-review.
   Confirm no geometry, module creation, AppMessage, or visual behavior
   changed.

4. Extract rectangular architecture.
   Move rectangular coordinate calculation and stratum/substratum frame
   assignment into `layout_rect.c/.h`.

5. Build and screenshot.
   Build all current targets. For visual confidence, screenshot Emery and one
   compact rectangular target if the emulator is available.

6. Stop for review.
   Do not add `layout_round.c/.h` in the same slice unless explicitly
   requested after rectangular extraction is validated.

## Non-Goals

- Do not enable Chalk or Gabbro.
- Do not introduce round geometry yet.
- Do not move Pebble layer creation into layout.
- Do not create a generic dynamic stratum array.
- Do not rename feature modules.
- Do not change AppMessage keys.
- Do not change font choices, palette choices, or visible geometry.
- Do not refactor glyph rendering as part of this layout split.

## Validation

Required before commit:

- `git diff --check`
- `pebble build`
- review staged diff for intended files only

Visual validation when geometry code moves:

- install/screenshot Emery
- install/screenshot one compact rectangular target when available
- compare date, time, rule, health row, bottom row, and icon alignment against
  the current working baseline

Unrelated files, including local plan copies or screenshots, must remain
unstaged unless explicitly requested.
