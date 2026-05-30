# Agent Instructions

This repo is for Pebble SDK 4+ watchfaces and apps. The first target is Pebble Time 2 / `emery`, but design and architecture should anticipate earlier Pebble platforms where SDK 4+ supports them.

## Working Style

- Spend most effort on architecture, design, testability, constraints, and look-and-feel before coding.
- Aim before acting: understand the goal, constraints, platform target, and likely failure modes before changing files.
- Measure twice, cut once: verify assumptions and inspect diffs before commits, publishing steps, or broad refactors.
- Use progressive disclosure: discover only the files needed, identify entry points, suggest changes, then edit.
- Do not guess Pebble behavior. Verify SDK 4+ APIs, generated resources, AppMessage keys, emulator behavior, and build output when they matter.
- Keep changes focused. Suggest cleanups and stylistic changes before making them unless they are required for the task.
- After meaningful work, critique the result: call out misses, uncertain assumptions, platform risks, verification gaps, and what could get dropped in later iterations.

## Project Facts

- Native C: `src/c`
- PebbleKit JS and Clay config: `src/pkjs`
- Metadata, target platforms, resources, capabilities, and message keys: `package.json`
- Build entry: `wscript`
- Current design reference: `DESIGN.md`
- Current product: "Life at a Glance", a Swiss-Rail-inspired glanceable watchface.

## Pebble Platform Rules

- Use only Pebble SDK 4+ APIs. If an API is platform-specific, guard it and document the fallback.
- Treat Pebble as an embedded environment: small memory, limited CPU, limited battery, small display, and platform-specific capabilities.
- Keep rendering callbacks lightweight. Do not allocate, fetch, parse JSON, or do expensive formatting in layer update procs.
- Treat health, heart rate, color, screen shape, resources, and phone data as optional by platform unless verified.
- Before broadening platform support, maintain a small platform matrix: bounds, shape, color capability, sensors, resources, and SDK constraints.
- Separate platform geometry from product logic. Prefer named constants and values derived from layer bounds over magic numbers.

## Coding Style

- Align naming, formatting, and structure with Pebble C conventions. This is important.
- Stick to standard C supported by the Pebble toolchain. Avoid unsupported extensions and desktop-only assumptions.
- Use spaces, not tabs. Remove trailing spaces.
- Use braces and new lines for all `if`, `else`, `switch`, `case`, `for`, `while`, and similar blocks, even for one statement.
- Keep variables in the narrowest practical local scope.
- Prefer readability over unnecessary optimization.
- Document succinctly: explain intent, contracts, non-obvious platform constraints, and manual test notes. Avoid comments that restate the line of code.
- Preserve fixed-size buffer discipline. Use `snprintf`; do not introduce unsafe string handling.
- Avoid heap allocation unless the Pebble API requires it. Pair allocations with clear destroy paths.
- Use `APP_LOG` sparingly. Remove or gate noisy logs before commit.

## JavaScript Bridge

- PebbleKit JS must remain compatible with the Pebble/Rebble JS runtime.
- Write readable JS with clean argument passing and semicolons.
- Use braces and new lines for all conditionals, loops, and switch cases.
- Keep AppMessage payloads compact, typed, and validated on both JS and C sides.
- AppMessage changes must update `package.json`, Clay config, JS send/receive code, C tuple handling, defaults, and docs together.
- Handle network errors, timeouts, malformed JSON, missing fields, and fallback data explicitly.

## Build Discipline

- Build with the strictest settings available for the Pebble SDK/toolchain. Do not add flags that the SDK cannot support.
- Use this command for normal builds:

```sh
pebble build
```

- If the build fails, stop and report the failure before moving toward commit or publish.

## Run And Debug

- Test with the Pebble emulator for visual or runtime changes.
- Confirm install command:

```sh
pebble install --emulator emery
```

- Keep logs visible in a shell session during emulator debugging:

```sh
pebble logs --emulator emery
```

- Use logs to debug real behavior, then remove noisy diagnostic logging before commit.

## Design And Docs

- Optimize for a one-second glance on a small watch display.
- Preserve hierarchy: date, hero time, rule, complications, bottom metrics.
- Validate visual changes against actual screen bounds, not only desktop intuition.
- Text must not clip at likely extremes such as `WED · 30 SEP`, `12:59`, `100`, `99999`, `---F`, and `100%`.
- Document entry points and user-facing behavior in `README.md`.
- Keep `README.md` simple, direct, and Markdown-native. Include screenshots when visual behavior changes.
- Avoid AI-generated slop: vague filler, overexplaining, contradiction phrases, double-dash constructions, and inflated claims.

## GitHub Publishing

- Everything here should be treated as eventually publishable to GitHub.
- New projects start in a private GitHub repo unless the user says otherwise.
- Never commit secrets, tokens, API keys, credentials, private device addresses, personal location data, signing artifacts, or machine-specific config.
- Before committing, inspect diffs, untracked files, ignored files, generated output, and docs.
- If something will not build, is wrong for GitHub, contains private data, has broken metadata, or is misleading, tell the user and fix it before commit.
- Do not publish generated build artifacts unless intentionally releasing them and the user agrees.
- This folder is not yet on GitHub. The next repo task should prep the initial private GitHub commit: build check, secret audit, `.gitignore`, SDK/platform metadata, README, screenshots if available, and a clear initial commit.

## Final Checklist

- Current target platform set is explicit.
- SDK 4+ API availability is verified or uncertainty is stated.
- Platform-specific behavior has a fallback.
- Resource filenames and generated IDs match.
- AppMessage keys match across package metadata, Clay, JS, and C.
- Defaults match across C, Clay, README, and persisted settings.
- `pebble build` was run, or the reason it could not run is reported.
- Emulator install/log verification was run for runtime or visual changes, or the gap is reported.
