# Build

This document is the current source of truth for *At A Glance* build,
installation, and editor-tooling support.

Use this document to understand:

- build goals
- canonical build flow
- compile database generation
- install flow
- editor-tooling contract
- build-stage ownership

## Required Reading

- [Validation.md](Validation.md) for validation scenarios, artifact expectations,
  and release evidence.
- [ArchitectureLedger.md](ArchitectureLedger.md) for the runtime architecture.
- [../qa/README.md](../qa/README.md) for QA System implementation details.

## Build Goals

Build tooling has been created to do five things well:

1. build the watch face deterministically
2. prepare contributor tooling such as `compile_commands.json`
3. install the watch face on emulators
4. install the watch face on hardware through Developer Connection
5. stay narrow enough that the tooling layer does not become a second project

The watch face is the product. The build layer is supporting infrastructure. It
must remain direct, auditable, and sustainable.

## Core Rules

- Prefer one coherent tooling slice at a time.
- Keep build tooling separate from watch face runtime logic.
- Do not redefine product semantics that belong in product or UI docs.
- Keep build, compile database, and install responsibilities explicit.

## Current Tooling Surface

The current build layer is made up of:

- `pebble` CLI
- local SDK toolchain and SDK headers
- `aag-build-qa.sh`
- `qa/stages/build.sh`
- `qa/stages/compile_db.sh`
- `qa/stages/install.sh`
- `gen_compile_commands.py`
- per-run verbose build logs under `qa/qa-runs/<run-id>/logs/build.log`
- `compile_commands.json`

Use [../qa/README.md](../qa/README.md) for QA System implementation details.
This document owns the build contract only.

## Current Build Flow

### Canonical build flow

```text
npm install
  -> PebbleKit JS dependencies available

pebble build
  -> configure project
  -> build platform outputs
  -> emit PBW and per-platform artifacts under build/
```

### Verbose build flow for tooling

```text
pebble build -v
  -> configure project
  -> emit verbose runner lines into qa/qa-runs/<run-id>/logs/build.log
  -> compile commands become available for database extraction
```

Notes:

- `pebble build` is the canonical build command.
- `pebble build -v` is the canonical input for compile database generation.
- The build may be incremental and therefore may not emit compile commands on
  every successful run.

## Compile Database Flow

The project uses `gen_compile_commands.py` to derive `compile_commands.json`
from a verbose Pebble build log.

### Current flow

```text
aag-build-qa.sh --build or --build-clean
  -> pebble build -v > qa/qa-runs/<run-id>/logs/build.log
  -> gen_compile_commands.py --log-path qa/qa-runs/<run-id>/logs/build.log --output compile_commands.json
       -> strip ANSI sequences
       -> extract runner lines
       -> filter to the selected platform, currently emery by default
       -> resolve compiler path
       -> write compile_commands.json
```

### Incremental-build fallback

```text
normal verbose build
  -> if the current run's build log contains emery compile commands
       -> write compile_commands.json
  -> else
       -> clean build
       -> regenerate the current run's build log
       -> write compile_commands.json
```

This fallback exists because a successful incremental build may contain only
configure or capability-probe runner lines and therefore may be insufficient
for `clangd`.

## Rationale for `compile_commands.json`

Pebble's build system does not emit `compile_commands.json` directly. This
project generates it from the current run's verbose build log so that
contributors get:

- accurate Pebble SDK include paths
- accurate Clay include paths
- accurate target defines such as `PBL_PLATFORM_EMERY`
- accurate compiler flags for the active platform
- better `clangd` navigation and diagnostics
- a reproducible editor setup derived from the real build instead of a
  hand-maintained guess

## Editor Tooling Contract

Current editor-tooling contract:

- `compile_commands.json` is generated from a real verbose build
- `.clangd` provides workspace-local `clangd` behavior
- `compile_flags.txt` remains as a narrow fallback/helper file
- contributors should open the repo root, not a subdirectory

If editor diagnostics look stale or incorrect, regenerate the compile database
from a fresh build rather than editing flags by hand.

## Current Install Flow

### Emulator Install

Canonical install shape:

```text
build succeeds
  -> pebble install --emulator <target>
```

Current supported local targets are:

```text
emery
flint
chalk
gabbro
aplite
diorite
basalt
```

### Emulator Recovery

Use emulator recovery when the emulator stops responding, logs stall, or stale
state obscures a clean install or validation run.

Basic recovery:

```sh
pebble kill --force
pebble wipe
```

Harness recovery:

```sh
./aag-build-qa.sh --nuclear
```

The harness `--nuclear` flag kills running emulators, wipes emulator
state, runs `pebble clean`, forces `pebble build -v`, and then regenerates
`compile_commands.json` through the normal compile-database stage. Combine it
with `--install` when recovery should continue into install.

### Hardware Install

Canonical phone install shape:

```text
build succeeds
  -> pebble install --phone <Developer Connection Server IP>
```

Harness-assisted phone install:

```sh
./aag-build-qa.sh -p <Developer Connection Server IP>
```

Rules:

- use the Pebble/RePebble app's Developer Connection Server IP
- reject `169.254.x.x` link-local addresses
- treat install as separate from validation sweeps unless the requested flow
  explicitly combines them

## Build Stage Ownership

The current public entrypoint is:

- `aag-build-qa.sh`

Build responsibilities currently owned by the build layer are:

- top-level build invocation
- compile database generation
- emulator install
- hardware install
- build failure handling

Implementation details live in `qa/stages/` and are mapped in
[../qa/README.md](../qa/README.md).

## Stable Build Boundaries

### Build Stage

Owns:

- `pebble clean`
- `pebble build`
- `pebble build -v`
- per-run verbose build-log creation under `qa/qa-runs/<run-id>/logs/`
- build success/failure handling

### Compile Database Stage

Owns:

- parsing verbose build logs
- ANSI stripping
- compile-command extraction
- target filtering
- compiler-path resolution
- `compile_commands.json` output

### Install Stage

Owns:

- emulator installs
- phone installs through Developer Connection
- validation of target names and phone IPs

## Further Reading

- [Validation.md](Validation.md) for validation scenarios, artifact expectations,
  and release evidence.
- [Contributing.md](Contributing.md) for contributor workflow and review
  discipline.
- [Settings_and_Configuration.md](Settings_and_Configuration.md) for settings,
  message-key, and transport obligations that validation must exercise.
