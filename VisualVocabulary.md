# Visual Vocabulary

This document defines the visual direction for At A Glance. It covers
the shared watchface composition, color vocabulary, and procedural glyph
direction.

The goal is not decorative detail. The goal is glanceable, compact, confident
glyphs that work across monochrome and color modes, including the current
Black on White, White on Black, Clear as Celeste, and Duke Blue Moon
display modes.

## Principles

- Design for one-second recognition.
- Keep the product hierarchy stable across display families: top heart-rate
  context, dominant time, battery track, weather/date context, and bottom
  steps context.
- Treat text as the primary glance surface. Icons support recognition,
  but they do not carry the product alone.
- Give each visual channel one job:
  text provides exact values, icons identify the metric, lines show progress,
  and color indicates state.
- Prefer solid filled silhouettes over hollow outlines.
- Use line art only when the line itself is the message.
- When using lines, use heavy strokes that survive watch scale.
- Make the primary metaphor large. Drop secondary accents if they force the
  primary shape to shrink.
- Use color as a state cue, not as the only source of meaning.
- Every glyph must work in black and white.
- Prefer fewer, stronger marks over dots, texture, wisps, and small accents.
- Keep glyphs stable inside a fixed 28x28 frame.
- Keep value/state changes from shifting the visual footprint.
- Create review variants in `glyph-lab` before porting into production code.
- A text-only metric is acceptable when an icon cannot be created or
  does not fit the platform. An icon-only metric is not acceptable.

## Text Metrics

Text widths and heights are per-field layout decisions. There is no uniform
text-width invariant.

Rules:

- The font role, selected font, and platform geometry dictate each text
  frame.
- Time, date, climate, BPM, and steps may use different widths because they
  carry different content and occupy different row contracts.
- Text heights follow the selected font role and the row's visual alignment
  needs.
- Width changes are product/layout changes. Do not change them during cleanup.
- A field's text frame should be large enough for its expected extreme value
  without making unrelated fields inherit that width.

## Surface Composition

The current selected watchface composition is:

```text
top heart-rate context
dominant centered time
centered battery track and charging bolt
weather/date context
bottom steps context
```

This shared stack and the dominant orange time text are the visual DNA between
rectangular and round displays. The implementation may calculate different
geometry by shape, but the user should recognize the same product immediately.

Rules:

- Time remains the dominant visual object.
- The battery track sits directly under time and uses a primary-color outline
  with a state-colored fill.
- Weather/date share one context row. Weather owns the temperature and icon
  group; date is aligned in the remaining space.
- BPM owns the top context row and steps own the bottom context row on health
  platforms.
- Health metrics are centered relative to the display centerline, not pushed
  into the far corners.
- The layout should preserve negative space instead of filling every pixel.
- Round displays require screenshot review because a shared stack does not
  guarantee safe chord width at each vertical position.

## Implementation Rules

Use a 28x28 design grid for procedural glyphs unless the production frame
changes. Keep the frame fixed even when states change.

For same-color layered filled shapes:

1. Draw the rear shape in glyph color.
2. Draw the foreground shape as a background-color halo.
3. Draw the foreground shape in glyph color.

The halo is functional. It creates separation on black-and-white displays.

For line-based marks:

- Use fixed pixel widths when the mark is a signature indicator.
- Use 3px for primary weather strokes unless a specific glyph needs a
  thinner exception.
- Keep stroke/gap choices explicit and review them on `aplite` and `emery`.

Layer creation follows the same hierarchy:

- Text layers are must-initialize controls.
- Icon layers are optional support.
- If a text layer fails, do not create or depend on that metric's icon.
- If an icon layer fails or is intentionally omitted for a smaller
  platform, continue updating the text.

## Display Modes And Palette Fallback

Display mode is a product decision, not a post-process tint. The current live
families are:

- `Black on White`
- `White on Black`
- `Clear as Celeste`
- `Duke Blue Moon`

On color-capable watches, all four modes are available. The first two are
monochrome-style presentations rendered on color hardware; the latter two are
the color-background palettes.

Base text, date, time, and unavailable colors come from the selected display
mode. Module-owned colors then layer on top for battery, BPM, steps, and
climate state.

If palette resolution ever fails, the watchface degrades to a simple
black-and-white palette instead of blocking data display.

Fallback rules:

- dark mode fallback: black background, white text, white rule, white
  icons
- light mode fallback: white background, black text, black rule, black
  icons
- no accent colors are required for the fallback path
- unavailable text and icons use the same foreground color as other data
  in fallback mode

This is a product decision: showing legible data in black and white is
more important than preserving the richer accent palette.

## Unavailable Data

