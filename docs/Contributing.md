# Contributing

This document explains how to change At A Glance safely.

Use it as the workflow and review guide for contributors. Keep product rules,
visual vocabulary, and runtime architecture decisions in their dedicated
documents.

## Required Reading

- [Build.md](Build.md) for build, install, and editor-tooling support.
- [Validation.md](Validation.md) for validation contract, scenarios, and
  evidence expectations.
- [UserInterface.md](UserInterface.md) for the full visual reference implementation.
- [ArchitectureLedger.md](ArchitectureLedger.md) for the runtime architecture.

## Scope: Changes to Code, Docs, User Interface

- Treat every change as small embedded firmware work.
- Review architecture before non-trivial edits.
- Design first. Audit live source, docs, generated files, and dirty tree before editing.
- Prefer direct code over helper churn when the boundary is obvious and local.

## C-Coding Decisions

- Use portable C within the Pebble SDK's constrained embedded runtime model.
- Format code with `clang-format`; the repo has a `.clang-format`.
- Use integer layout and drawing math only.
- Avoid heap allocation unless required.
- Match every acquired resource with a destroy path.
- Use fixed-size buffers and `snprintf`.
- Keep allocation, network, JSON, heavy formatting, and layout calculation
  out of layer update procs.
- Prefer Pebble platform-provided capability guards, e.g., `PBL_HEALTH`.
- Do not pass or return large watch face or layout structs by value.
- Keep module APIs narrow.
- Prefer direct, auditable code.
- Check `helper.c/.h` and `substratum_renderer.c/.h` before adding new helpers.

## C-Code Formatting and Clangd Integration

The repository includes `.clang-format` and `.clangd` so contributors get
consistent formatting and Pebble-aware editor diagnostics from the repo root.

Use `clang-format` before committing C changes:

```sh
clang-format -i src/c/*.[ch] src/modules/*.[ch]
```

If `clangd` cannot locate the Pebble compiler, configure your editor with the
toolchain driver:

```text
--query-driver=/path/to/arm-none-eabi-gcc
```

For how `compile_commands.json` is generated from Pebble's verbose build output,
see [Build.md](Build.md).

## Repository Maintenance

Use `.gitignore` to keep generated local state out of source control. Build
artifacts, editor indexes, logs, temporary QA vectors, dependency directories,
and generated compile databases should not become source files.

The exception is a release PBW when explicitly prepared for distribution. A PBW
is an output artifact to review or publish, not a substitute for committing the
source, manifest, resources, and documentation that produced it.

## Build And Validation

Use:

- [Build.md](Build.md) for build, install, compile database generation, and
  editor-tooling setup
- [Validation.md](Validation.md) for validation flow, scenarios, runtime
  validation commands, and evidence expectations

**Contributor rule**

- If a code change needs build or runtime verification, run relevant build and
  validation flows from [Build.md](Build.md) and [Validation.md](Validation.md), respecitively, or explicitly report the gap.
- If a change touches settings, Clay, PKJS normalization, AppMessage keys, or
  persistence, update [Settings_and_Configuration.md](Settings_and_Configuration.md) and run the relevant
  settings validation path.

## Basic Local Loop

Build and launch on the default emulator:

```sh
pebble build
pebble install --emulator emery
```

Recover a stale or unresponsive emulator:

```sh
pebble kill --force
pebble wipe
```

Use the harness nuclear path when the emulator is not responding or persisted
emulator state is interfering with validation:

```sh
./aag-build-qa.sh --nuclear
```

The `--nuclear` flag kills emulators, wipes emulator state, and forces a
clean build. This is stronger than a normal rebuild because it clears stale
emulator process and persistence state before rebuilding. Combine it with
`--install` when recovery should continue into emulator install.

For compile database generation and install flows, use [Build.md](Build.md).
For emulator matrices, config-page validation, validation scenarios, and QA
command details, use [Validation.md](Validation.md). For the QA System
implementation manual, use [../qa/README.md](../qa/README.md).

## Logs And Debugging

### Logs

Use Pebble's built-in `APP_LOG` with restraint.

- Use `INFO` logs as temporary messages during feature development.
- Use `APP_LOG_LEVEL_DEBUG` sparingly for temporary diagnostics.
- Use `APP_LOG_LEVEL_WARNING` and `APP_LOG_LEVEL_ERROR` for durable runtime
  messages when they materially help diagnosis.
- Delete all `APP_LOG(APP_LOG_LEVEL_INFO, ...)` calls before committing code.

### Debugging

For UI changes, one useful validation path is enabling `ATAGLANCE_DEBUG`,
installing in the emulator, and inspecting the debug-rendered layer or glyph
bounds. Build activation lives in [Build.md](Build.md); emulator validation
flows live in [Validation.md](Validation.md).

## Glyph Validation

Use `glyph-lab` for broad glyph work before production porting.

Recommended loop:

1. Define the glyph decision in prose.
2. Identify the real state variants used by the product.
3. Create or update lab variants only for those states.
4. Review color and black-and-white screenshots.
5. Review at least `emery`, `flint`, `chalk`, and `gabbro` before production porting.
6. Select final glyph design after visual confirmation.
7. Port the selected drawing rules into production modules.
8. Validate in the real watch face layout.

Glyph validation is required whenever glyphs are modified or new glyphs are
introduced. Treat screenshots as evidence; keep accepted visual evidence in
[UserInterface.md](UserInterface.md).

## Review and Commit Discipline

Before committing a change:

- run `git diff --check`
- run `pebble build` for code changes or explicitly report the gap
- run scenario smoke coverage across the areas touched by the change
- use emulator screenshots for visual or layout changes when practical
- review API, lifecycle, layout, and platform changes before coding
- update relevant documentation
- review the diff and stage only intended files
- delete temporary screenshots and buffers unless intentionally retained

## Documentation QA

Use the Ontology to place documentation in its canonical owner. Each document
should still carry enough local context to be useful without forcing a reader to
inspect code for basic understanding.

## Further Reading

- [Settings_and_Configuration.md](Settings_and_Configuration.md) for the settings catalog, Clay mapping,
  message-key contract, persistence, and validation obligations.
- [Build.md](Build.md) for build, install, and editor-tooling support.
- [Validation.md](Validation.md) for validation contract, scenarios, and
  evidence expectations.
- [ProductInvariants.md](ProductInvariants.md) for the invariants that preserve product identity across devices.
- [VisualVocabulary.md](VisualVocabulary.md) for the visual grammar that *can* satisfy visual invariants.
- [../README.md](../README.md) for a product introduction: screenshots, getting started, download links.
