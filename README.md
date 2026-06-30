# At A Glance

## What It Is

At A Glance is a Pebble watch face inspired by cockpit instrumentation. It displays information with minimal interpretation. Time stays dominant. Icons identify. Numbers provide values. Lines show progress. Color communicates state.

Share your feedback. The goal is a display that feels *precise, neutral, obvious, and available at a glance*.

## Install

Install on an emulator:

```sh
# Pebble Time 2, color, 200x228
pebble install --emulator emery

# Pebble 2 Duo, black and white, 144x168
pebble install --emulator flint

# Pebble Time Round, 180x180
pebble install --emulator chalk

# Pebble Round 2, 260x260
pebble install --emulator gabbro
```

To test the config page in an emulator:

```sh
pebble emu-app-config
```

Install on hardware:

```sh
pebble install --phone YOUR_PHONE_IP
```

- Pebble App Store: [Update this link before publishing](PASTE_APP_STORE_LINK_HERE)

`YOUR_PHONE_IP` is the Developer Connection Server IP shown by the Pebble mobile app. If the app reports a `169.254.x.x` address, reconnect Wi-Fi, confirm Local Network permission, and restart Developer Connection until it reports a LAN address such as `192.168.x.x` or `10.x.x.x`.

## Features

- Dominant time in 12H/24H format, configurable via Settings
- Must-have data points: time, weather, and date
- Battery status track with charging bolt
- Weather icon and temperature from Open-Meteo
- Date beside weather
- BPM and steps from Pebble Health where available
- Steps progress bar and configurable daily goal
- Two `monochrome` display modes: `Black on White`, `White on Black`
- Four `color` display modes: `Black on White`, `White on Black`, `Clear as Celeste`, and `Duke Blue Moon`
- Familiar battery, heart-rate, and weather glyphs, walking and heart-rate icons
- Settings page to personalize watch face on connected phone

### Health activity and goals

#### Steps

- Daily sum displayed with footprints icon
- Steps goal configured via Settings; out of the box goal is 10,000 steps
- Progress towards goal indicated by progress bar and color

#### Steps

- Current heart-rate displayed with heart icon
- Time between updates configured via Settings
- Heart-rate changes conveyed via icon color

### Climate: Temperature and Weather Condition based on Location

- Weather data retrieved from Open-Meteo every 30-minutes using phone geolocation, when available
- When unavailable, the fixed fallback weather location is Oakland, CA: `37.85626, -122.21383`
- Temperature rendered as Fahrenheit or Celsius according to Settings

## Supported Platforms

This watchface is compatible with all Pebble smartwatch models.

- `aplite` - Pebble / Pebble Steel, black and white
- `basalt` - Pebble Time / Pebble Time Steel, color
- `diorite` - Pebble 2, black and white
- `emery` - Pebble Time 2, color
- `flint` - Pebble 2 Duo, black and white
- `chalk` - Pebble Time Round
- `gabbro` - Pebble Round 2

## How information is displayed

The selected visual stack is shared across display families:

```text
bottom steps context
dominant centered time
centered battery track and charging bolt
weather/date context
top heart-rate context
```

This shared stack and dominant time are the product's visual DNA. Layout and style are adjusted for each display shape (round, rectangular, compact) while keeping the same information hierarchy and glanceability. Detailed geometry, font, palette, and screenshot references live in `UserInterface.md`.

Text is primary. Icons support recognition. The battery track uses a primary-color outline with a state-colored fill. The charging bolt is a shape cue, so charging does not rely on color alone. Health metrics use centerline vocabulary, and unavailable data uses a clear diagonal absence slash through the supporting icon.

### Display Modes

On monochrome devices, two display modes are available:

1. `Black on White`: white background, black primary text, black date, black time, black unavailable
2. `White on Black`: black background, white primary text, white date, white time, white unavailable

On color devices, four display modes are available:

1. `Black on White`: White background, Black primary text, Black date, Black time, Black unavailable, colors for battery status, steps progress, and heart-rate
2. `White on Black`: Black background, White primary text, White date, White time, White unavailable
3. `Clear as Celeste`: Celeste background, Black primary text, Oxford Blue date, Orange time, Dark Gray unavailable
4. `Night in Oxford`: Oxford Blue background, White primary text, Celeste date, Orange time, Light Gray unavailable

## Configuration

Configuration is powered by Rebble Clay:

```json
"dependencies": {
  "@rebble/clay": "^1.0.10"
}
```

### Personalization options (Settings)

- Time format: 24-hour or 12-hour
- Temperature unit: Fahrenheit or Celsius
- Display mode: one of four mode-specific palettes, depending on platform
- Heart rate sampling: 10, 15, 30, 60, or 120 minutes
- Steps goal: preset values from `4,000` to `20,000`, with custom override support from `4,000` to `32,000`

The watchface persists settings using Pebble's persistent storage.

## Build

### Prerequisites

- [Pebble SDK](https://developer.repebble.com/sdk/)
- Node.js for PebbleKit JS dependencies

### Build The PBW

```sh
npm install
pebble build
```

Confirm the generated PBW under `build/` before submission.

## Further Reading

- `./ataglance_build_test_harness.sh` wraps the local Pebble SDK flow for build, install, and emulator-led validation.
In the `./docs` folder:
- `Design.md` for product intent, influences, and glance-first design goals
- `ProductInvariants.md` for the product truths that should remain stable
- `VisualVocabulary.md` for the visual rules that express those truths
- `UserInterface.md` for current geometry, fonts, palettes, and screenshot status
- `ArchitectureLedger.md` for runtime architecture, boundaries, and ownership
- `Contributing.md` for contributor workflow, validation, and review discipline

Additional project references:

- `appstore-submission.md` for the RePebble App Store submission checklist
- `archive/project-rename-plan.md` only if package or repo rename work is reopened

## License

[MIT](LICENSE)
