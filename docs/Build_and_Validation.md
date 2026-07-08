# Build and Validation

This document is the current source of truth for *At A Glance* build,
installation, editor-tooling support, and validation architecture.

Use this document to understand:

- build flow
- compile database generation
- install flow
- validation flow
- current tooling ownership
- future modularization boundaries

## Required Reading

- [../qa/README.md](../qa/README.md) for Build & QA harness source navigation.
- [ArchitectureLedger.md](ArchitectureLedger.md) for the runtime architecture.
- [UserInterface.md](UserInterface.md) for the full visual reference implementation.

## Build And Validation Goals

Build and validation tooling has been created to do six things well:

1. build the watch face deterministically
2. prepare contributor tooling such as `compile_commands.json`
3. install the watch face on emulators
4. install the watch face on hardware through Developer Connection
5. run repeatable validation flows
6. stay narrow enough that the tooling layer does not become a second project

The watch face is the product. The tooling layer is supporting infrastructure.
It must remain direct, auditable, and sustainable.

## Core Rules

- Prefer one coherent tooling slice at a time.
- Build and validation tooling must stay separate from watch face runtime logic.
- Tooling must not redefine product semantics that belong in product or UI docs.
- Add validation scripts when new tooling functionality is added.
- Keep QA stage boundaries explicit.

## Current Tooling Surface

The current build and validation layer is made up of:

- `pebble` CLI
- local SDK toolchain and SDK headers
- `ataglance_build_test_harness.sh`
- `qa/`
- `gen_compile_commands.py`
- `build.log`
- `compile_commands.json`
- direct emulator commands
- direct AppMessage commands for focused debugging

Use [../qa/README.md](../qa/README.md) for the current modular QA implementation map. This
document defines the build and validation contract; the QA README maps the
helper, stage, test, and data files that implement that contract.

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
  -> emit verbose runner lines into build.log
  -> compile commands become available for database extraction
```

**Notes**

- `pebble build` is the canonical build command.
- `pebble build -v` is the canonical input for compile database generation.
- The build may be incremental and therefore may not emit compile commands on
  every successful run.

## Compile Database Flow

The project uses `gen_compile_commands.py` to derive `compile_commands.json`
from a verbose Pebble build log.

### Current flow

```text
ataglance_build_test_harness.sh --build or --build-clean
  -> pebble build -v > build.log
  -> gen_compile_commands.py --log-path build.log --output compile_commands.json
       -> strip ANSI sequences
       -> extract runner [...]
       -> filter to the selected platform, currently emery by default
       -> resolve compiler path
       -> write compile_commands.json
```

### Incremental-build fallback

```text
normal verbose build
  -> if build.log contains emery compile commands
       -> write compile_commands.json
  -> else
       -> clean build
       -> regenerate build.log
       -> write compile_commands.json
```

This fallback exists because a successful incremental build may contain only
configure or capability-probe runner lines and therefore may be insufficient
for clangd.

## Rationale for `compile_commands.json`

Pebble's build system does not emit `compile_commands.json` directly. This project
generates it from the verbose build log so that contributors get:

- accurate Pebble SDK include paths
- accurate Clay include paths
- accurate target defines such as `PBL_PLATFORM_EMERY`
- accurate compiler flags for the active platform
- better `clangd` navigation and diagnostics
- a reproducible editor setup that is derived from the real build rather than a
  hand-maintained guess

## Editor Tooling Contract

The compile database exists to support editor tooling and improve developer productivity.

Current editor-tooling contract:

- `compile_commands.json` is generated from a real verbose build
- `.clangd` provides workspace-local `clangd` behavior
- `compile_flags.txt` remains as a narrow fallback/helper file
- contributors should open the repo root, not a subdirectory

If editor diagnostics look stale or incorrect, regenerate the compile database
from a fresh build rather than editing flags by hand.

## Runtime Messages And Generated Keys

The QA harness depends on the same runtime-message contract as the watch face.
Use [Settings_and_Configuration.md](Settings_and_Configuration.md) for the full settings catalog, Clay
mapping, generated message-key contract, PKJS normalization, and persistence
rules.

Use this document for how runtime messages are exercised by validation tooling.

**Important validation boundary notes**

- `qa/lib/appmessage.sh` is the canonical helper for manual numeric keys used
  by emulator QA commands.
- `STEPS_GOAL_PRESET` and `STEPS_GOAL_CUSTOM` are Clay/PKJS inputs. They do not
  cross the JS-to-C boundary as separate tuples.
- One-shot health overrides are QA-oriented runtime keys defined in
  `watchface.h`, not manifest-backed Clay settings:
  `10020` = one-shot BPM, `10021` = one-shot steps.

**Validation rule**

- JS, C, generated outputs, `qa/lib/appmessage.sh`, and manual
  `pebble send-app-message` commands must all follow the same numeric mapping.

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

Use emulator recovery when the emulator stops responding, AppMessage commands
do not appear to affect the running app, logs stall, or a stale emulator state
obscures validation.

Basic recovery:

```sh
pebble kill --force
pebble wipe
```

Harness recovery:

```sh
./ataglance_build_test_harness.sh -n
```

The harness `-n` / `--nuclear` flag kills running emulators, wipes emulator
state, and forces a clean build. It is useful when an emulator is not
responding because it resets both the emulator process and its persisted state
before rebuilding from a clean tree. Combine it with `--install` or `--test`
when recovery should continue into emulator install or validation.

### Hardware Install

Canonical phone install shape:

```text
build succeeds
  -> pebble install --phone <Developer Connection Server IP>
