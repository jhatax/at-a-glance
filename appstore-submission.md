# RePebble App Store Submission

This document captures the current publishing path for the RePebble App
Store and the local checklist for this repo.

## Verified Entry Point

The current public developer site exposes publishing through:

- [developer.repebble.com](https://developer.repebble.com/)
- the top-level **Publish** link, which routes to the developer dashboard
  sign-in flow

The detailed upload form is login-gated, so final field names must be checked
inside the authenticated dashboard before release.

## Local Pre-Submission Checklist

1. Confirm product metadata in `package.json`:
   - `name`
   - `pebble.displayName`
   - `author`
   - `description`
   - `version`
   - `license`
   - `uuid`
   - `targetPlatforms`
   - `capabilities`
   - `messageKeys`
   - `resources`
2. Build a release PBW:
   ```sh
   pebble build
   ```
3. Confirm the generated PBW under `build/`.
4. Install and screenshot representative targets:
   - `aplite`
   - `flint`
   - `emery`
   - `chalk`
   - `gabbro`
5. Check both dark and light display modes.
6. Check unavailable climate and health states.
7. Check date/time extremes:
   - `12:59`
   - `23:59`
   - `WED 30 SEP`
8. Confirm no generated artifacts, screenshots, `.DS_Store` files, local
   IDE folders, logs, or build outputs are staged.
9. Review README for current screenshots and support claims.
10. Review `LICENSE`.

## Dashboard Submission Items To Prepare

The dashboard may request some or all of these release assets:

- PBW upload
- watchface name
- short description
- long description
- author/publisher
- version
- supported platforms
- capabilities disclosure, especially location and health
- screenshots
- source or support URL, if available
- license
- changelog/release notes

## Current Repo Notes

- The app uses Clay configuration, so `configurable` must remain in
  `package.json`.
- The phone companion uses geolocation and Open-Meteo, so `location` must
  remain in `package.json`.
- BPM and steps use Pebble Health where available, so `health` must remain in
  `package.json`.
- Do not publish temporary emulator screenshots unless they are selected as
  final store assets.
