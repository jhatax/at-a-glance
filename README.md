# At A Glance

At A Glance is a Pebble watch face inspired by cockpit instrumentation. It displays information with minimal interpretation. Time stays dominant. Icons identify. Numbers quantify. Lines show progress. Color communicates state.

The goal is a display that feels *precise, neutral, obvious, and available at a glance*.

## Screenshots

| <center>**Color rectangular**</center> | <center>**Round monochrome**</center> |
| --- | --- |
|<img src="docs/assets/screenshots/at-a-glance-clearasceleste.png" alt="At A Glance on a rectangular color Pebble in Clear as Celeste mode" width="200"><br><center>Emery: Clear as Celeste</center>|<img src="docs/assets/screenshots/at-a-glance-gabbro-whiteonblack.png" alt="At A Glance on a round Pebble in White on Black mode" width="220"><br><center>Gabbro: White on Black</center>|

## Installation

- Pebble App Store: [Update this link before publishing](PASTE_APP_STORE_LINK_HERE)

- If you've built it locally:

```sh
pebble install --phone YOUR_PHONE_IP
```

`YOUR_PHONE_IP` is the Developer Connection Server IP shown by the Pebble mobile app. If the app reports a `169.254.x.x` address, reconnect Wi-Fi, confirm Local Network permission, and restart Developer Connection until it reports a LAN address such as `192.168.x.x` or `10.x.x.x`.

## Features

- Dominant time in 12H/24H format
- Weather condition and temperature from Open-Meteo
- Date shown beside weather and temperature
- Battery status track with plugged-in bolt
- Steps, progress, and configurable daily goal (if Pebble Health is available)
- Heart rate (if Pebble Health is available)
- Display modes for monochrome and color devices
- Settings page on the connected phone

## Configuration

Personalization options:

- Time format: 24-hour or 12-hour
- Temperature unit: Fahrenheit or Celsius
- Weather updates: 15, 30, 45, or 60 minutes
- Display mode: platform-appropriate light, dark, and color palettes
- Heart rate sampling: 10, 15, 30, 60, or 120 minutes
- Steps goal: presets from `4,000` to `20,000`, with custom override support from `4,000` to `32,000`

Configuration is powered by Rebble Clay and opens from the Pebble mobile app.
Settings are persisted on the watch.

## Supported Platforms

This watch face is compatible with all Pebble smartwatch models.

- `aplite` - Pebble / Pebble Steel, black and white
- `basalt` - Pebble Time / Pebble Time Steel, color
- `diorite` - Pebble 2, black and white
- `emery` - Pebble Time 2, color
- `flint` - Pebble 2 Duo, black and white
- `chalk` - Pebble Time Round
- `gabbro` - Pebble Round 2

## Build

Prerequisites:

- [Pebble SDK](https://developer.repebble.com/sdk/)
- Node.js for PebbleKit JS dependencies

Build the PBW:

```sh
npm install
pebble build
```

## Install

Install on an emulator after building:

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

## Further Reading

- [docs/Contributing.md](docs/Contributing.md) for contributor workflow, validation, and review discipline.
- [docs/Build_and_Validation.md](docs/Build_and_Validation.md) for build, install, and validation details.
- [docs/UserInterface.md](docs/UserInterface.md) for the full visual reference implementation.

## License

[MIT](LICENSE)
