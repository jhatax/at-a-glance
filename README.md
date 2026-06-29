# At A Glance

Inspired by cockpit instrumentation, At A Glance presents information with minimal interpretation. Time is always dominant. Icons identify. Numbers provide values. Lines show progress. Color communicates state.

Share your feedback. I hope you find the display to be *Precise, Neutral, Obvious, and Available At a Glance.*

The current source targets `aplite`, `basalt`, `diorite`, `emery`, `flint`,
`chalk`, and `gabbro` through one watchface runtime with shape-aware layout and
platform-aware styling.

## Features

- Dominant time
- Must-have data-points: Time, Weather, Date (*Aplite* treated as 1st class devices)
- Battery status track with charging bolt
- Weather icon and temperature from Open-Meteo
- Date context beside weather
- BPM and steps from Pebble Health where available
- Steps progress bar and configurable daily goal
- Four display modes: `Black on White`, `White on Black`, `Clear as Celeste`,
  and `Duke Blue Moon`
- Color- and monochrome-aware palettes
- Familiar and obvious battery, heart-rate, and weather glyphs
- Recolored bitmap walking and heart-rate icons
- Rebble Clay configuration page

## Supported Watches

`package.json` currently targets:

- `aplite` - Pebble / Pebble Steel, black and white
- `basalt` - Pebble Time / Pebble Time Steel, color
- `diorite` - Pebble 2, black and white
- `emery` - Pebble Time 2, color
- `flint` - Pebble 2 Duo, black and white
- `chalk` - Pebble Time Round
- `gabbro` - Pebble Round 2

Round targets ship through the same runtime boundary and prepared-surface
contract as rectangular targets. Geometry differs by platform; hierarchy does
not.

## Layout

The selected visual stack is shared across display families:

```text
top heart-rate context
dominant centered time
centered battery track and charging bolt
weather/date context
bottom steps context
```

This shared stack and orange centered time represent the product's visual DNA. Layout and
style have been customized for each display's geometry while retaining core visual identity.

`src/c/ataglance.c` owns Pebble lifecycle, services, persistent settings,
AppMessage parsing, and transport dispatch. `src/modules/watchface_runtime_boundary.c`
interprets runtime events, mutates settings/domain state, and decides repaint
versus refresh. `src/modules/watchface.c` owns the live `WatchfaceSurface`,
feature-module lifecycle, and visual dispatch. `src/modules/layout_architect.c`
prepares geometry, and `src/modules/layout_stylist.c` resolves palette and font
selection.

Core layout invariant:

- the architect resolves compact/full once
- the result is stored on `WatchfaceSurfaceStyle.is_compact`
- the stylist consumes that state
- downstream modules use prepared substrata and do not rederive layout policy
- text widths and heights are per-field decisions dictated by font selection,
  field role, row ownership, and platform geometry
- there is no uniform text-width invariant across all fields

Detailed UI reference, including computed geometry, font selections, palettes,
and screenshot slots, lives in [`User Interface.md`](User%20Interface.md).

## Visual Vocabulary

- Time is the dominant visual object.
- Text is primary; icons support recognition.
- Battery uses a primary-color outline track with module-owned state-color fill.
- The charging bolt is a shape cue so charging does not rely on green alone.
- Health metrics use centerline vocabulary rather than far-corner placement.
- Unavailable data uses the shared diagonal absence slash.
- Round displays preserve the same visual hierarchy with tailored accommodations.

See `VisualVocabulary.md`, `DESIGN.md`, and `ARCHITECTURE_LEDGER.md` for current design
and boundary contracts.

## Display Modes

Monochrome: Use black, white primarily, sparing use of dithered gray.
- `Black on White`: white background, black primary text, black date, black
  time, black unavailable
- `White on Black`: black background, white primary text, white date, white
  time, white unavailable

Color: Customized palette to maximize information capture; uniform status-indicating
colors for all metrics.
- `Black on White`: white background, black primary text, black date, black
  time, black unavailable, colors for battery status, steps progress, heart-rate
- `White on Black`: black background, white primary text, white date, white
  time, white unavailable
- `Clear as Celeste`: Celeste background, black primary text, Duke Blue date,
  orange time, dark gray unavailable
- `Duke Blue Moon`: Duke Blue background, white primary text, Celeste date,
  orange time, light gray unavailable

Dynamic metric colors remain module-owned. Status-indicating colors are consistent across all modules:

- Battery owns charge-state fill colors.
- BPM owns zone colors.
- Steps owns approaching-goal and achieved colors.
- Climate owns weather glyph colors.
  - Weather glyphs are outline vs. filled to account for limited display affordances on monochrome devices

## Configuration

Configuration is powered by Rebble Clay:

```json
"dependencies": {
  "@rebble/clay": "^1.0.10"
}
```

Settings:

- Time format: 24-hour or 12-hour
- Temperature unit: Fahrenheit or Celsius
- Display mode: one of four mode-specific palettes, depending on platform
- Heart rate sampling: 10, 15, 30, 60, or 120 minutes
- Steps goal: preset values from `4,000` to `20,000`, with custom override
  support from `4,000` to `32,000`

The watchface persists settings through Pebble persistent storage.

## Climate Data

The phone companion app requests current weather from Open-Meteo every
30 minutes. It uses phone geolocation when available. When location is
unavailable, the fixed fallback weather location is Oakland, CA:

```text
37.85626, -122.21383
```

Temperature is sent to the watch in Celsius tenths and rendered as Fahrenheit
or Celsius according to settings. Open-Meteo `weather_code` and `is_day` are
sent to C for glyph selection. Runtime transport completeness is evaluated
before climate state is applied; incomplete or invalid weather updates clear
stale weather state and render the unavailable vocabulary.

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

## Run In An Emulator

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

## Install On Hardware

```sh
pebble install --phone YOUR_PHONE_IP
```

`YOUR_PHONE_IP` is the Developer Connection Server IP shown by the Pebble mobile
app. A `169.254.x.x` address is link-local; reconnect Wi-Fi, confirm Local
Network permission, and restart Developer Connection until the app reports a LAN
address such as `192.168.x.x` or `10.x.x.x`.

## Build/Test Harness

`ataglance_build_test_harness.sh` wraps the local Pebble SDK flow:

```sh
./ataglance_build_test_harness.sh --build
./ataglance_build_test_harness.sh --install --emulators emery,chalk
./ataglance_build_test_harness.sh --phone YOUR_PHONE_IP
./ataglance_build_test_harness.sh --test weather,battery --emulators emery
```

The harness automation is emulator-focused. The phone path only installs
through the Pebble mobile app Developer Connection server.

## Project Structure

```text
resources/      Static image and font resources
src/c/          Pebble lifecycle entrypoint and app-level constants
src/modules/    Watchface runtime, layout surface, renderer, and modules
src/pkjs/       Phone-side climate data and Clay configuration
package.json    Platforms, capabilities, message keys, and resources
wscript         Pebble build entrypoint
```

## Publication

Use `appstore-submission.md` for the RePebble App Store submission checklist.
Use `archive/project-rename-plan.md` only if the package/repo rename work is
re-opened.

Before publication:

- run `pebble build`
- review App Store metadata
- confirm no generated artifacts or local scratch files are staged

## License

[MIT](LICENSE)
