# User Interface

This document records the current UI of _At A Glance_. It is the reference for the implemented stack, typography, palettes, icon sizes, glyph realization, and screenshot status.

## Adjacent

- [ProductInvariants](ProductInvariants.md) defines the properties this implementation must satisfy.
- [VisualVocabulary](VisualVocabulary.md) defines the visual rules represented here.

## Visual Evidence

### On Device

Current device screenshots are checked in under `docs/assets/screenshots/`. Together with the palettes, typography, and glyph realization tables below, they provide evidence for legibility, placement, and compatibility across supported devices.

|  |  |  |
| :-: | :-: | :-: |
| <b><img src="assets/screenshots/at-a-glance-emery-blackonwhite.png" alt="Emery Black on White" width="160"><br>Emery: Black on White</b> | <b><img src="assets/screenshots/at-a-glance-emery-whiteonblack.png" alt="Emery White on Black" width="160"><br>Emery: White on Black</b> | <b><img src="assets/screenshots/at-a-glance-chalk-celeste.png" alt="Chalk Clear as Celeste" width="160"><br>Chalk: Clear as Celeste</b> |
| <b><img src="assets/screenshots/at-a-glance-aplite-blackonwhite.png" alt="Aplite Black on White" width="130"><br>Aplite: Black on White</b> | <b><img src="assets/screenshots/at-a-glance-flint-whiteonblack.png" alt="Flint White on Black" width="130"><br>Flint: White on Black</b> | <b><img src="assets/screenshots/at-a-glance-gabbro-nightinoxford.png" alt="Gabbro Night in Oxford" width="160"><br>Gabbro: Night in Oxford</b> |

Glyph validation is covered in [Contributing](Contributing.md). Prioritize it when glyphs are modified or introduced.

### Settings

The current Clay settings page is captured here as interface evidence only. The settings contract, defaults, valid ranges, AppMessage keys, PKJS normalization, and persistence rules live in [SettingsandConfiguration](SettingsandConfiguration.md).

<img src="assets/screenshots/at-a-glance-settings.png" alt="At A Glance settings page" width="320"/>

## Current Visual Stack

The current stack and information hierarchy is shared across display families and geometries:

```text
top steps context
dominant centered time
centered battery track and plugged-in bolt
weather/date context
centered location
bottom heart-rate context
```

### Typography

The `layout_stylist` selects fonts by display class (compact or full) and font role. The `Text` role is shared by climate, BPM, and steps; location has its own font role. System fonts are selected first and replaced by the corresponding custom font when it loads successfully.

**Notes**

- Sizes: Font sizes are customized for compact and full displays.
- Custom fonts: **Cabin** variants for time and location, **Oswald** for date and metrics.
- Location text: PKJS limits production location names to 15 characters and normalizes them to uppercase; uppercase text is more legible at selected location-font sizes.
- Location fonts: Retain lowercase and Latin-1 glyphs so direct AppMessage and QA inputs render without a separate font restriction.

| Role | Compact system font | Compact custom font | Full system font | Full custom font |
| :-- | :-- | :-- | :-- | :-- |
| <b>Time</b> | `FONT_KEY_BITHAM_42_MEDIUM_NUMBERS` | `RESOURCE_ID_FONT_CABIN_MEDIUM_42` | `FONT_KEY_LECO_60_NUMBERS_AM_PM` (Emery)<br>`FONT_KEY_LECO_60_BOLD_NUMBERS_AM_PM` (Gabbro) | `RESOURCE_ID_FONT_CABIN_MEDIUM_58` (Emery)<br>`RESOURCE_ID_FONT_CABIN_MEDIUM_70` (Gabbro) |
| <b>Date</b> | `FONT_KEY_GOTHIC_18_BOLD` | `RESOURCE_ID_FONT_DATE_TEXT_15` | `FONT_KEY_GOTHIC_24_BOLD` | `RESOURCE_ID_FONT_DATE_TEXT_20` |
| <b>Text (Steps, Temp., etc.)</b> | `FONT_KEY_GOTHIC_18_BOLD` | `RESOURCE_ID_FONT_DATE_TEXT_15` | `FONT_KEY_GOTHIC_24_BOLD` | `RESOURCE_ID_FONT_DATE_TEXT_20` |
| <b>Location</b> | `FONT_KEY_GOTHIC_18` | `RESOURCE_ID_FONT_LOCATION_14` | `FONT_KEY_GOTHIC_24_BOLD` | `RESOURCE_ID_FONT_LOCATION_16` |

- Font selection depends on role, display class, and platform.

## Supported Display Modes

- On color-capable watches, four modes are available.
- On monochrome-capable watches, two monochrome-specific modes are available.
- Each mode uses a defined palette to render text, icons, and convey state.
- Click the Back button three times to cycle display modes.
- The watch repaints immediately when the mode changes.

This satisfies the invariant that choices are available on all devices.

### Display Mode Palette Characteristics

