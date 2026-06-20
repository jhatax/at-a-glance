# Cleanup Series Retrospective

## Context

This retrospective covers the cleanup thread that produced code clean-up
commits #4 through #10 and the follow-up planning around text-first rendering,
main ownership, drift prevention, and redundant state.

## What Improved

- Temperature was removed from persisted settings and made runtime-only.
- JS now sends explicit unavailable weather values instead of letting stale
  runtime weather remain visible.
- OAK fallback coordinates were named and documented as a product decision.
- Zero steps now display as `0` when step data is accessible.
- Redundant `PBL_HEALTH` checks were removed while preserving real platform
  guardrails.
- AppMessage diagnostics now report C outbox failure reasons and JS send
  failures.
- Settings tuple parsing no longer uses `atoi()` and rejects malformed strings
  before applying enum range validation.

## What Went Wrong

- The weather unavailable slice initially introduced `WEATHER_CONDITION_INVALID`
  even though the project needed `WEATHER_CONDITION_UNKNOWN`. That created
  avoidable vocabulary drift across C and JS.
- The first zero-steps patch removed the negative-value fallback instead of
  making the minimal `steps > 0` to `steps >= 0` change.
- The first health-guard audit did not sufficiently emphasize include auditing
  for every file that includes `health.h`.
- Early planning for partial refresh treated icons and text as equal sibling
  outputs, instead of recognizing the product hierarchy: text is primary,
  icons are secondary.
- The first battery/health partial-refresh plan focused too narrowly on the
  modules and did not immediately consider the parallel temp/weather pair in
  `main.c`.

## Lessons

- Measure twice before writing code means checking existing vocabulary,
  ownership, include guards, and source-of-truth state before inventing names
  or flow.
- For platform-bound headers, include auditing is part of the API change.
  `health.h` must remain guarded because it exposes `HealthEventType`.
- Avoid adding adjacent constants or variables when a correct project-level
  macro, enum, or state field already exists.
- Avoid storing redundant derived render state. Store source state and derive
  colors or icon appearance from source state when it is cheap and clear.
- Text is the core glance surface on this watchface. Icons add recognition and
  polish, but text must drive initialization and refresh policy.
- Main should become lifecycle and dispatch only. Buffers, formatting,
  rendering, and module-owned data contracts should move into cohesive modules.

## Resulting Rules

- Inspect current symbols and call paths before editing.
- Identify existing vocabulary, ownership, platform guards, and state.
- State patch shape before editing.
- Edit only after terms and branches line up.
- Build and review the diff against stated intent.
- Log layer creation failures once; do not repeatedly log refresh attempts for
  known-missing optional controls.
