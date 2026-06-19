# Final Review Ledger

Objective outside-in review for Pebble App Store submission readiness.

Scope for this pass:

1. Layer 1: firmware safety.
2. Layer 2: Pebble platform correctness.
3. Layer 3: product and visual integrity.

Package-readiness review is intentionally deferred until code defects from
these layers have been resolved or explicitly accepted.

Validation snapshot:

- `pebble build` passed on June 17, 2026 for configured targets.
- Build success does not clear the submission-blocking findings below.

Severity:

- P0: Must fix before any release candidate.
- P1: Must fix or explicitly de-scope before App Store submission.
- P2: Should fix before App Store submission unless consciously accepted.
- P3: Cleanup, hardening, or maintainability debt.

---

## Layer 1: Firmware Safety

### FR-001: Heap allocation occurs inside draw/update path

- Status: Fixed.
- Resolution:
  - The filled bolt drawing path no longer performs heap allocation in the
    render/update path.
- Original severity: P1
- Evidence:
  - `src/modules/substratum_renderer.c`: `draw_filled_polygon_in_frame()`
    allocates `scaled_points` with `malloc()` and frees it after drawing.
  - This helper is used by `substratum_renderer_draw_filled_bolt_in_frame()`.
  - Battery bolt drawing reaches it from `battery_bolt_update_proc()`.
  - Weather thunderstorm drawing also reaches it from `draw_climate_icon()`.
- Risk:
  - Layer update procs are hot render callbacks. Allocating in them creates
    avoidable heap churn, failure-dependent drawing behavior, and fragmentation
    risk on memory-constrained Pebble targets.
- Decision:
  - No release gate remains for this finding.

### FR-002: Failure path can leave the app running after watchface create fails

- Status: Closed.
- Resolution:
  - Accepted as Pebble-recommended behavior for this lifecycle model. This has
    been reviewed and is not considered a release defect.
- Original severity: P2
- Evidence:
  - `main_window_load()` logs and returns when `watchface_create()` fails.
  - `watchface_create()` has internal failure paths that call
    `watchface_exit_path()`, but not every caller-level failure is treated as a
    terminal app initialization failure.
  - `init()` continues service/AppMessage setup after `window_stack_push()`.
- Risk:
  - A failed required stratum or layout/font creation can produce a blank app
    that still has services and AppMessage callbacks active. Current refresh
    guards make this unlikely to crash, but it is not a clean firmware failure
    model.
- Decision:
  - No code change required. Keep the existing Pebble lifecycle flow unless a
    separate concrete crash, leak, or teardown defect is found.

### FR-003: Helper macros are not fully expression-safe

- Severity: P2
- Evidence:
  - `HELPER_SCALE_ROUND(v, n, d)` and `HELPER_CLAMP_MIN(exp, min)` are not
    wrapped as full macro expressions.
  - `HELPER_SCALE_ROUND()` is used heavily in layout and glyph scaling.
  - `HELPER_CLAMP_MIN()` is currently unused.
- Risk:
  - The current call sites mostly use simple expressions, but these macros are
    public helpers. A future arithmetic, ternary, comparison, or assignment
    context can silently change meaning because the whole macro body is not
    parenthesized.
- Gate:
  - Fully parenthesize the macro bodies or replace them with small static inline
    functions where that is supported by the Pebble C build.

### FR-004: Shared text buffer size is still broad product leakage

- Status: Fixed.
- Resolution:
  - `ATAGLANCE_MAX_STR_LEN` and `src/c/ataglance.h` were removed. Date, time,
    climate, BPM, and steps now use module-local buffer constants sized to
    their own formatted values.
- Original severity: P3
- Evidence:
  - `ATAGLANCE_MAX_STR_LEN` lives in `src/c/ataglance.h`.
  - Date, time, climate, BPM, and steps modules use it for private static text
    buffers.
- Risk:
  - This is not an immediate overflow because current formatted values fit in
    16 bytes, but a global app-level buffer constant hides each module's real
    maximum string contract and keeps public header leakage alive.
- Gate:
  - Replace it with module-local constants sized to each module's formatted
    output.

---

## Layer 2: Pebble Platform Correctness

### FR-101: Round platforms are advertised without a round-native architect

- Status: Closed.
- Resolution:
  - The product direction is a singular native layout architect. Shape-specific
    geometry decisions belong inside that architect rather than separate
    shape-owned implementation files.
- Original severity: P1
- Evidence:
  - `package.json` lists `chalk` and `gabbro`.
  - `layout_architect.c` uses rectangular blueprint names and a rectangular
    flow for all targets.
  - `design.h` has only a margin switch under `PBL_ROUND`; it does not define a
    round-safe layout policy.
- Risk:
  - The package can build for round watches while shipping geometry that was not
    designed against circular clipping, row-specific safe width, or Chalk/Gabbro
    visual constraints. This is a platform correctness issue, not just polish.
- Decision:
  - No release gate remains for having a singular architect. Round publication
    confidence still depends on screenshot-led visual validation.

### FR-102: C default display mode disagrees with Clay and product docs

- Status: Fixed.
- Resolution:
  - Clay and current docs now make light mode the default, matching
    `DISPLAY_MODE_DEFAULT`.
- Original severity: P1
- Original evidence:
  - `DISPLAY_MODE_DEFAULT = DISPLAY_MODE_LIGHT` in `src/modules/settings.h`.
  - Clay previously defaulted `DISPLAY_MODE` to the dark-mode value.
  - Project instructions previously documented the persisted display-mode
    default as dark.
- Original risk:
  - First-run behavior before Clay sends settings can start in light mode even
    though the product default and configuration default are dark. This is a
    settings contract mismatch.
- Decision:
  - No release gate remains for the display-mode default mismatch.

