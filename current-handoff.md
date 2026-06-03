# Current Handoff

## Project State

- Workspace: `/Users/manomeht/Code/life-at-a-glance-emery-wf`
- Product: Pebble SDK watchface, "Life at a Glance".
- Date: June 3, 2026.
- Current target platforms in `package.json`: `aplite`, `basalt`,
  `diorite`, `emery`, and `flint`.
- Latest commits:
  - `f82682c` `Tune secondary metric typography`
  - `db7de61` `Expand weather condition glyph rendering`
  - `043cca4` `Add weather condition icon support`
  - `c34396c` `Add Flint support through Rebble Clay`
  - `6e2b9c0` `Tighten embedded geometry helper boundaries`
- Last known build status: `pebble build` passed after the weather glyph
  and typography slices. Linker RWX segment warnings are known non-fatal
  Pebble toolchain noise.
- Working tree at handoff: `wf-punchlist.md` has a pending docs-only
  update. Do not assume it is ready to commit without inspecting it.

## Collaboration Directives

This repo has produced the best results when the agent slows down and thinks
like an architect before editing. The recurring failure mode is jumping into
code too early, creating broad helpers, or making layout decisions without
auditing their visual and platform ramifications.

Use this cycle for substantive work:

1. Inspect the live code, package metadata, build rules, generated resources,
   and dirty tree.
2. Identify every affected state variable, layer, update proc, AppMessage key,
   persisted field, and platform guard.
3. Audit call-sites and declaration-definition pairs before proposing edits.
4. Confirm product/layout decisions with the user when they affect placement,
   alignment, hierarchy, or visible behavior.
5. Execute the smallest coherent slice.
6. Review the diff as if doing a code review, then build and commit the slice.

Be self-critical:

- Use a review mindset before and after editing. Findings should be grounded
  in files, functions, and behavior, not vibes.
- Do not create helpers just to avoid basic arithmetic. Extract helpers only
  when they clarify a real boundary, reduce meaningful duplication, or match
  an established module seam.
- Embedded C constraints matter. Avoid large struct pass-by-value when scalar
  fields or caller-owned pointer outputs are practical.
- Rendering callbacks must stay lightweight. Avoid allocation, I/O, parsing,
  or complicated formatting inside update procs.
- Keep code slices small: audit, patch, build, commit, then move on.

## Current Runtime Model

Settings and AppMessage keys:

- `TIME_FORMAT`: `0` 24-hour, `1` 12-hour
- `TEMP_UNIT`: `0` Fahrenheit, `1` Celsius
- `TEMPERATURE`: Celsius tenths from PebbleKit JS
- `WEATHER_CONDITION`: raw Open-Meteo `weather_code`
- `HR_SAMPLE_MINUTES`: enum-backed sampling interval
- `ICON_FALLBACK_MODE`: hidden, `0` disabled, `1` enabled
- `DISPLAY_MODE`: `0` dark, `1` light

Current PebbleKit JS weather behavior:

- `src/pkjs/index.js` fetches `temperature_2m` and `weather_code` from
  Open-Meteo.
- JS sends the raw numeric weather code to C. Do not convert it to strings.
- If geolocation fails, JS currently falls back to fixed coordinates in
  `DEFAULT_LAT` and `DEFAULT_LON`. This needs a deliberate follow-up.

## Current Layout Snapshot

Rectangular layout is calculated in `calculate_watchface_layout()`.

For Emery (`200x228`):