Unavailable data uses the accepted absence slash:

```text
\
```

This replaces the earlier pause mark because pause implies data existed and was
stopped. The slash reads as absent or unavailable.

Rules:

- Use a bold `\` slash.
- Start slightly above the cancelled glyph.
- End slightly below the cancelled glyph.
- Use the unavailable-state foreground color unless a higher-contrast fallback
  is required on the target display.
- Do not use question marks.
- Do not use exclamation points for unavailable data.
- Keep pause bars as rejected history, not as an active alternative.

Meaning:

- waveform + slash: BPM unavailable
- footprint + slash: steps unavailable
- cloud + slash: weather unavailable or no usable weather data
- future metric + slash: data not currently available

The mark means absent, unavailable, or not currently usable. It should be
visible without feeling alarming.

## Steps

Use a station-style footprint that reads as familiar and obvious at watch
scale, not paws, detailed sandals, or shoes.

Direction:

- One large filled ball-of-foot circle.
- One background-color separator cut.
- One smaller filled heel circle.
- Inspired by Swiss and Japanese station wayfinding.
- No toe dots.
- No shoe details.

States:

- Available: one steps color.
- Unavailable: muted footprint plus the shared slash mark.

Do not encode step count progress with icon color or shape. The text value
already carries the count.

## Weather

Weather uses a hybrid vocabulary:

- Black-and-white: bold line/outline first.
- Color: fills and accents may be added when they improve recognition.
- Same condition should keep the same metaphor across both capabilities.
- BnW readability is the constraint that decides the geometry.
- The goal is familiar and obvious weather identification, not decorative sky
  illustration.

For weather, outline does not mean delicate. It means heavy sign strokes.

General rules:

- Use bold strokes for monochrome weather icons.
- Use fills or color accents only as secondary treatment on color displays.
- Keep condition marks sparse and strong.
- Prefer condition-first icons over cloud-first icons.
- Cloud appears only when it helps the condition read faster.
- Drop the cloud when it competes with rain, heavy rain, snowy-rain, fog,
  lightning, or other primary condition marks.
- Keep precipitation separate from the cloud baseline.
- Do not let rain or snow marks read as legs.
- Reduce the cloud for variants that need space below.
- Use 3px primary weather strokes to reduce light-mode monochrome
  irradiation and halation.

### Cloud Primitive

Standalone cloudy uses the full cloud primitive.

The cloud primitive:

- Bold outline in black-and-white.
- Filled or partially filled cloud may be used on color displays.
- Use one cloud unless review proves a second cloud improves recognition.
- Lower baseline is flat enough to feel sign-like.
- Use fewer, more deliberate lobes.
- Keep total mass compact.

Filled color draw order, if same-color layered clouds are restored later:

1. Draw rear cloud in cloud color.
2. Draw front cloud halo in background color, 2px larger than final front
   cloud.
3. Draw front cloud in cloud color.

This draw order is required only for filled same-color layering. Monochrome
clouds should prefer bold outline geometry instead of filled layering.

### Clear Sky

Clear sky uses a simple disk, not a sun, moon, horizon, or text.

Rules:

- Filled center circle.
- 1px outer ring.
- Center at `(14, 14)` on the 28px grid.
- Filled radius: 9px.
- Ring radius: 11px.
- Draw the outer ring first, then draw the filled center.
- In monochrome light mode, use a dark-gray/dithered outer ring and black
  fill.
- In monochrome dark mode, invert the treatment: use a light-gray/dithered
  outer ring and white fill.
- In color light mode, keep the same structure: black fill with a dark-gray
  outer ring.
- In color dark mode, keep the same structure: white fill with a light-gray
  outer ring.
- In color mode, use the same pattern as black-and-white: subtle outer
  ring plus stronger filled center. Color may tint those two parts, but it
  must not collapse into a single flat disk.
- No rays.
- No celestial-body detail.
- If the 1px ring disappears on a monochrome target, the filled disk still
  carries the clear-sky meaning.

### Weather Variants

For rain, snow, sleet, storm, and similar variants:

- Start by drawing the condition without a cloud.
- Add a cloud only if it improves recognition.
- Rain, heavy rain, and snowy-rain may drop the cloud entirely.
- If a cloud is used, use it at about 70% of standalone cloud size.
- Move the reduced cloud upward or to the side to preserve condition space.
- Keep condition marks below or beside the cloud.
- Do not allow condition marks to touch the cloud baseline unless intentional.
- Prefer fewer, heavier strokes over multiple small drops or flakes.

### Partly Cloudy

Partly cloudy uses a 70% cloud placed bottom-right in the cell.

Weather mapping:

- Open-Meteo weather code `2` should map to partly cloudy.
- Production maps code `0` and code `1` to clear sky, code `2` to partly
  cloudy, and code `3` to cloudy.

Rules:

- Cloud remains dominant.
- Sun is subordinate and sits behind the cloud.
- Sun needs three or four visible rays in monochrome.
- If the sun forces the cloud to become too small, show only the cloud.
- Open-Meteo code `0` and code `1` use clear sky.
- Open-Meteo code `2` uses partly cloudy.
- Open-Meteo code `3` uses cloudy.

### Fog

Fog may omit the cloud.

Preferred direction:

- Use two or three strong horizontal bars.
- Keep the bars heavy enough to survive black-and-white display.
- Add the cloud only if it does not reduce fog readability.

### No Weather

No usable weather data uses the weather primitive plus a weather-specific
absence slash.

Rules:

- Use a bold `\` slash for weather absence.
- The slash starts slightly above the icon and ends slightly below it.
- Use a high-contrast slash.
- In black-and-white, use the foreground color.
- On color displays, muted glyph plus high-contrast slash is acceptable.
- Do not use `?`.
- Use the same unavailable slash treatment as other metrics.

## BPM

BPM uses a heart-rate monitor / ECG-style waveform icon, not a heart.

Rules:

- Use a familiar and obvious heart-rate monitor glyph.
- Preserve BPM state color on color displays.
- Avoid overly emotional unavailable treatment.
- Unavailable BPM uses muted waveform plus the shared slash.
- Available BPM color may reflect BPM state:
  - normal: primary text
  - elevated: warning color
  - high: danger color

Unavailable treatment uses the shared absence mark.

## Battery

Rectangular and current unified layouts use a horizontal battery track rather
than a text percentage. The track is a small status band: the outer six-pixel
track is drawn as a primary-color outline, and the inner fill uses the
module-owned battery state color.

Rules:

- Fill amount must remain readable in black and white.
- Charging must use a visible shape cue, not color alone.
- Charging bolt should be prominent enough to read at watch scale.
- Use color for charge state only as a secondary cue.
- The track outline belongs to the primary-color visual vocabulary.
- The fill color is module-owned because it depends on live battery state.
- Medium battery should use `GColorRajah` on color displays.
  `GColorYellow` is too washed out in light mode.

States:

- low
- medium
- high
- charging

Charging state should not rely on green alone.

## Round Displays

Round displays should preserve the shared text-first hierarchy while using
round-aware geometry. Chalk `180x180` remains the pressure target and Gabbro
`260x260` is the larger expansion case.

Rules:

- Use the same surface stack before inventing a separate round-only product.
- Confirm readable chord width at each y-position.
- Keep time, battery, and date visually centered.
- Keep metric icons close to center-left or center-right vocabulary when
  they fit; do not default to corner-pinned complications.
- Disable optional icons before allowing text to clip.

## Review Workflow

Use `glyph-lab` for broad vocabulary work.

Recommended loop:

1. Define the glyph decision in prose.
2. Identify the state variants that actually exist in the product.
3. Create or update lab variants only for those states.
4. Review color and black-and-white screenshots.
5. Review at least `aplite` and `emery` before production porting.
6. Pick one direction.
7. Port the selected drawing rules into production modules.
8. Validate in the real watchface layout.

Do not use the production watchface as the first sketch surface for broad
glyph vocabulary changes.

## Current Decisions

- Unavailable data uses the shared `\` slash vocabulary.
- Text layers are the must-initialize visual controls; icons are
  subordinate and optional.
- Visual channels have fixed jobs: text for exact values, icons for metric
  identity, lines for progress, and color for state.
- Palette resolution has a black-and-white fallback: black background
  with white foreground in dark mode, white background with black
  foreground in light mode.
- Steps use the station-style footprint primitive.
- Weather uses bold line/outline first in black-and-white.
- Color weather glyphs may add fills or accents.
- Cloud, drizzle, rain, snow, and sleet use module-owned climate palette colors
  in color mode.
- Primary weather strokes use 3px unless a specific glyph needs an exception.
- Clear sky uses a filled disk with radius 9px and a 1px outer ring at
  radius 11px. Color mode uses the same ring/disk pattern as
  black-and-white.
- Thunderstorm uses a bolt only. Do not add a cloud.
- Rain, heavy rain, and snowy-rain may drop the cloud.
- Fog can be bars-first and may drop the cloud.
- Weather variants use a 70% cloud only when the cloud helps recognition.
- Cloudy currently uses one cloud.
- Partly cloudy uses a subordinate sun behind a dominant reduced cloud.
- The top context row is BPM and the bottom context row is steps on health
  platforms.
