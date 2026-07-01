# User Interface

This document records the current UI of At A Glance.

It is the reference for the implemented stack, geometry, fonts, palettes, icon sizes, glyph realization, and screenshot status. It does not define product principles, visual rules, or runtime ownership.

## Required Reading

- `../README.md`
- `Design.md`
- `ProductInvariants.md`
- `VisualVocabulary.md`

## Current Screenshots

Screenshots are still placeholders until the visual QA pass captures the final emulator set.

Use this document to track what has been captured, what is still pending, and which platforms or display modes still need review.

## Current Visual Stack

The current stack is shared across display families:

```text
top steps context
dominant centered time
centered battery track and charging bolt
weather/date context
bottom heart-rate context
```

Rectangular and round displays use different geometry, but this is the implemented stack on both families.

## Current Geometry

Geometry is platform-specific and integer-based.

The current implementation uses shape-aware layout and platform-aware styling rather than scaling one master layout across every device.

Health rows are present only on `PBL_HEALTH` builds.

## Current Coordinates

Coordinates are `x,y,w,h` and reflect the current architect formulas.

### Emery

- Platform: Pebble Time 2
- Face: `200x228`
- Classification: full

| Element | Frame |
| --- | --- |
| Steps icon | `69,10,28,28` |
| Steps text | `100,10,56,28` |
| Steps progress | `69,38,87,4` |
| Time text | `4,62,192,60` |
| Battery track | `25,127,150,8` |
| Battery fill | `26,128,148,6` |
| Battery bolt | `178,122,18,18` |
| Climate text | `4,140,62,28` |
| Climate icon | `69,140,28,28` |
| Date text | `100,140,99,28` |
| BPM icon | `69,190,28,28` |
| BPM text | `100,190,36,28` |

### Gabbro

- Platform: Pebble Round 2
- Face: `260x260`
- Classification: full

| Element | Frame |
| --- | --- |
| Steps icon | `99,10,28,28` |
| Steps text | `130,10,56,28` |
| Steps progress | `99,38,87,4` |
| Time text | `4,70,252,72` |
| Battery track | `32,147,195,8` |
| Battery fill | `33,148,193,6` |
| Battery bolt | `230,142,18,18` |
| Climate text | `4,160,92,28` |
| Climate icon | `99,160,28,28` |
| Date text | `130,160,129,28` |
| BPM icon | `99,222,28,28` |
| BPM text | `130,222,36,28` |

### Chalk

- Platform: Pebble Time Round
- Face: `180x180`
- Classification: compact

| Element | Frame |
| --- | --- |
| Steps icon | `69,10,18,18` |
| Steps text | `90,10,40,18` |
| Steps progress | `69,28,61,4` |
| Time text | `2,43,176,40` |
| Battery track | `22,88,135,8` |
| Battery fill | `23,89,133,6` |
| Battery bolt | `160,83,18,18` |
| Climate text | `2,101,64,18` |
| Climate icon | `69,101,18,18` |
| Date text | `90,101,89,18` |
| BPM icon | `69,152,18,18` |
| BPM text | `90,152,30,18` |

### Flint

- Platform: Pebble 2 Duo
- Face: `144x168`
- Classification: compact

| Element | Frame |
| --- | --- |
| Steps icon | `51,6,18,18` |
| Steps text | `72,6,40,18` |
| Steps progress | `51,24,61,4` |
| Time text | `2,40,140,40` |
| Battery track | `18,85,108,8` |
| Battery fill | `19,86,106,6` |
| Battery bolt | `129,80,18,18` |
| Climate text | `2,98,46,18` |
| Climate icon | `51,98,18,18` |
| Date text | `72,98,71,18` |
| BPM icon | `51,144,18,18` |
| BPM text | `72,144,30,18` |

### Aplite

- Platform: Pebble Classic / Aplite
- Face: `144x168`
- Classification: compact
- Health: not present