The combined palette for each mode satisfies the design invariant of perceptual fluency. Colors within each palette are mutually exclusive to satisfy, "one visual channel, one responsibility", visual invariant.

There are five sub-palettes that harmonize together to maximize contrast and convey state.

1. Primary Display
2. Module: Battery
3. Module: BPM
4. Module: Climate
5. Module: Steps

### Realization of Shared Visual Identity

1. On Monochrome hardware, light and dark palettes are mirror images of one another.
2. Palette pairs ({mono-light, mono-dark}, {color-light, color-dark}) have {time+date text, background} in one swapped with their opposite metric in the other.
3. Module displays define `normal` and `unknown` that mirror values from the active primary display palette.

### Consolidated Color Palettes for Display Modes

**Source of truth**: Current palette code in `layout_stylist.c`, `climate.c`, `bpm.c`, `steps.c`, `battery.c`.

**Columns**

- Color's primary role
- `Mono-Light BW`: `DISPLAY_MODE_LIGHT_MONOCHROME` (on monochrome)
- `Mono-Dark BW`: `DISPLAY_MODE_DARK_MONOCHROME` (on monochrome)
- `Mono-Light C`: `DISPLAY_MODE_LIGHT_MONOCHROME` (on color)
- `Mono-Dark C`: `DISPLAY_MODE_DARK_MONOCHROME` (on color)
- `Clear as Celeste`: `DISPLAY_MODE_LIGHT_COLOR` (on color)
- `Night in Oxford`: `DISPLAY_MODE_DARK_COLOR` (on color)

| Color role | Mono-Light BW | Mono-Dark BW | Mono-Light C | Mono-Dark C | Clear as Celeste | Night in Oxford |
| :-- | :-: | :-: | :-: | :-: | :-: | :-: |
| **PRIMARY** |  |  |  |  |  |  |
| Background | `GColorWhite` | `GColorBlack` | `GColorWhite` | `GColorBlack` | `GColorCeleste` | `GColorOxfordBlue` |
| Primary text | `GColorBlack` | `GColorWhite` | `GColorBlack` | `GColorWhite` | `GColorBlack` | `GColorWhite` |
| Out-of-Range text | `GColorBlack` | `GColorWhite` | `GColorBlack` | `GColorWhite` | `GColorDarkGray` | `GColorLightGray` |
| Date | `GColorBlack` | `GColorWhite` | `GColorBlack` | `GColorWhite` | `GColorOxfordBlue` | `GColorCeleste` |
| Time | `GColorBlack` | `GColorWhite` | `GColorBlack` | `GColorWhite` | `GColorOxfordBlue` | `GColorCeleste` |
| **BATTERY** |  |  |  |  |  |  |
| Background | `GColorWhite` | `GColorBlack` | `GColorWhite` | `GColorBlack` | `GColorCeleste` | `GColorOxfordBlue` |
| Normal (`>50`) | `GColorBlack` | `GColorWhite` | `GColorBlack` | `GColorWhite` | `GColorBlack` | `GColorWhite` |
| Medium (`21-50`) | `GColorBlack` | `GColorWhite` | `GColorVividViolet` | `GColorIcterine` | `GColorVividViolet` | `GColorIcterine` |
| Critical (`<=20`) | `GColorBlack` | `GColorWhite` | `GColorRed` | `GColorRed` | `GColorRed` | `GColorRed` |
| Plugged In | `GColorBlack` | `GColorWhite` | `GColorDarkGreen` | `GColorGreen` | `GColorDarkGreen` | `GColorGreen` |
| **STEPS** |  |  |  |  |  |  |
| Background | `GColorWhite` | `GColorBlack` | `GColorWhite` | `GColorBlack` | `GColorCeleste` | `GColorOxfordBlue` |
| Normal | `GColorBlack` | `GColorWhite` | `GColorBlack` | `GColorWhite` | `GColorBlack` | `GColorWhite` |
| Out-of-Range | `GColorBlack` | `GColorWhite` | `GColorBlack` | `GColorWhite` | `GColorDarkGray` | `GColorLightGray` |
| Approaching (`>70%` of goal) | `GColorBlack` | `GColorWhite` | `GColorVividViolet` | `GColorIcterine` | `GColorVividViolet` | `GColorIcterine` |
| Achieved (`>= goal`) | `GColorBlack` | `GColorWhite` | `GColorGreen` | `GColorIslamicGreen` | `GColorGreen` | `GColorIslamicGreen` |
| **CLIMATE** |  |  |  |  |  |  |
| Background | `GColorWhite` | `GColorBlack` | `GColorWhite` | `GColorBlack` | `GColorCeleste` | `GColorOxfordBlue` |
| Normal | `GColorBlack` | `GColorWhite` | `GColorBlack` | `GColorWhite` | `GColorBlack` | `GColorWhite` |
| Out-of-Range | `GColorBlack` | `GColorWhite` | `GColorBlack` | `GColorWhite` | `GColorDarkGray` | `GColorLightGray` |
| Sun | `GColorBlack` | `GColorWhite` | `GColorWindsorTan` | `GColorChromeYellow` | `GColorWindsorTan` | `GColorChromeYellow` |
| Cold | `GColorBlack` | `GColorWhite` | `GColorCobaltBlue` | `GColorMintGreen` | `GColorCobaltBlue` | `GColorMintGreen` |
| Cloud | `GColorBlack` | `GColorWhite` | `GColorBlue` | `GColorElectricBlue` | `GColorBlue` | `GColorElectricBlue` |
| Clear Ring (night) | `GColorLightGray` | `GColorLightGray` | `GColorLightGray` | `GColorBabyBlueEyes` | `GColorLightGray` | `GColorBabyBlueEyes` |
| Clear Fill | `GColorDarkGray` | `GColorLightGray` | `GColorDarkGray` | `GColorBabyBlueEyes` | `GColorDarkGray` | `GColorBabyBlueEyes` |
| **HEART-RATE BPM** |  |  |  |  |  |  |
| Background | `GColorWhite` | `GColorBlack` | `GColorWhite` | `GColorBlack` | `GColorCeleste` | `GColorOxfordBlue` |
| Normal | `GColorBlack` | `GColorWhite` | `GColorBlack` | `GColorWhite` | `GColorBlack` | `GColorWhite` |
| Out-of-Range | `GColorBlack` | `GColorWhite` | `GColorBlack` | `GColorWhite` | `GColorDarkGray` | `GColorLightGray` |
| Elevated (`>=100`) | `GColorBlack` | `GColorWhite` | `GColorVividViolet` | `GColorIcterine` | `GColorVividViolet` | `GColorIcterine` |
| Critical (`>=120`) | `GColorBlack` | `GColorWhite` | `GColorRed` | `GColorRed` | `GColorRed` | `GColorRed` |

