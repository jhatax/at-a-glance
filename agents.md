# Agent Guidelines: Life at a Glance Watchface

**Inherits from:**

- [Universal development principles](../AGENTS.md)
  (`~/Code/AGENTS.md`)
- [Pebble platform guidelines](../agent-templates/AGENTS-PEBBLE.md)
  (`~/Code/agent-templates/AGENTS-PEBBLE.md`)

Read the inherited files above before applying this project file. This file is
only for Life at a Glance facts, durable repo guardrails, product decisions,
and validation expectations. Do not duplicate broad operating rules or generic
Pebble guidance here; put those in the parent files.

`ARCHITECTURE_LEDGER.md` is the current source of truth for architecture
decisions and invariants. Start there for non-trivial code work, reconcile the
requested change against it, and update it when a decision changes.

---

## Project Facts

- Native C entrypoint: `src/c`
- Watchface modules: `src/modules`
- PebbleKit JS and Clay config: `src/pkjs`
- Manifest, target platforms, capabilities, resources, and message keys:
  `package.json`
- Build entry: `wscript`
- Design reference: `DESIGN.md`
- Architecture contract: `ARCHITECTURE_LEDGER.md`
- Round plan and handoff: `RoundWatchfacePlan.md` and
  `RoundLayoutHandoff.md`

Pebble manifest capabilities currently required:

- `configurable` for the Clay settings page
- `location` for weather geolocation
- `health` for BPM and steps

AppMessage keys currently required:

- `TIME_FORMAT`
- `TEMP_UNIT`
- `TEMPERATURE`
- `WEATHER_CONDITION`
- `HR_SAMPLE_MINUTES`
- `DISPLAY_MODE`

Persisted settings defaults:

- time format: `24h`
- temperature unit: `°F`
- HR sampling: `10` minutes
- display mode: dark

Clay configuration UI:

```text
At A Glance: Configuration
  - Time format: 24-hour, 12-hour
  - Temperature unit: Fahrenheit, Celsius
  - Display mode: Dark mode, Light mode
  - HR Sampling Frequency: 10, 15, 30, 60, 120 minutes
  - Submit: Save Settings
```

---

## Durable Guardrails

- Inspect the live tree before editing. Treat old docs, memory, and current
  diffs as evidence to verify, not as current truth.
- Keep changes in one coherent slice. Do not mix architecture cleanup,
  visual redesign, AppMessage changes, platform enablement, and glyph tuning
  unless the user explicitly approves that larger slice.
- Preserve user work in the dirty tree. Do not revert unrelated changes.
- Reuse existing sentinels, enum names, macros, message keys, and product
  vocabulary. Do not introduce adjacent names for an existing concept.
- Prefer existing module-owned contracts over new variables or helper
  functions. Add a helper only when it clarifies a real boundary, removes
  meaningful repeated complexity, or is required by a Pebble callback.
- Keep straightforward one-line or short calculation logic inline when doing
  so makes the code easier to audit.
- Do not pass or return large watchface/layout structs by value. Use scalar
  values or pointers to caller-owned storage.
- Keep programming source lines at or below 80 characters where practical.
- If a fix reveals a broken pattern, audit sibling modules and analogous call
  sites before declaring the slice complete.
- Do not commit API changes, new APIs, or changed callers until the old and
  new flows have been reviewed and approved. A clean build plus post-hoc diff
  review is not sufficient for API or lifecycle changes.
- Before implementing platform layout work, name the design authority and
  challenge foundational assumptions. In particular, distinguish reusing
  architecture from reusing geometry, constants, or scaling policy.
- Maintain `RAID_LOG.md` for non-trivial layout, platform, lifecycle, or API
  work. Record risks, assumptions, active issues, and decisions before they
  become invisible momentum.

Required API-change workflow:

1. Discuss the plan and spec before editing.
2. Agree on invariants and behavior that must not change.
3. Write code using the deliberate protocol, within guardrails, and with a
   narrow scope.
4. Create a clean flow diagram showing prior flow(s), new flow(s), and the
   supporting commit message.
5. Get explicit approval of the revised flows and commit message.
6. Commit only after approval.
7. Update the architecture ledger for the accepted decision.

---

## Architecture Orientation

The current watchface is organized around a prepared `WatchfaceSurface`:

```text
main.c
  -> watchface_create(window, settings)
  -> layout_watchface_initialize(width, height, &surface)
  -> layout_update_watchface_style(&surface.style, display_mode)
  -> feature_module_create(root, &surface)
  -> watchface_refresh_strata(WATCHFACE_UPDATE_ALL_STRATA)
```

`layout_watchface_initialize()` is the public watchface layout initialization
API. The active shape implementation clears the caller-owned surface and
calculates geometry. `layout_update_watchface_style()` applies display-mode
style separately so repaint can cascade style without reinitializing geometry.

Core invariants:

