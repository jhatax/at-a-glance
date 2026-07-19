# Contributing

This document explains how to extend, modify, and create your own incarnation of *At A Glance*.

- Use it as the workflow and review guide for contributors. Keep product rules, visual vocabulary, and runtime architecture decisions in their dedicated documents.
- Contributing to a project involves a cycle that spans: Setup Repo -> Configure IDE environment -> Build -> Validate -> Repeat.
- This watch face is no different!

To shortcircuit the time it takes to setup and maximize productivity, this repo has config files for code formatting, git commits, IDE integration, and automation to help you execute the development cycle without learning the `pebble` tool. There are a number of follow-up documents that can accelerate bootstrap setup and IDE integration, and facilitate feature validation.

**Key document relationships**

```text
`this` covers the contributor flow at a high-level
|
V
`Build` describes the build flow
|
V
`Validation` describes validation and introduces the QA harness
|
V
`qa/README` dives into executing and creating QA plans for the watch face
```

## Required Reading

- [UserInterface](UserInterface.md) for the full visual reference implementation.
- [ArchitectureLedger](ArchitectureLedger.md) for the runtime architecture.
- [SourceMap](SourceMap.md) for high-level source-code structure.

## Part-1: Environment and IDE Setup

### GitHub integration

Clone the repository and enter its directory:

```sh
git clone https://github.com/jhatax/at-a-glance.git
cd at-a-glance
```

Update an existing checkout before starting work:

```sh
git pull --ff-only origin main
```

### First build

`npm install`: PebbleKit JS dependencies available

`pebble build`: Configure project, build platform outputs, populate build/ with PBW and artifacts

### IDE Support

The following are in the repo to facilitate development and QA activities:

1. `compile_commands.json` for `IDE-clangd` integration
2. `.clangd` for workspace-local `clangd` behavior
3. `.clang-format`, `.pre-commit-config.yaml` for formatting consistency
4. `.githooks`, `setup-git-hook.sh` for repo check-in quality control
   - Source `setup-git-hook.sh` to initialize the hook on first pull

If editor diagnostics look stale or incorrect, regenerate the compile database from a fresh build rather than editing flags by hand.

### Extensions to Setup, Build, Install, and Validation in Repo

The automation harness, `aag-build-qa.sh`, can accelerate your contribution workflows. Setup instructions:
`chmod +x <watchface-home>/aag-build-qa.sh`

Features:

1. Install the watch face on emulators and devices

```sh
./aag-build-qa.sh -i -e emery
./aag-build-qa.sh -i -e chalk
./aag-build-qa.sh -p <Developer Connection Server IP>
```

2. Reset emulator and build state: `./aag-build-qa.sh --nuclear`
- kills running emulators
- wipes emulator state
- runs `pebble clean`
- runs a verbose build and generates `compile_commands.json`
- combine it with `--install` when recovery should continue into install

3. Execute QA plans: `./aag-build-qa.sh -qaplan canary.scenario`

4. More info: `./aag-build-qa.sh -h`

## Part-2: Changes to Code, Docs, User Interface

- Treat every change as small embedded firmware work.
- Review architecture before non-trivial edits.
- Design first. Audit live source, docs, generated files, and dirty tree before editing.
- Prefer direct code over helper churn when the boundary is obvious and local.

### Guidelines for C-code

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

### Logs

Use the following to guide your use of Pebble's built-in `APP_LOG`:
1. Feature development logging:
- `INFO` logs temporary messages during feature development.
- `APP_LOG_LEVEL_DEBUG` to review debug build messages
- Delete all `APP_LOG(APP_LOG_LEVEL_INFO, ...)` calls before committing code
- Retain only absolutely essential `APP_LOG_LEVEL_DEBUG` messages
2. `APP_LOG_LEVEL_WARNING` and `APP_LOG_LEVEL_ERROR` capture durable runtime messages when they materially help diagnosis of issues with watch face post deployment.

### Debugging

For UI changes, one useful validation path is enabling `ATAGLANCE_DEBUG`, installing in the emulator, and inspecting the debug-rendered layer or glyph bounds. Build activation lives in [Build](BuildandInstall.md); emulator validation flows live in [Validation](Validation.md).

### Glyph Development

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
[UserInterface](UserInterface.md).

## Part-3: Repository Maintenance

Use `.gitignore` to keep generated local state out of source control. Build
artifacts, editor indexes, logs, temporary QA vectors, dependency directories,
and generated compile databases should not become source files.

The exception is a release PBW when explicitly prepared for distribution. A PBW
is an output artifact to review or publish, not a substitute for committing the
source, manifest, resources, and documentation that produced it.

### Review and Commit Discipline

Before committing a change:

- run `git diff --check`
- run `pebble build` for code changes or explicitly report the gap
- run scenario smoke coverage across the areas touched by the change
- use emulator screenshots for visual or layout changes when practical
- review API, lifecycle, layout, and platform changes before coding
- update relevant documentation
- review the diff and stage only intended files
- delete temporary screenshots and buffers unless intentionally retained
- initialize the githook if you haven't already for repo hygiene

### Formatting consistency: Clang-format, ruff, shfmt

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
see [Build](BuildandInstall.md).

### Git check-in hook

Formatting tools (`ruff`, `shfmt`, in addition to `clang-format`) have been configured to consistently format code right before you commit to git.

**One-Time Setup**

After cloning the repository, run the initialization script once from your terminal root to activate the local formatting pipeline:

```bash
zsh setup-hooks.sh
```

**Notes**
- This installs a `git commit` hook using git's out-of-the-box features.
- No extensions or additional binaries are needed.

## Part-4: Documentation Management

- Use [Documentation_Ontology] (Documentation_Ontology.md) to place documentation in its canonical owner.
- Each document should still carry enough local context to be useful without forcing a reader to inspect code for basic understanding.
- Adhere to "one concept, one document, one responsibility" documentation invariant

## Further Reading

- [Build](BuildandInstall.md) for build, install, and editor-tooling support.
- [Validation](Validation.md) for validation contract, scenarios, and evidence expectations.
- [QA_Readme](../qa/README.md) for how to execute watch face QA plans.
- [WritingTestCasesAndPlans](../qa/docs/WritingTestCasesAndPlans.md) for a deeper-dive into QA plan authoring.
- [QA_Harness_Implementation_Flow](../qa/docs/QA_Harness_Implementation_Flow.md) for the live execution and reporting flow.
- [SettingsandConfiguration](SettingsandConfiguration.md) for the settings catalog, Clay mapping, message-key contract, persistence, and validation obligations.
- [ProductInvariants](ProductInvariants.md) for the invariants that preserve product identity across devices.
- [VisualVocabulary](VisualVocabulary.md) for the visual grammar that *can* satisfy visual invariants.
- [Watchface_Readme](../README.md) for a product introduction: screenshots, getting started, download links.
