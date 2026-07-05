# User Interface

This document records the current UI of At A Glance.

It is the reference for the implemented stack, geometry, fonts, palettes, icon sizes, glyph realization, and screenshot status. It does not define product principles, visual rules, or runtime ownership.

## Required Reading

- `../README.md`
- `Design.md`
- `ProductInvariants.md`
- `VisualVocabulary.md`

## Current Visual Stack

The current stack is shared across display families:

```text
top steps context
dominant centered time
centered battery track and plugged-in bolt
weather/date context
bottom heart-rate context
```

Rectangular and round displays implement the same information hierarchy and visual stack.

## Current Information Placement

- Platform-specific and integer-based.
- Resolved from defined layout constants for the active platform rather than scaling one master layout across every device.
- Health rows are present only on `PBL_HEALTH` builds.

## Current Coordinates

Coordinates are `x,y,w,h`.

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

### Typography

The stylist selects fonts by display class (compact or full) and font role. The `Text` role is shared by all non-time, non-date fields (for example, climate, BPM, and steps). System fonts are selected first and replaced by the corresponding custom font when it loads successfully.

Custom Fonts: ##Cabin## for Time, ##Barlow Condensed## for date & metrics
Font sizes: Customized for display sizes

| Role | Compact system font | Compact custom font | Full system font | Full custom font |
| --- | --- | --- | --- | --- |
| Time | `FONT_KEY_BITHAM_34_MEDIUM_NUMBERS` | `RESOURCE_ID_FONT_CABIN_SEMIBOLD_40` | `FONT_KEY_ROBOTO_BOLD_SUBSET_49` | `RESOURCE_ID_FONT_CABIN_SEMIBOLD_60`<br>`RESOURCE_ID_FONT_CABIN_SEMIBOLD_72` (Gabbro) |
| Date | `FONT_KEY_GOTHIC_18_BOLD` | `RESOURCE_ID_FONT_TEXT_SEMIBOLD_16` | `FONT_KEY_GOTHIC_28_BOLD` | `RESOURCE_ID_FONT_TEXT_SEMIBOLD_22` |
| Text (Climate, BPM, Steps, etc.) | `FONT_KEY_GOTHIC_18` | `RESOURCE_ID_FONT_TEXT_SEMIBOLD_16` | `FONT_KEY_GOTHIC_28` | `RESOURCE_ID_FONT_TEXT_SEMIBOLD_22` |

- Text metrics -- coordinates, height, width --  are defined per field.
- Font selection depends on role, display class, and platform.

## Supported Display Modes

- There are four display modes.
  - Each mode uses a defined palette to render text, icons, and convey state.
- On color-capable watches, all four modes are available.
- On monochrome-capable watches, two monochrome-specific modes are available.


### Display Mode Palette Characteristics

- Each palette is composed of five mutually exclusive and collectively exhaustive sets of colors.
- The combined palette for each mode satisfies the design invariant of perceptual fluency.

1. Primary Display
2. Module: Battery
3. Module: BPM
4. Module: Climate
5. Module: Steps

### Realization of Shared Visual Identity

1. On Color hardware, time remains orange.
2. On Monochrome hardware, light and dark palettes are mirror images of one another.
3. Palette pairs ({mono-light, mono-dark}, {color-light, color-dark}) have {date text, background} in one swapped with their opposite metric in the other.
4. Module displays define `normal` and `unknown` that mirror values from the active primary display palette.

### Primary Display Palette

- Defines the background, primary text, unavailable text, date, and time colors used.
- Colors are defined in `layout_stylist.c`.

| Palette | Mode | Display Type | Background | Primary text | Unavailable text | Date | Time |
| --- | --- | --- | --- | --- | --- | --- | --- |
| **1. Black on White** | Light | Monochrome | `GColorWhite` | `GColorBlack` | `GColorBlack` | `GColorBlack` | `GColorBlack` |
| **2. White on Black** | Dark | Monochrome | `GColorBlack` | `GColorWhite` | `GColorWhite` | `GColorWhite` | `GColorWhite` |
| **3. Clear as Celeste** | Light | Color | `GColorCeleste` | `GColorBlack` | `GColorDarkGray` | `GColorOxfordBlue` | `GColorOrange` |
| **4. Night in Oxford** | Dark | Color | `GColorOxfordBlue` | `GColorWhite` | `GColorLightGray` | `GColorCeleste` | `GColorOrange` |

### BPM Palette

- The BPM module palette is initialized in `bpm.c`
- `warning` and `critical` come from the BPM-specific light/dark template.