```

Harness-assisted phone install:

```sh
./ataglance_build_test_harness.sh -p <Developer Connection Server IP>
```

Rules:

- use the Pebble/RePebble app's Developer Connection Server IP
- reject `169.254.x.x` link-local addresses
- treat install as separate from validation sweeps unless the requested flow
  explicitly combines them

## Current Validation Flow

The current tooling surface supports four validation families:

1. weather
2. battery
3. health
4. smoke

Validation is emulator-first. Current transport split:

- weather, display mode, and one-shot health values use AppMessage
- battery state uses the emulator battery service

### Config Page Validation

```sh
pebble emu-app-config
```

### Battery Validation

Battery state is changed through the emulator battery service:

```sh
pebble emu-battery --emulator emery --percent 19
pebble emu-battery --emulator emery --percent 75 --charging
```

### Weather And Display-Mode Validation

Weather, weather cadence, and display mode are sent through AppMessage.

Weather data keys:

- `10002` = `TEMPERATURE`, for example `10002=539` sends `53.9C`
- `10003` = `WEATHER_CONDITION`
- `10004` = `IS_DAY`, where `1` is day and `0` is night

Settings keys commonly used for focused validation:

- `10005` = `WEATHER_UPDATE_MINUTES`
- `10006` = `DISPLAY_MODE`

Examples:

```sh
pebble send-app-message --emulator emery --int 10002=700 10003=95 10004=0
pebble send-app-message --emulator emery --int 10002=700 10003=95 10004=1
pebble send-app-message --emulator emery --int 10002=-32768 10003=-1 10004=0
pebble send-app-message --emulator emery --int 10005=15
pebble send-app-message --emulator emery --int 10006=0
pebble send-app-message --emulator emery --int 10006=2
```

Use the same weather code and change only `IS_DAY` when the goal is to inspect
the day/night glyph delta.

### One-Shot Health Validation

One-shot health ingress exists as QA-only overrides that affect the next health
refresh only.

Current one-shot keys:

- `10020` = one-shot BPM
- `10021` = one-shot steps

Examples:

```sh
pebble send-app-message --emulator emery --int 10020=72
pebble send-app-message --emulator emery --int 10021=8500
pebble send-app-message --emulator emery --int 10020=0
pebble send-app-message --emulator emery --int 10021=0
```

### Debug Builds

`ATAGLANCE_DEBUG` is an optional build-time gate for visual/runtime diagnosis.
It is not a product mode.

Activation point:

```text
wscript
  -> ctx.env.append_value('DEFINES', ['ATAGLANCE_DEBUG=1'])