| Element | Frame |
| --- | --- |
| Time text | `2,40,140,40` |
| Battery track | `18,85,108,8` |
| Battery fill | `19,86,106,6` |
| Battery bolt | `129,80,18,18` |
| Climate text | `2,98,46,18` |
| Climate icon | `51,98,18,18` |
| Date text | `72,98,71,18` |

### Current Fonts

The stylist selects fonts by display class (compact or full) and font role. The `Text` role is shared by all non-time, non-date fields (for example, climate, BPM, and steps). System fonts are selected first and replaced by the corresponding custom font when it loads successfully.

| Role | Compact system font | Compact custom font | Full system font | Full custom font |
| --- | --- | --- | --- | --- |
| Time | `FONT_KEY_BITHAM_34_MEDIUM_NUMBERS` | `RESOURCE_ID_FONT_CABIN_SEMIBOLD_40` | `FONT_KEY_ROBOTO_BOLD_SUBSET_49` | `RESOURCE_ID_FONT_CABIN_SEMIBOLD_60`<br>`RESOURCE_ID_FONT_CABIN_SEMIBOLD_72` (Gabbro) |
| Date | `FONT_KEY_GOTHIC_18_BOLD` | `RESOURCE_ID_FONT_ANTONIO_SEMIBOLD_16` | `FONT_KEY_GOTHIC_28_BOLD` | `RESOURCE_ID_FONT_ANTONIO_SEMIBOLD_22` |
| Text (Climate, BPM, Steps, etc.) | `FONT_KEY_GOTHIC_18` | `RESOURCE_ID_FONT_ANTONIO_REGULAR_16` | `FONT_KEY_GOTHIC_28` | `RESOURCE_ID_FONT_ANTONIO_REGULAR_22` |

Text metrics are defined per field. Font selection depends on role, display class, and platform, with Gabbro using a larger custom time font on large-display devices.

## Current Palettes

Static palettes are defined in `layout_stylist.c`. Dynamic BPM and battery-state colors remain owned by their respective modules.

Each palette defines the background, primary text, unavailable text, date, and time colors used for a display mode.

| Palette | Mode | Capability | Background | Primary text | Unavailable text | Date | Time |
| --- | --- | --- | --- | --- | --- | --- | --- |
| **Clear as Celeste** | Light | Color | `GColorCeleste` | `GColorBlack` | `GColorDarkGray` | `GColorOxfordBlue` | `GColorOrange` |
| **Black on White** | Light | Monochrome | `GColorWhite` | `GColorBlack` | `GColorBlack` | `GColorBlack` | `GColorBlack` |
| **Night in Oxford** | Dark | Color | `GColorOxfordBlue` | `GColorWhite` | `GColorLightGray` | `GColorCeleste` | `GColorOrange` |
| **White on Black** | Dark | Monochrome | `GColorBlack` | `GColorWhite` | `GColorWhite` | `GColorWhite` | `GColorWhite` |

On color-capable watches, all four modes are available. The first two rows above describe the monochrome-capability palettes. When those same modes are rendered on color hardware, time remains orange. The latter two are the color-background palettes.

Module-owned colors then layer on top for battery, BPM, steps, and climate state. On monochrome displays, weather glyphs use heavier outline and line treatment so the same conditions still read clearly without color.

## Current Icon Sizes

Current icon sizes are stable within the compact and full layout classes:

- Compact icon size: `18x18`
- Full icon size: `28x28`

## Current Glyph Realization

The current glyph system uses familiar heart-and-waveform, weather, and walking marks that read at watch scale.

Walking and heart-and-waveform icons are recolored bitmap assets in the current implementation. Their color is selected based on the metric's current value.

Battery uses a track to convey level. The outer track is a primary-color outline, and the inner fill uses the battery module's live state color. Charging uses a visible bolt so the state does not rely on green alone.

Weather glyph treatment varies by capability. Black-and-white rendering favors bold line and outline treatment. Color rendering may add fills or accents when they help recognition without becoming the only source of meaning.

Unavailable states use the shared absence slash.