| Palette | Mode | Display Type | Background | Normal | Unavailable | Warning (`>=100`) | Critical (`>=120`) |
| --- | --- | --- | --- | --- | --- | --- | --- |
| **1. Black on White** | Light | Monochrome | `GColorWhite` | `GColorBlack` | `GColorBlack` | `GColorBlack` | `GColorBlack` |
| **2. White on Black** | Dark | Monochrome | `GColorBlack` | `GColorWhite` | `GColorWhite` | `GColorWhite` | `GColorWhite` |
| **3. Clear as Celeste** | Light | Color | `GColorCeleste` | `GColorBlack` | `GColorDarkGray` | `GColorVividViolet` | `GColorRed` |
| **4. Night in Oxford** | Dark | Color | `GColorOxfordBlue` | `GColorWhite` | `GColorLightGray` | `GColorYellow` | `GColorShockingPink` |

### Battery Palette

- The battery module palette is initialized in `battery.c`.
- `medium`, `low`, and `pluggedin` come from the battery-specific light/dark template.

| Palette | Mode | Display Type | Background | Normal (`>50`) | Medium (`21-50`) | Low (`<=20`) | Plugged In |
| --- | --- | --- | --- | --- | --- | --- | --- |
| **1. Black on White** | Light | Monochrome | `GColorWhite` | `GColorBlack` | `GColorBlack` | `GColorBlack` | `GColorBlack` |
| **2. White on Black** | Dark | Monochrome | `GColorBlack` | `GColorWhite` | `GColorWhite` | `GColorWhite` | `GColorWhite` |
| **3. Clear as Celeste** | Light | Color | `GColorCeleste` | `GColorBlack` | `GColorVividViolet` | `GColorRed` | `GColorGreen` |
| **4. Night in Oxford** | Dark | Color | `GColorOxfordBlue` | `GColorWhite` | `GColorYellow` | `GColorShockingPink` | `GColorIslamicGreen` |

### Climate Palette

- The climate module palette is initialized in `climate.c` and consumed by `climate_glyphs.c`.
- Colors selected enhance weather condition identification.
- On monochrome displays, weather glyphs use heavier outline and line treatment so the same conditions still read clearly sans color.

| Palette | Mode | Display Type | Background | Normal | Unavailable | Sun | Cold | Cloud | Clear Ring (night) | Clear Fill |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| **Clear as Celeste** | Light | Color | `GColorCeleste` | `GColorBlack` | `GColorDarkGray` | `GColorWindsorTan` | `GColorCobaltBlue` | `GColorBlue` | `GColorBabyBlueEyes` | `GColorBabyBlueEyes` |
| **Black on White** | Light | Monochrome | `GColorWhite` | `GColorBlack` | `GColorBlack` | `GColorBlack` | `GColorBlack` | `GColorBlack` | `GColorDarkGray` | `GColorDarkGray` |
| **Night in Oxford** | Dark | Color | `GColorOxfordBlue` | `GColorWhite` | `GColorLightGray` | `GColorChromeYellow` | `GColorMintGreen` | `GColorElectricBlue` | `GColorBabyBlueEyes` | `GColorBabyBlueEyes` |
| **White on Black** | Dark | Monochrome | `GColorBlack` | `GColorWhite` | `GColorWhite` | `GColorWhite` | `GColorWhite` | `GColorWhite` | `GColorWhite` | `GColorLightGray` |

### Steps Palette

- The steps module palette is initialized in `steps.c`.
- `approaching` and `achieved` come from the steps-specific light/dark template.

| Palette | Mode | Display Type | Background | Normal | Unavailable | Approaching (`>70%` of goal) | Achieved (`>= goal`) |
| --- | --- | --- | --- | --- | --- | --- | --- |
| **1. Black on White** | Light | Monochrome | `GColorWhite` | `GColorBlack` | `GColorBlack` | `GColorBlack` | `GColorBlack` |
| **2. White on Black** | Dark | Monochrome | `GColorBlack` | `GColorWhite` | `GColorWhite` | `GColorWhite` | `GColorWhite` |
| **3. Clear as Celeste** | Light | Color | `GColorCeleste` | `GColorBlack` | `GColorDarkGray` | `GColorVividViolet` | `GColorIslamicGreen` |
| **4. Night in Oxford** | Dark | Color | `GColorOxfordBlue` | `GColorWhite` | `GColorLightGray` | `GColorYellow` | `GColorIslamicGreen` |

## Current Icon Sizes

Current icon sizes are stable within the compact and full layout classes:

- Compact icon size: `18x18`
- Full icon size: `28x28`

## Current Glyph Realization

The current glyph system uses familiar heart-and-waveform, weather, and walking marks that read at watch scale.

Walking and heart-and-waveform icons are recolored bitmap assets in the current implementation. Their color is selected based on the metric's current value.

Battery uses a track to convey level. The outer track is a primary-color outline, and the inner fill uses the battery module's live state color. A visible bolt indicates that the watch is plugged in. On color displays, the plugged-in state also shifts the battery color to the module's plugged-in color.

Weather glyph treatment varies by capability. Black-and-white rendering favors bold line and outline treatment. Color rendering may add fills or accents when they help recognition without becoming the only source of meaning.

Unavailable states use the shared absence slash.
