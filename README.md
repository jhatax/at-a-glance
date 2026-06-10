# Life at a Glance

A glanceable Pebble watchface inspired by Swiss railway signage.

Life at a Glance keeps the screen focused on the essentials: date, time,
heart rate, steps, temperature, and battery. The layout is tuned for
rectangular Pebble displays and supports color and monochrome devices.

Screenshots will be added before publishing once the final color and
monochrome emulator captures are selected.

## Features

- Large hero time with a compact date row
- Heart rate and step count from Pebble Health when available
- Temperature from Open-Meteo through the phone companion app
- Battery icon and percentage
- Dark and light display modes
- Color-aware and monochrome-aware visual palette
- Procedural icons for BPM, steps, climate, and battery
- Rebble Clay configuration page

## Supported Watches

The current build targets rectangular Pebble platforms:

- `aplite` - Pebble / Pebble Steel, black and white
- `basalt` - Pebble Time / Pebble Time Steel, color
- `diorite` - Pebble 2, black and white
- `emery` - Pebble Time 2, color
- `flint` - Pebble 2 Duo, black and white

Round platforms such as Chalk and Gabbro are not supported by this layout
pass.

## Layout

The active watchface is coordinated by `src/modules/watchface.c`.
`main.c` owns Pebble lifecycle and AppMessage parsing, then hands runtime
events to `watchface`. The watchface owns one calculated `WatchfaceSurface`
from `src/modules/layout.c`.

`WatchfaceSurface` has six fixed product strata:

- date
- time
- BPM
- steps
- battery
- climate, which is weather icon plus temperature text

Rectangular layout places those six strata into this visual hierarchy:

```text
top boundary
  spacing
date row

time row
  spacing
horizontal rule
  spacing
health metrics row

bottom row
  spacing
bottom boundary
```

The rectangular layout is computed from the display bounds instead of fixed
Emery-only coordinates. Coordinates are scaled from the `200x228` Emery
design baseline with rounded integer scaling.

All text columns are left-aligned. The left health and bottom columns
start at the content margin. The right health and bottom columns are
positioned by column math, but their text alignment remains left.

### Rectangular Geometry

For Emery and other full rectangular displays, `200x228` computes to:

```text
content margin: 4
row gap: 8
icon: 28x28
icon/text gap: 4

date text:      GRect(4, 4, 192, 20)
time text:      GRect(4, 58, 192, 48)
rule:           (4, 114) -> (196, 114)
BPM icon:       GRect(4, 122, 28, 28)
BPM text:       GRect(36, 126, 40, 20)
steps icon:     GRect(124, 122, 28, 28)
steps text:     GRect(156, 126, 40, 20)
climate icon:   GRect(4, 196, 28, 28)
temperature:    GRect(36, 200, 40, 20)
battery icon:   GRect(124, 196, 28, 28)
battery text:   GRect(156, 200, 40, 20)
```

For compact rectangular displays such as Aplite, Basalt, Diorite, and
Flint, `144x168` computes to:

```text
content margin: 3
row gap: 6
icon: 20x21
icon/text gap: 3

date text:      GRect(3, 3, 138, 15)
time text:      GRect(3, 43, 138, 35)
rule:           (3, 84) -> (141, 84)
BPM icon:       GRect(3, 90, 20, 21)
BPM text:       GRect(26, 93, 29, 15)
steps icon:     GRect(89, 90, 20, 21)
steps text:     GRect(112, 93, 29, 15)
climate icon:   GRect(3, 144, 20, 21)
temperature:    GRect(26, 147, 29, 15)
battery icon:   GRect(89, 144, 20, 21)
battery text:   GRect(112, 147, 29, 15)
```

The horizontal rule is 1 pixel wide. Value text frames for BPM, steps,
temperature, and battery use compact rectangular sizing to preserve negative
space on `144x168` displays.

### Fonts

Font roles are stored in the surface per text stratum. Full rectangular
displays use:

- Date: `FONT_KEY_GOTHIC_18_BOLD`
- Time: `FONT_KEY_BITHAM_42_BOLD`
- BPM: `FONT_KEY_GOTHIC_18`
- Steps: `FONT_KEY_GOTHIC_18`
- Temperature: `FONT_KEY_GOTHIC_18`
- Battery: `FONT_KEY_GOTHIC_18_BOLD`

Compact rectangular displays use:

- Date: `FONT_KEY_GOTHIC_14_BOLD`
- Time: `FONT_KEY_BITHAM_30_BLACK`
- BPM: `FONT_KEY_GOTHIC_14`
- Steps: `FONT_KEY_GOTHIC_14`
- Temperature: `FONT_KEY_GOTHIC_14`
- Battery: `FONT_KEY_GOTHIC_14_BOLD`

## Icons

