# Build and Install

This document is the current source of truth for *At A Glance* build, installation, and editor-tooling support.

Use this document to understand:

- build goals
- canonical build flow
- compile database generation
- install flow
- editor-tooling contract
- build-stage ownership

**Key document relationships**

```text
`Contributing` covers the contributor flow at a high-level
|
V
`this` describes the build and install flow
|
V
`Validation` describes validation and introduces the QA harness
|
V
`qa/README` dives into executing and creating QA plans for the watch face
```

## Required Reading

- [ArchitectureLedger](ArchitectureLedger.md) for the runtime architecture.
- [Contributing](Contributing.md) for contributor flows.

## Build Automation

Build tooling has been created to:

1. serve as a wrapper for `pebble` build and install commands
1. build the watch face deterministically
2. facilitate the developer experience via `compile_commands.json`
3. install the watch face on emulators
4. install the watch face on a Pebble through Developer Connection
5. stay narrow enough that the tooling layer does not become a second project

The watch face is the product. The build layer is supporting infrastructure. It
must remain direct, auditable, and sustainable.

### Core Rules

- Keep build tooling separate from watch face runtime logic.
- Do not redefine product semantics that belong in product or UI docs.
- Keep build, compile database, and install responsibilities explicit.

## Current Tooling Surface

The current build layer is made up of:

- `pebble` CLI
- local SDK toolchain and SDK headers
- `aag-build-qa.sh` for the public build and install entrypoint
- `gen_compile_commands.py`
- `build.log`
- `compile_commands.json`

Use [QA_Readme](../qa/README.md) for QA harness implementation details.
This document owns the build contract only.

## Current Build Flow

### Native Pebble Tools
```text
npm install
  -> PebbleKit JS dependencies available

pebble build
  -> configure project
  -> build platform outputs
  -> emit PBW and per-platform artifacts under build/
```

## Current Install Flow

### Emulator Install

Supported emulators: emery | flint | chalk | gabbro | aplite | diorite | basalt

```sh
pebble build
pebble install --emulator <target>
```

Automation harness:

```sh
./aag-build-qa.sh -b
./aag-buid-qa.sh -i -e <target>
```

### Emulator Recovery

Use emulator recovery when the emulator stops responding, logs stall, or stale state obscures a clean install or validation run.

```sh
pebble kill --force
pebble wipe
```

Automation harness:

```text
./aag-build-qa.sh -nuclear
```

### Watch / device Install

Watch install uses a phone's developer connection capability to sideload the watch face.

```text
pebble build
pebble install --phone <Developer Connection Server IP>
```

Automation harness:

```text
./aag-build-qa.sh -bc
./aag-buid-qa.sh -p <Developer Connection Server IP>
```

## Further Reading

- [Validation](Validation.md) for validation scenarios, artifact expectations,
  and release evidence.
- [Contributing](Contributing.md) for contributor workflow and review
  discipline.
- [SettingsandConfiguration](SettingsandConfiguration.md) for settings,
  message-key, and transport obligations that validation must exercise.