- `watchface` owns the live `WatchfaceSurface` storage and lifetime.
- Layout initialization clears and prepares the caller-owned surface from
  scratch inside the active shape implementation.
- There is one public layout initialization contract. Rectangular and round
  layout implementations provide `layout_watchface_initialize()` behind
  `PBL_RECT` and `PBL_ROUND`.
- The active architect owns geometry and compact/full classification.
- Compact/full is resolved once by the architect and stored on
  `WatchfaceSurfaceStyle.is_compact`.
- The stylist consumes `WatchfaceSurfaceStyle`; it does not re-derive
  compact/full from dimensions.
- Architects use private immutable blueprints and private calculated metrics.
  They must not expose layout-private metrics through `WatchfaceSurface`.
- Blueprints are constants/product choices. Calculated layout is the resolved
  geometry used to lay strata on the surface.
- Feature modules own Pebble layer lifecycle. Shared render helpers may create
  or update layers from calculated substrata, but feature modules remain the
  owners.
- Module `create(root, surface)` may retain the calculated surface once.
  Module `refresh()` APIs must not accept `WatchfaceSurface`; they may accept
  only narrow runtime payloads such as time format or temperature unit.

---

## Module Boundaries

- `src/c/main.c`: Pebble window lifecycle, service subscriptions, settings,
  AppMessage parsing, and watchface dispatch.
- `src/modules/watchface.c`: active watchface runtime, root layer discovery,
  `WatchfaceSurface` ownership, module create/destroy order,
  `s_strata_created_mask`, full-display refresh coordination, and event
  routing.
- `src/modules/watchface_components.h`: shared display component model:
  palette, font/color roles, surface, strata, and substrata types.
- `src/modules/layout.h`: public layout initialization and style-update API.
- `src/modules/layout_rect.c`: rectangular architect and rectangular geometry.
- `src/modules/layout_round.c`: round architect and round geometry.
- `src/modules/layout_stylist.c`: style helper for palette,
  font-role, custom-font, and compact/full style consumption.
- `src/modules/substratum_renderer.c/.h`: shared Pebble rendering helper for
  calculated substrata: text-layer setup/update, icon-coordinate scaling,
  and shared glyph primitives. Keep repeated frame-aware primitive math here;
  keep glyph-specific composition helpers such as `weather_subframe()`
  private to the owning module.
- `src/modules/settings.c`: persisted settings defaults, validation, load,
  save, and HR sampling interval mapping.
- `src/modules/helper.c/.h`: shared arithmetic, color, and parsing helpers.
- `src/modules/date.c`: date text layer, date buffer, uppercase date
  formatting, and date refresh.
- `src/modules/time.c`: time text layer, time buffer, time-format rendering,
  custom font lifecycle, and time refresh.
- `src/modules/climate.c`: weather source state, temperature formatting,
  temperature text layer, weather availability, and weather icon layer
  ownership.
- `src/modules/climate_glyphs.c`: raw Open-Meteo weather-code mapping and
  procedural weather glyph rendering from explicit condition, frame, and
  palette inputs.
- `src/modules/bpm.c`: BPM source state, text/icon layers, procedural BPM
  icon, colors, and heart-rate update handling.
- `src/modules/steps.c`: steps source state, text/icon layers, procedural
  steps icon, colors, and movement update handling.
- `src/modules/battery.c`: battery source state, text/icon layers, procedural
  battery drawing, colors, and battery callback updates.
- `src/modules/background.c`: background layer, palette background, and
  horizon rule rendering.

Do not move behavior across these boundaries unless the new boundary is
cleaner, behavior-preserving, and reflected in the ledger.

---

## State And Rendering

- Store source state, not redundant derived render state, unless the derived
  value is expensive or impossible to recompute.
- Setters mutate source state only. Rendering happens through
  `watchface_refresh()`.
- `watchface_refresh(WatchfaceUpdateMask updates)` is the public render
  dispatcher. `main.c` knows update categories, not feature modules.
- AppMessage handling in `main.c` should accumulate a local
  `WatchfaceUpdateMask` and make one final coalesced `watchface_refresh()`
  call.
- Dynamic icon/text colors are module-owned when they depend on live source
  state, such as BPM or battery.
- Palette resolution should degrade to simple legible black-and-white choices
  instead of blocking the watchface.
- The background/rule is a module-owned stratum. The rule belongs in
  background substratum data, not ad hoc watchface drawing.

---

## Text-First Module Semantics

- Text is the primary glance surface. Icons are secondary visual support.
- For text-bearing modules, required text-layer creation determines module
  success.
- If a metric's text layer cannot be created, do not create or depend on that
  metric's icon.
- If an icon layer cannot be created, continue updating the corresponding
  text.
- Optional icon creation does not gate module success unless the module
  explicitly defines the icon as required.
- Log layer creation failures once where creation happens. Avoid repeated
  refresh-time log spam for known-missing layers.
- Assign a module's retained `s_surface` only after required layer creation
  succeeds.