```

To enable a debug build:

1. Open `wscript`.
2. Uncomment the `ATAGLANCE_DEBUG=1` define.
3. Run a clean build and install.
4. Validate in the emulator.
5. Re-comment the define before normal release builds unless the debug behavior
   is intentionally being inspected.

Debug builds can help validate UI changes because debug-only render
instrumentation makes layer bounds and glyph bounds easier to inspect in the
emulator.

## Validation Stage Model

Validation work should be understood as explicit stages:

1. validate request
2. optional reset
3. optional build
4. optional compile database generation
5. optional install
6. optional validation automation

This is important because not every run needs every stage.

Examples:

- editor bootstrap needs build plus compile database generation
- phone install needs build plus phone install
- weather inspection needs install plus weather automation
- smoke coverage may need build, install, and multiple automation families

## Harness Ownership

The current public entrypoint is:

- `ataglance_build_test_harness.sh`

Its current responsibilities are:

- top-level CLI
- stage sequencing
- build invocation
- compile database invocation
- install invocation
- validation invocation

This is the practical current state, not the desired long-term final shape.

## Modular Direction

Build and validation tooling uses the same modular shape as the
watch face runtime and docs.

### Governing Principle: One File, One Function, One Responsibility

At the script level:

- one file should own one operation, one stage, or one test family
- one file should not own parsing, building, install, and every validation loop
  at once
- composition should happen in a thin entrypoint, not in a growing monolith

At the directory level:

- stage orchestration belongs in one layer
- reusable command helpers belong in one layer
- test-family flows belong in one layer
- static test vectors belong in one layer

## Current Modular Layout

The current implementation shape is:

```text
qa/
  README.md
  lib/
    common.sh
    validate.sh
    pebble.sh
    appmessage.sh
  stages/
    reset.sh
    build.sh
    compile_db.sh
    install.sh
  tests/
    weather.sh
    battery.sh
    health.sh
    smoke.sh
  data/
    all_qa_vectors.sh
```

## Ownership Rules For The Target Layout

### Top-Level Harness

`ataglance_build_test_harness.sh` should eventually own only:

- public CLI
- high-level staging
- exit behavior

Supporting layers should own:

- repeated test matrices
- repeated transport-key wiring
- repeated emulator iteration logic
- detailed test-family implementations

### Shared Library Layer

`qa/lib/` should own only shared helpers:

- logging
- fatal error helpers
- validation helpers
- Pebble CLI wrappers
- typed AppMessage send helpers

Numeric AppMessage keys should not be scattered through test files once the
modular layout exists.

### Stage Layer

`qa/stages/` should own one stage per file.

Examples:

- `build.sh` builds and stops
- `compile_db.sh` generates the compile database and stops
- `install.sh` owns emulator and phone install stages and stops

### Test Layer

`qa/tests/` should own one test family per file.

Examples:

- `weather.sh` owns weather sweeps
- `battery.sh` owns battery sweeps
- `health.sh` owns BPM/steps sweeps
- `smoke.sh` owns composed smoke coverage

`smoke.sh` should compose narrower helpers. It should not become a second
monolith.

### Data Layer

`qa/data/` should own static vectors only.

Examples:

- display modes
- weather codes
- weather day-state values
- battery levels and charging states
- BPM and steps override samples

Static validation vectors should not be duplicated across files.

## Stable Boundaries

### Build Stage

Owns:

- `pebble clean`
- `pebble build`
- `pebble build -v`
- `build.log` creation
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

### Validation Stage

Owns:

- repeatable emulator-driven checks
- transport payload sequences
- battery state sweeps
- smoke composition across multiple dimensions

## Refactor Triggers

Refactor the current harness only when at least one of these becomes true:

- the top-level file is no longer readable in one pass
- a test-family change risks unrelated flows
- shared logic is duplicated across multiple branches
- transport details are spread across too many locations
- contributors need to run one stage independently but cannot do so safely

## Migration Strategy

When modular refactoring begins, follow this order:

1. freeze current public behavior
2. document current stages and test families
3. extract shared validation helpers
4. extract build and compile database stages
5. extract install stages
6. extract one test family at a time
7. leave `smoke` for last because it composes others
8. shrink the top-level harness only after extracted helpers are stable

## Review Checklist

Before accepting a build/validation refactor:

- does each file have one clear responsibility?
- does the top-level entrypoint remain thin?
- are build, compile database, install, and validation stages still explicit?
- are transport keys centralized?
- are static validation vectors centralized?
- can one test family change without reopening unrelated flows?
- does the refactor preserve the working contributor path?
- is the result easier to delegate than the monolith it replaces?

## Further Reading

- [Contributing.md](Contributing.md) for contributor workflow, validation, and review discipline.
- [Settings_and_Configuration.md](Settings_and_Configuration.md) for everything related to Settings, from Clay-config to message-keys and transport
- [ProductInvariants.md](ProductInvariants.md) states invariants to achieve goals across devices.