## Current Icon Sizes

- **Compact** icon size: `18x18`
- **Full** icon size: `28x28`

## Current Glyph Realization

The current glyph system uses familiar heart-and-waveform, weather, walking, battery, and power marks that read at watch scale.

### Health Glyphs

- Steps use a walking bitmap asset.
- BPM uses an ECG bitmap asset.
- Walking and ECG bitmap foreground pixels are recolored from the metric's live state color.
- Health text uses normal text color when available and out-of-range text color when unavailable.
- Out-of-range steps and BPM draw the shared absence slash over the icon.

### Battery Glyphs

- Battery uses a horizontal track to convey charge level.
- The track outline and fill use the live battery state color.
- The fill width is proportional to charge percentage.
- A filled bolt appears only when the watch is plugged in.
- On color displays, plugged-in state also shifts the battery color to the plugged-in palette color.

### Climate Glyphs

Climate glyphs are procedural and drawn in the climate icon frame. The current procedural glyph reference grid is `28x28`. Weather condition codes are grouped into the current icon families:

| Weather code range | Current icon family |
| ------------------ | ------------------- |
| `<0` or `>99`      | Out-of-range        |
| `0-1`              | Clear               |
| `2`                | Partly cloudy       |
| `3`                | Cloud               |
| `4-48`             | Fog                 |
| `49-55`            | Drizzle             |
| `56-57`            | Sleet drizzle       |
| `58-63`            | Rain                |
| `64-65`            | Heavy rain          |
| `66-67`            | Heavy sleet         |
| `68-77`            | Snow                |
| `78-81`            | Showers             |
| `82`               | Heavy showers       |
| `83-86`            | Snow showers        |
| `87-99`            | Thunderstorm        |

**Current climate glyph realization**

- Day clear weather uses a sun glyph.
- Night clear weather uses a filled/ringed clear glyph.
- Partly cloudy draws a reduced clear glyph behind a cloud.
- Cloud uses three lobes plus a shared body fill so the silhouette reads as one cloud.
- Fog uses three horizontal bars.
- Drizzle uses three heavier staggered marks.
- Rain uses slanted rain marks; heavy rain uses longer marks.
- Snow uses a procedural snowflake with six spokes, chevrons, and a center dot.
- Sleet combines cloud, snowflake, and rain marks.
- Showers combine a cloud with rain marks while keeping precipitation separate from the cloud baseline.
- Snow showers combine a cloud with a snowflake while keeping precipitation visually separate from the cloud shape.
- Thunderstorm uses the shared filled bolt primitive.
- Out-of-range weather draws a cloud with the shared absence slash.

### Out-of-range Visual Identification: Left-to-Right Diagonal Slash

Out-of-range metric and weather states use a shared diagonal slash. The slash is drawn from the 28x28 design-space diagonal with three line segments and a 2px stroke, then scaled to the active icon frame.

## Read Next

- [WatchfaceImplementationFlow](WatchfaceImplementationFlow.md) for implementation flow, source responsibilities, and interconnections.
- [SettingsandConfiguration](SettingsandConfiguration.md) for the settings catalog.
- [Validation](Validation.md) for validation contract, scenarios, and visual review evidence.