- Failure paths must not leave stale surface pointers, custom fonts, layers,
  or partially-owned state behind.
- If a module loads a custom font or other resource during `create()`, the
  same module must release it both on later create failure and in `destroy()`.

---

## Product Constants And Layout

- Canonical product constants live with the shared component/product contract,
  currently `src/modules/watchface_components.h` for design inputs and
  `src/c/ataglance.h` for app-level flags/string limits.
- Do not mutate or copy immutable product constants into active mutable
  profiles.
- Do not change icon/text sizes beyond canonical constants or macros unless an
  approved design decision says so.
- Full rectangular displays use the canonical rectangular blueprint values.
- Compact rectangular displays may use the compact rectangular blueprint.
  Do not make every calculation scale just because a device differs from the
  design reference.
- Round layout is round-native. Do not scale rectangular coordinates
  wholesale and call it round support.
- Round geometry must account for circular clipping by vertical position,
  row-specific available widths, shape-specific margins, font extremes, icon
  legibility near curved edges, and screenshots on real round emulators.
- Chalk is the hard round target; Gabbro should reuse the same safe-span model
  as an expansion case.

---

## Visual Semantics

- Unavailable text token: `---`
- Dark mode: black background, white rule, Celeste primary text on color,
  Light Gray unavailable text on color, Electric Blue date, Sunset Orange
  time, and primary-text metric icons.
- Light mode: white background, Oxford Blue rule, Cobalt Blue primary text,
  Dark Gray unavailable text on color, black date, Sunset Orange time, and
  primary-text metric icons.
- Black-and-white displays fall back to legible black-and-white choices via
  `PBL_IF_COLOR_ELSE` and `gcolor_legible_over()`.
- BPM color zones:
  `<=0` or unavailable uses unavailable color; `1-99` uses primary text;
  `100-120` uses Chrome Yellow on dark and Windsor Tan on light; `>120` uses
  Orange on dark and Bulgarian Rose on light.
- Battery colors:
  charging uses Islamic Green on color and primary text on monochrome;
  not charging `>50` uses primary text; `21-50` uses Rajah on dark and
  Windsor Tan on light; `<=20` uses Red on dark and Bulgarian Rose on light.
- PebbleKit JS sends raw Open-Meteo `weather_code`; C maps codes into private
  weather glyph buckets in `src/modules/climate_glyphs.c`.
- PebbleKit JS uses phone geolocation when available and falls back to OAK at
  `37.85626, -122.21383`.

---

## Visual Quality

- Optimize for a one-second glance: time first, then supporting facts without
  searching.
- Preserve the Swiss-Rail influence quietly: strong alignment, restrained
  color, deliberate spacing, and no decorative clutter.
- Keep time visually dominant. Secondary metrics must not compete with it.
- Use stable frame sizes for icon layers and text layers so values such as
  `---`, `100`, `99999`, `90°F`, and `100%` do not shift layout.
- Preserve negative space around the rule and between rows. Do not fill every
  available pixel.
- Validate optical alignment, not only mathematical alignment. Small glyphs,
  icons, and baselines may need screenshot review.
- Avoid thin decorative strokes that disappear on monochrome or low-contrast
  screens.
- Avoid hidden assumptions from desktop previews. Use emulator captures for
  visual changes.

---

## Validation Checklist

Use the parent Pebble checklist plus these repo-specific checks:

- Current target platform set in `package.json` is intentional.
- AppMessage keys match across `package.json`, Clay, PebbleKit JS, C, docs,
  and manual test commands.
- Manual AppMessage numeric keys were checked after any message-key removal or
  reordering.
- Defaults match across C, Clay, README, docs, and persisted settings.
- Resource filenames and generated IDs match.
- `PBL_HEALTH` guardrails protect `bpm.h`, `steps.h`, and every `Health*`
  symbol.
- `main.c` includes `watchface.h`, not feature module headers.
- `watchface_components.h` stays type-focused and does not gain behavior.
- No feature-module `*_refresh()` declaration accepts `WatchfaceSurface`.
- Text does not clip at likely extremes: `WED 30 SEP`, `12:59`, `100`,
  `99999`, `---F`, and `100%`.
- Visual QA considered dark and light modes, color and black-and-white
  displays, available and unavailable health values, weather unavailable,
  long date, `99999` steps, and `100%` battery.
- `git diff --check` passed.
- `pebble build` ran for code changes, or the validation gap is reported.
- Emulator screenshots were used for visual/layout changes when practical.
- Ephemeral screenshots and temporary buffers were deleted after validation,
  or the user explicitly chose to keep them.

---

## Publishing Notes

This folder is not yet on GitHub. The next publishing task should include:

- build check
- secret audit
- `.gitignore`
- SDK/platform metadata
- README review
- screenshots if useful and approved
- clear initial commit

Do not publish generated build artifacts unless intentionally releasing them
and the user agrees.