- BPM uses a procedural heart icon.
- Steps uses a procedural paw-style icon.
- Battery uses a horizontal procedural icon drawn inside a 28x28 layer.
- Battery fill moves left to right and uses the same color state as the
  battery percentage.

When BPM or steps data is unavailable on health-capable watches, the
corresponding icon uses a diagonal data-gap slash and unavailable text color.
Step count `0` is treated as a valid value when Pebble Health reports step data
as accessible. Some emulator runs can report accessible health data with `0`
steps even when the emulator has no meaningful health history; that is a known
emulator quirk and is not special-cased by the watchface.

## Colors

Dark mode:

- Background: black
- Date: lavender on color, white on monochrome
- Time: sunset orange on color, white on monochrome
- Primary text: light gray on color, white on monochrome
- Rule: light gray on color, white on monochrome
- Unavailable text: Windsor Tan on color, white on monochrome

Light mode:

- Background: white
- Date: imperial purple on color, black on monochrome
- Time: sunset orange on color, black on monochrome
- Primary text: black
- Rule: light gray on color, black on monochrome
- Unavailable text: light gray on color, black on monochrome

BPM zones:

- Unavailable or invalid: unavailable color
- `1-99`: Jaeger Green on color
- `100-120`: Magenta on color
- `>120`: Red on color
- Monochrome devices use the current primary text color for available BPM
  values.

Battery zones:

- Charging: Jaeger Green on color
- `>50%`: Cobalt Blue on color
- `21-50%`: Yellow on color
- `<=20%`: Red on color
- Monochrome devices use the current primary text color.

## Configuration

Configuration is powered by Rebble Clay:

```json
"dependencies": {
  "@rebble/clay": "^1.0.4"
}
```

The companion JavaScript initializes Clay from `src/pkjs/config.json`.

Settings:

- Time format: 24-hour or 12-hour
- Temperature unit: Fahrenheit or Celsius
- Display mode: dark or light
- Heart rate sampling: 10, 15, 30, 60, or 120 minutes

Settings persistence is best-effort Pebble persistent storage. If a
write fails, the current in-memory setting still applies for the running
watchface, but the value may not survive restart. Persistence failures
are logged for debugging rather than shown on the watchface.

## Climate Data

The phone companion app requests weather from Open-Meteo every 30 minutes.
It uses phone geolocation when available. When location is unavailable, it
falls back to OAK, the home location for this watchface:
`37.85626, -122.21383`.

Temperature is sent to the watch in Celsius tenths and rendered as Fahrenheit
or Celsius according to the selected setting.

## Build

### Prerequisites

- [Pebble SDK](https://developer.repebble.com/sdk/)
- Node.js for PebbleKit JS dependencies

### Build the PBW

```sh
npm install
pebble build
```

The build output is:

```text
build/life-at-a-glance-emery-wf.pbw
```

## Run In An Emulator

```sh
# Pebble Time 2, color, 200x228
pebble install --emulator emery

# Pebble 2 Duo, black and white, 144x168
pebble install --emulator flint

# Pebble 2, black and white, 144x168
pebble install --emulator diorite

# Original Pebble, black and white, 144x168
pebble install --emulator aplite
```

To test the config page in an emulator:

```sh
pebble emu-app-config
```

## Install On Hardware

For hardware:

```sh
pebble install --phone YOUR_PHONE_IP
```

You can also use the Pebble SDK login flow and CloudPebble install path if
that is how your local SDK is configured.

## To Do

- Add final README screenshots for color and monochrome rectangular devices.
- Run an emulator screenshot pass for Emery, Diorite, Aplite, and Flint.
- Decide whether round platforms need a separate future layout.
- Publish release PBW once the visual pass is complete.

## Project Structure

```text
resources/      Static image resources
src/
  c/            Native Pebble C watchface code
  modules/      Watchface runtime, layout surface, and feature modules
  pkjs/         Phone-side climate data and Clay configuration code
package.json    Platforms, capabilities, message keys, and resources
wscript         Pebble build entrypoint
```

## Development Notes

- `sdkVersion` remains `"3"` for the Pebble SDK workflow used by this repo.
- The watchface is designed for SDK 4+ APIs where available, with platform
  guards for optional capabilities.
- Keep runtime behavior, visual layout, and documentation changes in coherent
  commits when possible.

## Publish README Target

Before publishing, use the
[Carbon README](https://github.com/cr0ybot/carbon/blob/main/README.md) as
the structural target:

- Badges only when release and store URLs exist.
- Project title and one-sentence product description.
- Color and monochrome screenshots near the top.
- Short rationale for the watchface.
- Feature list.
- To-do/status list.
- Development prerequisites.
- Build and emulator commands.
- Project structure.
- License.

## License

[MIT](LICENSE)
