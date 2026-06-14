# Life at a Glance

A glanceable Pebble watchface inspired by Swiss railway signage.

Life at a Glance keeps the screen focused on the essentials: date, time,
heart rate, steps, temperature, and battery. The current shipped build is
tuned for rectangular Pebble displays and supports color and monochrome
devices.

Screenshots will be added before publishing once the final color and
monochrome emulator captures are selected.

## Features

- Large hero time with a compact date row
- Heart rate and step count from Pebble Health when available
- Temperature from Open-Meteo through the phone companion app
- Rectangular battery track with charging bolt
- Dark and light display modes
- Color-aware and monochrome-aware visual palette
- Procedural climate, BPM, and battery glyphs
- Recolored walking bitmap for steps
- Rebble Clay configuration page

## Supported Watches

The current build targets rectangular Pebble platforms:

- `aplite` - Pebble / Pebble Steel, black and white
- `basalt` - Pebble Time / Pebble Time Steel, color
- `diorite` - Pebble 2, black and white
- `emery` - Pebble Time 2, color
- `flint` - Pebble 2 Duo, black and white

Round layout code exists in `src/modules/layout_round.c`, but `chalk` and
`gabbro` are not currently enabled in `package.json`.

## Layout

The active watchface is coordinated by `src/modules/watchface.c`.
`main.c` owns Pebble lifecycle and AppMessage parsing, then hands runtime
events to `watchface`. The watchface owns one calculated `WatchfaceSurface`
initialized through the shape-specific layout implementation declared in
`src/modules/layout.h`.

`WatchfaceSurface` has six fixed product strata:

- date
- time
- BPM
- steps
- battery
- climate, which is weather icon plus temperature text

The current rectangular layout places those six strata into this visual
hierarchy:

```text
top margin
dominant time
centered battery track and bolt
weather icon + temperature      right-aligned date
steps icon + text               bpm icon + text
```

The rectangular layout is computed from design decisions and display bounds,
not from fixed Emery-only coordinates. Full rectangular displays use the
reference blueprint directly. Compact rectangular displays use a compact
blueprint rather than passive scaling of every metric.

Text columns remain left-aligned inside their own frames. Time is centered.
Date is right-aligned in the space after the weather stratum.

### Rectangular Geometry

For Emery and other full rectangular displays, `200x228` computes to:

```text
content margin: 7
icon: 28x28
icon/text gap: 2

time text:        GRect(7, 45, 186, 54)
battery track:    GRect(50, 104, 100, 8)
battery fill:     GRect(51, 105, 98, 6)
battery bolt:     GRect(152, 100, 16, 16)
climate icon:     GRect(7, 117, 28, 28)
temperature text: GRect(37, 121, 40, 20)
date text:        GRect(79, 117, 114, 28)
steps icon:       GRect(7, 193, 28, 28)
steps text:       GRect(37, 197, 40, 20)
bpm icon:         GRect(123, 193, 28, 28)
bpm text:         GRect(153, 197, 40, 20)
```

For compact rectangular displays such as Aplite, Basalt, Diorite, and
Flint, `144x168` computes to:

```text
content margin: 7
icon: 20x20
icon/text gap: 2

time text:        GRect(7, 33, 130, 42)
battery track:    GRect(36, 80, 72, 8)
battery fill:     GRect(37, 81, 70, 6)
battery bolt:     GRect(110, 76, 16, 16)
climate icon:     GRect(7, 93, 20, 20)
temperature text: GRect(29, 95, 40, 16)
date text:        GRect(71, 93, 66, 20)
steps icon:       GRect(7, 141, 20, 20)
steps text:       GRect(29, 143, 40, 16)
bpm icon:         GRect(84, 141, 20, 20)
bpm text:         GRect(106, 143, 33, 16)
```

The rectangular battery is a filled track plus a charging bolt, not a text
percentage stratum. Battery text remains part of the round layout only.

### Fonts

Font roles are stored in the surface per text stratum. Full rectangular
displays use:

- Date: `FONT_KEY_GOTHIC_24_BOLD`
- Time: `FONT_KEY_LECO_42_NUMBERS` on most full rectangular targets
- Time: `FONT_KEY_ROBOTO_BOLD_SUBSET_49` on `emery`
- BPM: `FONT_KEY_GOTHIC_18`
- Steps: `FONT_KEY_GOTHIC_18`
- Temperature: `FONT_KEY_GOTHIC_18`
- Battery: not applicable on rectangular layout; battery uses a track

Compact rectangular displays use:

- Date: `FONT_KEY_GOTHIC_14_BOLD`
- Time: `FONT_KEY_BITHAM_34_MEDIUM_NUMBERS`
- BPM: `FONT_KEY_GOTHIC_14`
- Steps: `FONT_KEY_GOTHIC_14`
- Temperature: `FONT_KEY_GOTHIC_14`
- Battery: not applicable on rectangular layout; battery uses a track

## Icons

- BPM uses a procedural boxed waveform icon.
- Steps uses a recolored walking bitmap resource.
- Climate uses procedural condition glyphs.
- Rectangular battery uses a centered track and charging bolt layer.
- Round battery continues to use icon-plus-text rendering.

When BPM or steps data is unavailable on health-capable watches, the
corresponding icon uses a diagonal data-gap slash and unavailable text color.
Step count `0` is treated as a valid value when Pebble Health reports step data
as accessible. Some emulator runs can report accessible health data with `0`
steps even when the emulator has no meaningful health history; that is a known
emulator quirk and is not special-cased by the watchface.

## Colors

Dark mode:

- Background: black
- Date: electric blue on color, white on monochrome
- Time: sunset orange on color, white on monochrome
- Primary text: Celeste on color, white on monochrome
- Rule: white
- Unavailable text: light gray on color, white on monochrome

Light mode:

- Background: white
- Date: black
- Time: sunset orange on color, black on monochrome
- Primary text: cobalt blue on color, black on monochrome
- Rule: oxford blue on color, black on monochrome
- Unavailable text: dark gray on color, black on monochrome

Color choices prefer high-contrast colors on dark backgrounds and darker,
high-light-absorption colors on light backgrounds.

BPM zones:

- Unavailable or invalid: unavailable color
- `1-99`: current mode primary text color
- `100-120`: chrome yellow on dark, Windsor Tan on light
- `>120`: orange on dark, Bulgarian Rose on light
- Monochrome devices use the current primary text color for available BPM
  values.

Battery zones:

- Charging: Islamic Green on color, primary text on monochrome
- `>50%`: current mode primary text color
- `21-50%`: Rajah on dark, Windsor Tan on light
- `<=20%`: red on dark, Bulgarian Rose on light
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
Open-Meteo `is_day` is also sent so clear conditions can render as a sun by
day and a clear-sky icon by night.

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
- Finish the screenshot-led round design pass before enabling round targets.
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
