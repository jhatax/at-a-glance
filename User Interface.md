# User Interface

This document is the UI reference for screenshots, computed layout, font
selections, and palettes. It should be updated when visual geometry, fonts,
or palette values change.

Screenshots are intentionally slots until the App Store visual QA pass
captures final emulator images.

## Metric Rules

- Text metrics are per-field, not globally uniform.
- Width and height choices are dictated by font selection, content role, row
  ownership, and platform geometry.
- Time, date, climate, BPM, and steps do not need to share a text width.
- Icon sizes remain stable within compact or full geometry.
- Geometry changes require screenshot review on the affected platform class.

## Font Selections

The stylist selects fonts by compact/full classification and font role.

| Role | Compact system font | Compact custom font | Full system font | Full custom font |
| --- | --- | --- | --- | --- |
| Date | `FONT_KEY_GOTHIC_14_BOLD` | `RESOURCE_ID_FONT_CABIN_TEXT_14_MEDIUM` | `FONT_KEY_GOTHIC_18_BOLD` | `RESOURCE_ID_FONT_CABIN_TEXT_18_MEDIUM` |
| Time | `FONT_KEY_BITHAM_34_MEDIUM_NUMBERS` | `RESOURCE_ID_FONT_CABIN_TIME_36_SEMIBOLD` | `FONT_KEY_ROBOTO_BOLD_SUBSET_49` | `RESOURCE_ID_FONT_CABIN_TIME_48_SEMIBOLD` |
| Climate | `FONT_KEY_GOTHIC_14` | `RESOURCE_ID_FONT_CABIN_TEXT_14_MEDIUM` | `FONT_KEY_GOTHIC_18` | `RESOURCE_ID_FONT_CABIN_TEXT_18_MEDIUM` |
| BPM | `FONT_KEY_GOTHIC_14` | `RESOURCE_ID_FONT_CABIN_TEXT_14_MEDIUM` | `FONT_KEY_GOTHIC_18` | `RESOURCE_ID_FONT_CABIN_TEXT_18_MEDIUM` |
| Steps | `FONT_KEY_GOTHIC_14` | `RESOURCE_ID_FONT_CABIN_TEXT_14_MEDIUM` | `FONT_KEY_GOTHIC_18` | `RESOURCE_ID_FONT_CABIN_TEXT_18_MEDIUM` |

## Palettes

Static palette roles come from `layout_stylist.c`. Dynamic BPM and battery
state colors remain module-owned.

| Mode | Capability | Background | Primary text | Unavailable text | Date | Time |
| --- | --- | --- | --- | --- | --- | --- |
| Light | Color | `GColorWhite` | `GColorCobaltBlue` | `GColorDarkGray` | `GColorBlack` | `GColorSunsetOrange` |
| Light | BnW | `GColorWhite` | `GColorBlack` | `GColorBlack` | `GColorBlack` | `GColorBlack` |
| Dark | Color | `GColorBlack` | `GColorCeleste` | `GColorLightGray` | `GColorElectricBlue` | `GColorSunsetOrange` |
| Dark | BnW | `GColorBlack` | `GColorWhite` | `GColorWhite` | `GColorWhite` | `GColorWhite` |

## Computed Geometry

Coordinates are `x,y,w,h` and reflect the current integer architect formulas.
Health rows are present only on `PBL_HEALTH` builds.

### Flint

- Platform: Pebble 2 Duo
- Face: `144x168`
- Classification: compact
- Screenshot: pending

| Element | Frame |
| --- | --- |
| Time text | `2,42,140,38` |
| Battery track | `22,85,100,6` |
| Battery fill | `23,86,98,4` |
| Battery bolt | `123,80,16,16` |
| Climate text | `2,96,39,20` |
| Climate icon | `42,96,16,16` |
| Date text | `59,96,83,20` |
| BPM icon | `44,4,16,16` |
| BPM text | `61,2,39,20` |
| Steps icon | `44,148,16,16` |
| Steps text | `61,146,39,20` |

### Chalk

- Platform: Pebble Time Round
- Face: `180x180`
- Classification: compact
- Screenshot: pending

| Element | Frame |
| --- | --- |
| Time text | `2,45,176,38` |
| Battery track | `27,88,126,6` |
| Battery fill | `28,89,124,4` |
| Battery bolt | `154,83,16,16` |
| Climate text | `2,99,39,20` |
| Climate icon | `42,99,16,16` |
| Date text | `59,99,119,20` |
| BPM icon | `62,4,16,16` |
| BPM text | `79,2,39,20` |
| Steps icon | `62,160,16,16` |
| Steps text | `79,158,39,20` |

### Emery

- Platform: Pebble Time 2
- Face: `200x228`
- Classification: full
- Screenshot: pending

| Element | Frame |
| --- | --- |
| Time text | `6,61,188,50` |
| Battery track | `30,116,140,6` |
| Battery fill | `31,117,138,4` |
| Battery bolt | `171,111,16,16` |
| Climate text | `6,127,46,20` |
| Climate icon | `53,127,24,24` |
| Date text | `78,127,116,20` |
| BPM icon | `67,6,24,24` |
| BPM text | `92,8,40,20` |
| Steps icon | `64,198,24,24` |
| Steps text | `89,200,46,20` |

### Gabbro

- Platform: Pebble Round 2
- Face: `260x260`
- Classification: full
- Screenshot: pending

| Element | Frame |
| --- | --- |
| Time text | `6,70,248,50` |
| Battery track | `39,125,182,6` |
| Battery fill | `40,126,180,4` |
| Battery bolt | `222,120,16,16` |
| Climate text | `6,136,46,20` |
| Climate icon | `53,136,24,24` |
| Date text | `78,136,176,20` |
| BPM icon | `97,6,24,24` |
| BPM text | `122,8,40,20` |
| Steps icon | `94,230,24,24` |
| Steps text | `119,232,46,20` |

## Screenshot Matrix

| Platform | Light color | Light BnW | Dark color | Dark BnW |
| --- | --- | --- | --- | --- |
| Flint | N/A | pending | N/A | pending |
| Chalk | pending | pending | pending | pending |
| Emery | pending | N/A | pending | N/A |
| Gabbro | pending | pending | pending | pending |
