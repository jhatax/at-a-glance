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
- Procedural icons for health, weather, and battery
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

The rectangular layout is computed from the display bounds instead of fixed
Emery-only coordinates. Rectangular spacing is:

```c
PBL_DISPLAY_HEIGHT / 28
```

Screen hierarchy:

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

All text columns are left-aligned. The left health and bottom columns
start at the content margin. The right health and bottom columns are
positioned by column math, but their text alignment remains left.

Current fonts:

- Date: `FONT_KEY_GOTHIC_18_BOLD`
- Time: `FONT_KEY_BITHAM_42_BOLD`
- BPM: `FONT_KEY_GOTHIC_18_BOLD`
- Steps: `FONT_KEY_GOTHIC_18_BOLD`
- Temperature: `FONT_KEY_GOTHIC_18_BOLD`
- Battery: `FONT_KEY_GOTHIC_18_BOLD`

The horizontal rule is 1 pixel wide. Value text frames for BPM, steps,
temperature, and battery use compact rectangular sizing to preserve negative
space on 144x168 displays.

## Icons

- BPM uses a procedural heart icon.
- Steps uses a procedural paw-style icon.
- Battery uses a horizontal procedural icon drawn inside a 28x28 layer.
- Battery fill moves left to right and uses the same color state as the
  battery percentage.

When BPM or steps data is unavailable on health-capable watches, the
corresponding icon uses a diagonal data-gap slash and unavailable text color.

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

## Weather

The phone companion app requests weather from Open-Meteo every 30 minutes.
It uses phone geolocation when available and falls back to bundled default
coordinates when location is unavailable.

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
  pkjs/         Phone-side weather and Clay configuration code
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