### FR-103: Health setting is exposed even where health strata are absent

- Status: Closed.
- Resolution:
  - The Clay configuration screen now states that health update settings apply
    only on Pebble models with health capabilities. The broad target platform
    set and guarded C health paths remain intentional.
- Original severity: P2
- Evidence:
  - `package.json` includes `aplite` in target platforms.
  - Health modules and `Health*` usage are correctly guarded by `PBL_HEALTH`.
  - Clay always exposes `HR_SAMPLE_MINUTES`.
- Risk:
  - On platforms/builds without `PBL_HEALTH`, the configuration page can present
    a heart-rate sampling control that has no visible effect. The C path is
    guarded, but the product/platform contract is confusing.
- Gate:
  - Either accept this as harmless cross-platform UI debt, remove unsupported
    targets, or make the config/platform story explicit before submission.

### FR-104: AppMessage weather values are range-sanitized, but stale partial
weather remains possible

- Severity: P2
- Evidence:
  - `ataglance.c` applies each weather tuple independently.
  - `climate.c` treats weather as available only when temperature is available
    and condition is known.
  - If a partial message updates temperature but omits condition or `IS_DAY`,
    previous condition/day state can remain until the next complete weather
    update.
- Risk:
  - Normal JS sends all weather tuples together, so this is not expected during
    healthy operation. AppMessage drops, manual sends, or future send paths can
    create mixed-source weather display state.
- Gate:
  - Either document that weather AppMessages must be complete, or add an atomic
    weather setter/update path if partial weather sends are expected.

---

## Layer 3: Product And Visual Integrity

### FR-201: Round visual product promise is not yet supportable

- Status: Closed.
- Resolution:
  - The singular layout architect is the accepted product architecture for
    shape-specific geometry decisions.
- Original severity: P1
- Evidence:
  - Product docs define round-specific design needs, including safe width,
    curve clipping, and Chalk/Gabbro screenshot validation.
  - The live architect does not provide round-native geometry.
  - The package still targets round platforms.
- Risk:
  - The watchface can be submitted as round-capable while failing the visual DNA
    and legibility expectations that define the product.
- Decision:
  - No release gate remains for separate round-architect absence. Keep
    Chalk/Gabbro screenshot review as product validation before publication.

### FR-202: Text width constants are per-field product decisions

- Status: Closed.
- Resolution:
  - The former uniform text-width invariant has been removed. Text widths and
    heights are now documented as per-field layout decisions dictated by font
    selection, role, row ownership, and available geometry.
- Original severity: P2
- Historical evidence:
  - Full climate and steps widths are `46`.
  - Full BPM width is `40`.
  - Compact climate, steps, and BPM widths are `39`.
- Decision:
  - These differing constants are intentional unless a specific field's font,
    value range, or geometry changes.
  - No release gate remains for this finding.

### FR-203: Weather unavailable slash color is inconsistent with unavailable
weather glyph color

- Status: Closed.
- Resolution:
  - The primary-text slash is intentional for contrast over the unavailable
    weather glyph.
- Original severity: P2
- Evidence:
  - `WEATHER_ICON_UNKNOWN` maps to `palette->unavailable_text`.
  - `draw_weather_unavailable_icon()` draws the slash using
    `palette->primary_text`.
- Risk:
  - The unavailable glyph mixes unavailable and primary semantics in one icon.
    That weakens the current visual rule that unavailable state should use a
    shared calm unavailable vocabulary.
- Decision:
  - No release gate remains for this finding.

### FR-204: Battery and weather bolt share a renderer path, but not necessarily
the same visual contract

- Status: Closed.
- Resolution:
  - This is an accepted product decision. The shared bolt is intentionally a
    low-level glyph primitive reused by battery charging and thunderstorm
    weather, not a product-level semantic component.
- Original severity: P3
- Evidence:
  - `substratum_renderer_draw_filled_bolt_in_frame()` is used for both charging
    battery and thunderstorm weather.
  - The shared helper draws a filled bolt from the same design points.
- Risk:
  - The shape reuse is economical, but charging and thunderstorm are unrelated
    semantics. Any future tweak for one can leak into the other unless the
    shared primitive is treated as a low-level glyph, not a product-level icon.
- Gate:
  - Keep the primitive low-level and avoid adding product-specific behavior to
    the shared helper.

### FR-205: Tracked prototype/scratch artifact remains in source tree

- Status: Fixed.
- Resolution:
  - `src/scratch.txt` has been deleted.
- Original severity: P3
- Evidence:
  - `src/scratch.txt` is tracked.
  - The architecture ledger already calls out tracked prototype artifacts for
    publication review.
- Risk:
  - Not a runtime defect, but it is inconsistent with a polished submission
    source package and increases review noise.
- Gate:
  - Delete it before package-readiness review unless it is intentionally kept as
    source documentation.

---

## Checks With No Blocking Finding In This Pass

- Required layer create/destroy paths are mostly symmetrical.
- Required text layers gate module success as intended.
- Optional icon failures generally keep text modules alive.
- AppMessage key names align across `package.json`, C, Clay, and JS in this
  pass.
- Health API references found in this pass are behind `PBL_HEALTH`.
- `pebble build` passes for the configured target set.

---

## Deferred Package-Readiness Review

Run after P1/P2 decisions above:

1. Confirm target platform set.
2. Confirm App Store metadata, screenshots, menu icon, generated PBW, and
   release notes.
3. Audit README/docs for promises that exceed live behavior.
4. Remove scratch/prototype files and temporary artifacts.
5. Run visual QA across dark/light, color/BW, health available/unavailable,
   weather unavailable, long date, `99999` steps, `100%` battery, and round
   platforms if they remain targeted.
