# Contributing

Thanks for your interest. This document explains how to extend and modify *At A Glance* from cloning the repo, configuring your IDE, changing and validating code, and updating docs.

## Adjacent
- [Watchface_Readme](Readme.md) gives an overview of the watch face with screenshots
- [BuildandInstall](BuildandInstall.md) describes build and install flows and associated commands.
- [Validation](Validation.md) for validation paths and evidence.

## Environment Setup

### Getting Started

Clone the repository and enter its directory:

```sh
git clone https://github.com/jhatax/at-a-glance.git
cd at-a-glance
npm install
pebble build
```
The build produces the PBW and platform artifacts under `build/`.

### Prerequisites

Install local tools used by the build, QA, and formatting flows:

```sh
brew install node python pre-commit yapf shfmt clang-format
```

Install the Pebble SDK and `pebble` CLI using the official Pebble SDK instructions. The repository uses the SDK toolchain to build, install, and exercise the watch face on supported emulators.

Verify the environment:

```sh
node --version
npm --version
python3 --version
pre-commit --version
yapf --version
shfmt --version
clang-format --version
pebble --version
```

Initialize the repository's checked-in Git hook:

```sh
zsh setup-git-hook.sh
```

The hook delegates each commit to the repository's pre-commit configuration.

### Making a Change
1. Update local to main before starting work, create a branch, get to work:
```sh
git switch main
git pull --ff-only origin main
git switch -c my-fix
```
2. Test your code; review documentation
```sh
./aag-build-qa.sh --plans
./aag-build-qa.sh --list-steps canary
./aag-build-qa.sh --list-steps dev-smoke
./aag-build-qa.sh --exec-plan dev-smoke
```
3. Submit a PR with a clear description of change(s): What, Why, How Validated

PRs get reviewed in the order of submission and as fast as possible.

### IDE Support

The following are in the repo to facilitate development and QA activities:

1. `compile_commands.json` for `IDE-clangd` integration
2. `.clangd` for workspace-local `clangd` behavior
3. `.clang-format` and `.pre-commit-config.yaml` for formatting consistency
4. `.githooks` and `setup-git-hook.sh` for Git check-in quality control

If editor diagnostics look stale or incorrect, regenerate the compile database from a fresh build rather than editing flags by hand.

The verbose harness build may generate `compile_commands.json` for the current compile-database target. `PebbleAdapter` supplies the active SDK compiler path to `compilerdbgenerator.py`; build and compile-database ownership is documented in [BuildandInstall](BuildandInstall.md).

### Build, Install, and QA Automation

The automation harness, `aag-build-qa.sh`, provides build, install, and QA commands. Make it executable if needed:

```sh
chmod +x aag-build-qa.sh
```

**Features**

1. Install the watch face on emulators and devices

```sh
./aag-build-qa.sh --emulators emery
./aag-build-qa.sh --emulators chalk
./aag-build-qa.sh -p <Developer Connection Server IP>
```

2. Reset emulator and build state: `./aag-build-qa.sh --nuclear`
- kills running emulators
- wipes emulator state
- runs `pebble clean`
- runs a verbose build and generates `compile_commands.json`
- run `./aag-build-qa.sh --emulators <target>` separately when recovery should continue into emulator installation

3. Execute a QA plan: `./aag-build-qa.sh --exec-plan canary`

4. More info: `./aag-build-qa.sh -h`

## Repo Hygiene Guidelines

- Treat every change as small embedded firmware work.
- Review architecture before non-trivial edits.
- Design first. Audit live source, docs, generated files, and dirty tree before editing.
- Prefer direct code over helper churn when the boundary is obvious and local.

### Guidelines for C Code

- Use portable C within the Pebble SDK's constrained embedded runtime model.
- Format code with `clang-format`; the repo has a `.clang-format`.
- Use integer layout and drawing math only.
- Avoid heap allocation unless required.
- Match every acquired resource with a destroy path.
- Use fixed-size buffers and `snprintf`.
- Keep allocation, network, JSON, heavy formatting, and layout calculation out of layer update procs.
- Prefer Pebble platform-provided capability guards, e.g., `PBL_HEALTH`.
- Do not pass or return large watch face or layout structs by value.
- Keep module APIs narrow.
- Prefer direct, auditable code.
- Check `helper.c/.h` and `substratum_renderer.c/.h` before adding new helpers.

### Runtime Logs

Use the following to guide your use of Pebble's built-in `APP_LOG`:

- Use `INFO` logs temporarily during feature development.
- Use `APP_LOG_LEVEL_DEBUG` for debug-build diagnosis.
- Remove temporary `INFO` logs before committing code.
- Retain only `APP_LOG_LEVEL_WARNING` and `APP_LOG_LEVEL_ERROR` messages that materially help diagnose deployed watch-face issues.

### Debugging

For UI changes, one useful validation path is enabling `ATAGLANCE_DEBUG`, installing in the emulator, and inspecting the debug-rendered layer or glyph bounds. Build activation lives in [BuildandInstall](BuildandInstall.md); emulator validation flows live in [Validation](Validation.md).

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

Glyph validation is required whenever glyphs are modified or new glyphs are introduced. Treat screenshots as evidence; keep accepted visual evidence in [UserInterface](UserInterface.md).

## Repository Maintenance

Use `.gitignore` to keep generated local state out of source control. Build artifacts, editor indexes, logs, temporary QA vectors, dependency directories, and generated compile databases should not become source files.

The exception is a release PBW when explicitly prepared for distribution. A PBW is an output artifact to review or publish, not a substitute for committing the source, manifest, resources, and documentation that produced it.

### Review and Commit Discipline

Before committing a change:

- run `git diff --check`
- run `pebble build` for code changes or explicitly report the gap
- run the relevant scenario or QA plan from [Validation](Validation.md)
- use emulator screenshots for visual or layout changes when practical
- review API, lifecycle, layout, and platform changes before coding
- update relevant documentation
- review the diff and stage only intended files
- delete temporary screenshots and buffers unless intentionally retained

### Formatting consistency: Clang-format, YAPF, shfmt

The repository's pre-commit configuration runs YAPF for Python, `shfmt` for shell, and `clang-format` for C and headers. `.clang-format` and `.clangd` provide consistent formatting and Pebble-aware editor diagnostics from the repo root.

Use `clang-format` before committing C changes:

```sh
clang-format -i src/c/*.[ch] src/modules/*.[ch]
```

If `clangd` cannot locate the Pebble compiler, configure your editor with the toolchain driver:

```text
--query-driver=/path/to/arm-none-eabi-gcc
```

## Documentation Management

- Use [DocumentationContracts](DocumentationContracts.md) to define a document's purpose, sub-concepts, and navigation.
- Each document should still carry enough local context to be useful without forcing a reader to inspect code for basic understanding.
- Follow the one-concept, one-document, one-responsibility invariant.

## Read Next

- [BuildandInstall](BuildandInstall.md) for build, install, and editor-tooling details.
- [Validation](Validation.md) for validation contract, evidence, and release profiles.
- [RuntimeArchitecture](RuntimeArchitecture.md) for runtime ownership and boundaries.
- [QA_Readme](../qa/README.md) for watch-face QA plan execution.
