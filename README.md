# At A Glance

At A Glance is built for immediate glanceability and readable context across
color and monochrome Pebbles without making the screen feel crowded. A strong
centered time display anchors the face, while restrained color and compact
supporting details keep weather, date, battery, steps, and heart rate easy to
read. The design borrows from transit signage and instrument panels, favoring
alignment, contrast, and calm hierarchy over decoration. Use settings to tune
information displayed to your needs, and share feedback as it becomes part of
your daily rhythm.

The current source targets color, black-and-white, rectangular, and round
Pebble platforms. Final App Store screenshots are still pending.

## Features

- Dominant centered time
- Battery status track with charging bolt
- Weather icon and temperature from Open-Meteo
- Date context beside weather
- BPM and steps from Pebble Health where available
- Dark and light display modes
- Color-aware and monochrome-aware palette
- Procedural battery, BPM, and weather glyphs
- Recolored bitmap walking icon for steps
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

Round targets build from the unified architect path, but final Chalk and
Gabbro screenshots are still required before publication confidence.

## Layout

The selected visual stack is shared across display families:

```text
top metric context
dominant centered time
centered battery track and charging bolt
weather/date context
bottom metric context
```

This shared stack is the product's visual DNA. It does not mean every platform
uses the same coordinates.

`src/c/ataglance.c` owns Pebble lifecycle, services, settings, AppMessage
parsing, and dispatch to `watchface`. `src/modules/watchface.c` owns the live
`WatchfaceSurface` and module lifecycle. `src/modules/layout_architect.c`
prepares final geometry, and `src/modules/layout_stylist.c` applies palette,
font, and custom-font decisions.

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
- Battery uses a primary-color outline track with module-owned state-color
  fill.
- The charging bolt is a shape cue so charging does not rely on green alone.
- Health metrics use centerline vocabulary rather than far-corner placement.
- Unavailable data uses the shared diagonal absence slash.
- Round displays preserve the same visual hierarchy but still require
  round-aware screenshot review.

See `VisualVocabulary.md` and `DESIGN.md` for the current design notes.

## Colors

Dark mode:

- Background: black
- Date: electric blue on color, white on monochrome
- Time: sunset orange on color, white on monochrome
- Primary text: Celeste on color, white on monochrome
- Unavailable text: light gray on color, white on monochrome

Light mode:

- Background: white
- Date: black
- Time: sunset orange on color, black on monochrome
- Primary text: cobalt blue on color, black on monochrome
- Unavailable text: dark gray on color, black on monochrome

Dynamic metric colors remain module-owned:

- BPM owns BPM zone colors.
- Battery owns charge state colors.
- Climate owns weather glyph colors.

## Configuration

Configuration is powered by Rebble Clay:

```json
"dependencies": {
  "@rebble/clay": "^1.0.4"
}
```

Settings:

- Time format: 24-hour or 12-hour
- Temperature unit: Fahrenheit or Celsius
- Display mode: light by default, with dark mode available
- Heart rate sampling: 10, 15, 30, 60, or 120 minutes

The watchface persists settings through Pebble persistent storage.

## Climate Data

The phone companion app requests current weather from Open-Meteo every
30 minutes. It uses phone geolocation when available. When location is
unavailable, the fixed fallback weather location is OAK:

```text
37.85626, -122.21383
```

Temperature is sent to the watch in Celsius tenths and rendered as Fahrenheit
or Celsius according to settings. Open-Meteo `weather_code` and `is_day` are
sent to C for glyph selection.

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

The harness automation is emulator-focused. The phone path only installs through
the Pebble mobile app Developer Connection server.

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
Use `project-rename-plan.md` before renaming the repo folder or package.

Before publication:

- run `pebble build`
- capture final screenshots for representative targets
- review App Store metadata
- confirm no generated artifacts or local scratch files are staged
- review `ARCHITECTURE_LEDGER.md` for remaining release-blocking TODOs

## License

[MIT](LICENSE)