- spacing: `PBL_DISPLAY_HEIGHT / 28`, currently `8`
- date: `GRect(8, 8, 184, 20)`, `FONT_KEY_GOTHIC_18_BOLD`
- time: `GRect(8, 58, 184, 48)`, `FONT_KEY_BITHAM_42_BOLD`
- rule: line from `(8, 114)` to `(192, 114)`, 1px stroke
- BPM icon: `GRect(8, 118, 28, 28)`
- BPM text: `GRect(38, 122, 28, 20)`, `FONT_KEY_GOTHIC_18`
- steps icon: `GRect(122, 118, 28, 28)`
- steps text: `GRect(152, 122, 40, 20)`, `FONT_KEY_GOTHIC_18`
- weather icon: `GRect(8, 196, 28, 28)`
- temperature text: `GRect(38, 200, 40, 20)`, `FONT_KEY_GOTHIC_18`
- battery icon: `GRect(128, 196, 28, 28)`
- battery text: `GRect(158, 200, 34, 20)`,
  `FONT_KEY_GOTHIC_18_BOLD`

Text alignment is intentionally left-aligned across all columns.

## Palette And Unavailable State

Palette definitions live in `src/c/main.c` as `VisualPalette`.

Unavailable text token: `---`.

Color displays:

- available text background is `GColorClear`
- unavailable text background is `GColorClear`
- unavailable text is mode-specific
- weather, BPM, and steps icons use unavailable icon backgrounds only when
  their paired text value is unavailable

Black-and-white displays:

- unavailable backgrounds invert against the current display mode so hidden
  state remains readable without color.
- color-specific semantic choices fall back to primary text.

Important symmetry rule:

- If an unavailable text value gets an unavailable background, its icon layer
  should get the matching unavailable background unless there is an explicit
  product decision otherwise.

## Weather Module State

Weather icon rendering now lives in `src/modules/weather.c`.

Current model:

- JS sends raw Open-Meteo weather codes.
- C maps codes into private `WeatherIconKind` buckets.
- Glyphs are procedural and lightweight. No Carbon/IcoMoon font and no PDC
  weather asset set yet.
- Weather glyph wrappers are file-local `static inline` functions.
- Snow grains are folded into snow.
- Freezing drizzle and freezing rain share the frozen-rain glyph.
- Unknown maps to a question-mark glyph.

Do not move weather drawing back into `main.c`.

## Recent Lessons

- Screenshots and emulator automation have been flaky. Build validation is
  still required; screenshots are optional when the user says to skip them.
- The user has repeatedly corrected unplanned layout changes. Treat x/y
  placement, alignment, font hierarchy, and row density as product decisions.
- `PBL_IF_COLOR_ELSE()` keeps monochrome/color palette choices cleaner than
  open-coded `#if defined(PBL_COLOR)` in many value assignments.
- Large by-value structs in embedded C are a non-negotiable concern for this
  project.
- Commit messages for non-trivial code should have a precise header and a
  bulleted body naming key functions and behavior changes.

## Pending Punchlist

Immediate planning topics:

1. Round-watchface layout plan.
   - Research Pebble/Rebble round-display guidance before coding.
   - Account for row-specific clipping on circular screens.
   - Do not assume the rectangular layout can simply scale.

2. Main-module architecture plan.
   - Review `main.c` for low-dependency extraction seams.
   - Likely modules: settings, layout, palette/display, health, battery,
     messaging, and weather.
   - Extract only when the boundary is real and behavior-preserving.

3. State hygiene plan.
   - Split state into explicit structs only where it improves clarity.
   - Avoid a giant app-state struct if it only hides globals without reducing
     coupling.

4. Persisted settings tolerance.
   - Current `settings_load()` reads the whole struct directly.
   - Investigate `persist_get_size()` and versioned migration patterns.
   - Take cues from TimeStyle and Carbon, but keep the first patch small.

5. Phone weather fallback configurability.
   - Current fixed-coordinate fallback is a product/privacy decision.
   - Consider localStorage cache-first fallback before adding Clay settings.

## Validation Checklist For Next Agent

Before any commit:

1. Run `git status --short`.
2. Inspect diffs, including docs and generated files.
3. Run `pebble build` for code changes.
4. Confirm no untracked screenshots, build artifacts, or private data are
   staged.
5. Use a detailed commit body for non-trivial changes.
