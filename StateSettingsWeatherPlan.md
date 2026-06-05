# State, Settings, And Weather Fallback Plan

This plan covers state clarity, persisted-settings tolerance, and phone
weather fallback work after the glyph and round-layout planning passes.

## Current Findings

- `main.c` still owns mixed state: text buffers, fonts, layer handles,
  settings, layout, palette, AppMessage receive flow, and service
  lifecycle.
- `settings_load()` previously read the full persisted settings struct
  without checking the stored size or recording a storage version.
- PebbleKit JS falls back to fixed coordinates when geolocation fails.
- TimeStyle uses explicit stored settings, derived runtime settings,
  versioned persistence, and phone-side `localStorage` for weather
  source and location details.
- Pebble/Rebble docs support saving a settings struct for startup speed,
  but warn that struct layout changes require versioning and migration.

## Available Actions

1. State clarity.
   - Low-risk: group existing `main.c` globals into explicit
     module-local state buckets.
   - Medium-risk: extract AppMessage handling into a messaging module.
   - Higher-risk: extract temperature state/display into its own module.
   - Avoid: one giant app-state struct that hides ownership without
     reducing coupling.

2. Settings tolerance.
   - Low-risk: default-fill settings, read only the persisted byte
     count, then sanitize fields.
   - Medium-risk: add a persisted storage-version key for future
     migrations.
   - Higher-risk: introduce legacy structs and migration branches when a
     real incompatible settings shape exists.

3. Weather fallback.
   - Low-risk: cache successful GPS coordinates in PebbleKit JS and use
     the cache before fixed fallback coordinates.
   - Medium-risk: add Clay settings for manual latitude/longitude or
     city.
   - Higher-risk: support multiple weather providers or user API keys.
   - Product decision: whether fixed fallback coordinates should remain
     after a cached or configured location exists.

## Prepared Patch

This first patch should stay small:

- Group `main.c` static state into:
  - `WatchfaceTextState`
  - `WatchfaceFontState`
  - `WatchfaceLayerState`
  - `WatchfaceRuntimeState`
- Update `settings_load()` to:
  - apply defaults first
  - read only the persisted byte count
  - tolerate shorter or longer persisted blobs
  - sanitize enum fields after loading
  - write a storage-version key on save
- Update PebbleKit JS weather fallback to:
  - cache successful geolocation coordinates
  - use cached coordinates when geolocation fails
  - fall back to the existing fixed coordinates only as the last resort

This patch does not change AppMessage keys, Clay config, target
platforms, or the weather API provider.

## Recommended Next Patch

After review, the next patch should extract either:

1. AppMessage receive handling into a small messaging module, or
2. temperature formatting/display into a temperature module.

The temperature module is the cleaner next candidate because it owns one
visible value, one text buffer, one setting-derived format decision, and
one weather-availability signal.
