# Life at a Glance

Swiss-Rail Inspired watchface for **Pebble Time 2** (emery).

## Features

- Left-aligned typographic layout with a mid-screen horizontal rule
- Time (12h or 24h) and short date (`THU · 28 MAY`)
- Heart rate and full step count from Pebble Health
- Temperature from phone (Open-Meteo) in °F or °C
- Battery percentage (bottom-right)
- Settings: time format, temperature unit, display mode, bpm sampling interval

## Target And Capabilities

- Platform target: `emery` (Pebble Time 2), `200x228`, color.
- Pebble SDK metadata: `sdkVersion: "3"` (Pebble SDK 4+ compatible workflow in this repo).
- Required Pebble capabilities from `package.json`:
  - `configurable` for Clay settings page.
  - `location` for weather lookup via phone geolocation.
  - `health` for step count and heart-rate metrics.

## Layout Spec (Current Implementation)

Screen hierarchy:

```text
DATE
TIME
---------------------- (rule at y=110)
HEART ICON + BPM        STEPS ICON + STEPS
TEMP              BATTERY ICON + BATTERY %
```

Key geometry from `src/c/main.c` and `src/c/ataglance.h`:

- Date layer: `GRect(12, 10, 176, 36)`
- Time layer: `GRect(12, 46, 176, 50)`
- Horizontal rule: from `(12, 110)` to `(188, 110)`
- Heart icon: `GRect(20, 118, 28, 28)`
- BPM value: `GRect(52, 118, 58, 36)`
- Steps icon: `GRect(94, 118, 28, 28)`
- Steps value: `GRect(126, 118, 54, 36)`
- Temperature: `GRect(44, 184, 64, 30)`
- Battery icon: `GRect(86, 184, 28, 28)`
- Battery value: `GRect(118, 184, 54, 30)`

## Typography And Color

Fonts used:

- Hero time: `FONT_KEY_ROBOTO_BOLD_SUBSET_49`
- Date: `FONT_KEY_GOTHIC_24_BOLD`
- BPM and steps values: `FONT_KEY_GOTHIC_28`
- Temperature and battery values: `FONT_KEY_GOTHIC_24_BOLD`

Color usage:

- Dark mode:
  - Background: `GColorBlack`
  - Time: `GColorSunsetOrange`
  - Date: `GColorRichBrilliantLavender`
  - Primary data text: `GColorLightGray`
  - Unavailable data fallback (`---`): `GColorWindsorTan`
  - Rule: `GColorLightGray`
- Light mode:
  - Background: `GColorWhite`
  - Time: `GColorSunsetOrange`
  - Date: `GColorImperialPurple`
  - Primary data text: `GColorBlack`
  - Unavailable data fallback (`---`): `GColorLightGray`
  - Rule: `GColorLightGray`
- Steps icon: `GColorChromeYellow`
- Heart icon and BPM text:
  - `<= 0` or unavailable: mode unavailable color
  - `1-99`: `GColorJaegerGreen`
  - `100-120`: `GColorMagenta`
  - `>120`: `GColorRed`
- Battery icon and battery `%`:
  - Charging: `GColorJaegerGreen`
  - Not charging, `>50%`: `GColorCobaltBlue`
  - Not charging, `21-50%`: `GColorYellow`
  - Not charging, `<=20%`: `GColorRed`

## Icons Used

- Heart icon: vector resource `ICON_BPM`
  (`resources/images/bpm_option-tick.pdc`), recolored in C by BPM zone.
- Steps icon: custom-drawn paw/footprint using `graphics_fill_circle`
  calls (no bitmap resource).
- Battery icon: custom-drawn AA-style battery in a 28x28 layer.
- App/menu icon: `resources/images/icon.png` (`MENU_ICON`).
- Hidden `ICON_FALLBACK_MODE` defaults disabled and can force procedural icons
  for testing.

## Configuration Page (Clay) Mockup

Current `src/pkjs/config.json`:

```text
At A Glance: Configuration
Select units and update frequencies for key capabilities.

Time format
  ( ) 24-hour
  ( ) 12-hour

Temperature unit
  ( ) Fahrenheit (°F)
  ( ) Celsius (°C)

Display mode
  ( ) Dark mode
  ( ) Light mode

HR Sampling Frequency
  ( ) Every 10-minutes
  ( ) Every 15-minutes
  ( ) Every 30-minutes
  ( ) Every 60-minutes
  ( ) Every 120-minutes

[ Save Settings ]

Hidden input:
  ICON_FALLBACK_MODE = 0 (disabled)
```

## Build

```bash
cd life-at-a-glance-emery-wf
npm install
pebble build
```

Output: `build/life-at-a-glance-emery-wf.pbw`

## Install

```bash
pebble install --emulator emery
# or on hardware via phone:
pebble install --phone YOUR_PHONE_IP
```

## Settings

In the Pebble app on your phone: Life at a Glance → Settings

- **Time format:** 24-hour or 12-hour
- **Temperature unit:** Fahrenheit or Celsius
- **Display mode:** dark or light mode
- **Heart rate sampling:** every 10, 15, 30, 60, or 120 minutes (default: 10)

Temperature uses your phone’s location (falls back to NYC if unavailable). Weather refreshes every 30 minutes while the companion app is active.

## Layout

See [DESIGN.md](DESIGN.md) for the full spec.

## Visual Validation Notes (Emulator)

Validated against emulator screenshot from June 1, 2026 (`14:50` runtime sample):

- Date (`MON · 01 JUN`) and hero time (`14:50`) render without clipping.
- Mid horizontal rule is visible and aligns with the complication row.
- Health placeholders (`---`) display correctly when BPM/steps are unavailable.
- Bottom row (`90°F`, battery icon, `80%`) fits within bounds with visible right/bottom padding.
- No vertical rail is currently rendered in this implementation.
