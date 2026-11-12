# Build and Install

Use this document to build _At A Glance_, install it on an emulator or watch, generate editor compiler data, and prepare the upload artifact.

## Adjacent

- [Contributing](Contributing.md) for contributor flows.
- [Validation](Validation.md) for validation paths and evidence.

## Build Automation

`pebble build` builds the watchface directly. `./aag-build-qa.sh` provides build, installation, and recovery operations, with optional verbose output and compile-database generation. Both use the Pebble SDK toolchain.

## Current Tooling Surface

Build responsibilities:

- Pebble Tool and the SDK: Compiler, Waf, and platform tooling.
- `aag-build-qa.sh`: Public build, install, and recovery commands.
- `tools/harness_py/ataglanceharness.py`: Python command dispatch.
- `tools/harness_py/pebbleadapter.py`: Waf builds and libpebble2 emulator operations.
- `build.log` and `compile_commands.json`: Optional verbose-build artifacts.

Use [QA README](../qa/README.md) to run plans and inspect results. [QAHarnessImplementationFlow](../qa/docs/QAHarnessImplementationFlow.md) maps the QA execution code.

## Current Build Flow

### Pebble Tool SDK/Waf

`PebbleAdapter` invokes Pebble Tool's Waf runner. The repository's compile-database helper extracts compiler commands from the verbose build log.

```text
./aag-build-qa.sh --build
  -> ataglanceharness.py build
       -> PebbleAdapter.build()
            -> Pebble Tool SDKProjectCommand._waf()
            -> configure project
            -> build platform outputs
            -> emit PBW and per-platform artifacts under build/
            -> create a -publish.pbw copy without the JavaScript source map
            -> compilerdbgenerator.generate_compile_database() when verbose
                 -> extract Emery compiler commands from build.log
                 -> write compile_commands.json at the project root
```

Install JavaScript dependencies with `npm install` before building. The harness invokes Waf configure and build directly.

For clangd, use a clean verbose build so the log includes compiler commands:

```sh
./aag-build-qa.sh --build-clean
```

`--build-verbose` captures an incremental build. If no source compiles, the log may contain no entries for `compile_commands.json`. Compile-database generation does not determine build success.

Verbose build behavior:

- Output: Capture the complete build output.
- Failure log: Append the Python traceback to `build.log`.
- Operator output: Show only extracted compiler diagnostics in the console and QA log.
- Non-verbose builds: Do not create `build.log`; use the harness failure path without exposing a Python traceback.

## Publishing

Run the normal build to produce both PBW files:

```sh
./aag-build-qa.sh --build
```

- `build/at-a-glance.pbw` contains the JavaScript bundle and source map for local development.
- `build/at-a-glance-publish.pbw` is the upload artifact. It excludes only `pebble-js-app.js.map`; every other archive entry matches the original PBW.

`wscript` creates the publish copy after the original PBW is built. The local `build/pebble-js-app.js.map` remains available to `pebble logs`.

Complete [release validation](Validation.md#release-validation), then upload `build/at-a-glance-publish.pbw` to the app store.

## Current Install Flow

### Emulator Install

Supported emulators: emery | flint | chalk | gabbro | aplite | diorite | basalt

The examples use `emery`; substitute the target you want to install or recover.

```sh
pebble build
./aag-build-qa.sh --install emery
```

Automation harness:

```sh
./aag-build-qa.sh --build
./aag-build-qa.sh --install emery
```

### Emulator Recovery

Use emulator recovery when the emulator stops responding, logs stall, or stale state obscures a clean install or validation run.

```sh
pebble kill --force
pebble wipe
```

Automation harness:

```sh
./aag-build-qa.sh --kill emery
pebble wipe
```

Alternatively, to start afresh:

```sh
./aag-build-qa.sh --nuclear
```

### Watch Install

Replace `YOUR_PHONE_IP` with the Developer Connection Server IP shown in the Pebble mobile app.

```sh
pebble build
pebble install --phone YOUR_PHONE_IP
```

Automation harness:

```sh
./aag-build-qa.sh -bc
./aag-build-qa.sh -p YOUR_PHONE_IP
```
