# Life at a Glance

Swiss-Rail Inspired watchface for **Pebble Time 2** (emery).

## Features

- Left-aligned typographic layout with vertical rail
- Time (12h or 24h) and short date (`THU · 28 MAY`)
- Heart rate and full step count from Pebble Health
- Temperature from phone (Open-Meteo) in °F or °C
- Battery percentage (bottom-right)
- Settings: time format, temperature unit, bpm sampling interval (Clay config page)

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
- **Heart rate sampling:** every 15, 30, 60, or 120 minutes

Temperature uses your phone’s location (falls back to NYC if unavailable). Weather refreshes every 30 minutes while the companion app is active.

## Layout

See [DESIGN.md](DESIGN.md) for the full spec.
