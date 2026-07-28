# Build and Install

This document is the current source of truth for *At A Glance* build, installation, and editor-tooling support.

Use this document to understand:

- build goals
- canonical build flow
- compile database generation
- install flow
- editor-tooling contract
- build-stage ownership

## Adjacent

- [Contributing](Contributing.md) for contributor flows.
- [Validation](Validation.md) for validation paths and evidence.

## Build Automation

Build tooling exists to:

1. provide one public entrypoint for build, installation, and recovery operations
1. build the watch face deterministically
2. facilitate the developer experience via `compile_commands.json`
3. install the watch face on emulators
4. install the watch face on a Pebble through Developer Connection
5. stay narrow enough that the tooling layer does not become a second project

The watch face is the product. The build layer is supporting infrastructure. It must remain direct, auditable, and sustainable.

### Core Rules

- Keep build tooling separate from watch face runtime logic.
- Do not redefine product semantics that belong in product or UI docs.
- Keep build, compile database, and install responsibilities explicit.

## Current Tooling Surface

The current build layer is made up of:

- the Pebble Tool SDK and local SDK toolchain;
- `aag-build-qa.sh` for the public build and install entrypoint;
- `tools/harness_py/ataglanceharness.py` for Python build and emulator-install dispatch;
- `tools/harness_py/pebbleadapter.py` for Pebble Tool SDK/Waf builds and libpebble2 emulator operations;
- `tools/harness_py/compilerdbgenerator.py` for optional compile-database generation;
- `build.log` and `compile_commands.json` as generated editor/build artifacts.

Use [QA README](../qa/README.md) for operating the QA harness and [QAHarnessImplementationFlow](../qa/docs/QAHarnessImplementationFlow.md) for its implementation details. This document owns the build, installation, recovery, and compile-database contract.

## Current Build Flow

### Pebble Tool SDK/Waf

Python invokes the installed Pebble Tool SDK/Waf APIs through `PebbleAdapter`. The build path does not launch a second Pebble CLI build process or run npm as part of the harness build.

```text
./aag-build-qa.sh --build
  -> ataglanceharness.py build
       -> PebbleAdapter.build()
            -> SDKProjectCommand._waf("configure")
            -> SDKProjectCommand._waf("build")
            -> configure project
            -> build platform outputs
            -> emit PBW and per-platform artifacts under build/
```

Use `npm install` separately when the project’s PebbleKit JS dependencies need to be installed.

## Current Install Flow

### Emulator Install

Supported emulators: emery | flint | chalk | gabbro | aplite | diorite | basalt

```sh
pebble build
./aag-build-qa.sh --emulators <target>
```

Automation harness:

```sh
./aag-build-qa.sh --build
./aag-build-qa.sh --emulators <target>
```

### Emulator Recovery

Use emulator recovery when the emulator stops responding, logs stall, or stale state obscures a clean install or validation run.

```sh
pebble kill --force
pebble wipe
```

Automation harness:

```text
./aag-build-qa.sh --nuclear
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
./aag-build-qa.sh -p <Developer Connection Server IP>
```
