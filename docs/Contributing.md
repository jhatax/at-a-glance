# Contributing

This document is the contributor workflow for _At A Glance_: prepare the repository, make a narrow source or documentation change, validate the affected path, and submit evidence with the change.

## Adjacent

- [BuildandInstall](BuildandInstall.md) owns build, installation, recovery, and editor-tooling operations.
- [Validation](Validation.md) owns validation-path selection and evidence expectations.

## Prepare The Repository

Clone the repository, install its JavaScript dependencies, and initialize the checked-in Git hook:

```sh
git clone https://github.com/jhatax/at-a-glance.git
cd at-a-glance
npm install
zsh setup-git-hook.sh
```

Install the Pebble SDK and CLI separately. The complete build and environment contract is in [BuildandInstall](BuildandInstall.md). The local formatting and QA tools are:

```sh
brew install node python pre-commit yapf shfmt clang-format
```

Verify the tools needed for the path you intend to run:

```sh
node --version
npm --version
python3 --version
clang-format --version
pebble --version
```

## Make A Change

1. Start from an up-to-date branch and create a focused work branch.

   ```sh
   git switch main
   git pull --ff-only origin main
   git switch -c my-fix
   ```

2. Read the source owner and runtime boundary before editing. Keep one coherent slice, preserve unrelated user changes, and do not add compatibility scaffolding without a live requirement.

3. Reconcile every affected producer and consumer. Settings, AppMessage, resources, capabilities, generated files, source, QA commands, and documentation change together when their contract changes. Use [SettingsandConfiguration](SettingsandConfiguration.md) for the settings and message-key checklist.

4. Choose the narrowest validation path that proves the change. Use [Validation](Validation.md) for the change-type matrix, [BuildandInstall](BuildandInstall.md) for build or install operations, and [QA README](../qa/README.md) for scenario execution.

5. Review the live diff, generated artifacts, and documentation before staging. Do not treat an old QA run or stale document as a source-of-truth substitute.

## Editor And Local Checks

The repository includes VS Code settings and tasks for clangd, format-on-save, Prettier, Python unittest discovery, harness tests, and explicit emulator QA. Regenerate `compile_commands.json` from a fresh build when editor diagnostics are stale; do not edit compiler flags by hand.

The npm commands provide the same repeatable local checks outside the editor:

```sh
npm run lint
npm run format:check
npm run docs:check
npm run test:harness
npm run check
```

Use one harness test or test class when narrowing a change:

```sh
npm run test:harness:focused -- test_harness_correctness.HarnessCorrectnessTests.test_named_scenario_loads_and_expands
```

Use `npm run docs:reflow` to repair artificial prose breaks across tracked Markdown files. `npm run docs:check` verifies the prose-wrapping rule without rewriting files. Prettier uses `proseWrap: "never"`; tables, headings, lists, blockquotes, and code fences retain their structural breaks.

The checked-in pre-commit hook formats staged files, runs staged JavaScript linting, checks Markdown, and runs the harness unit suite. Emulator scenarios remain explicit because they install, reset, and exercise stateful devices. Use the `QA: dev-smoke (emulator)` task or the named plan documented by [Validation](Validation.md) when runtime evidence is required.

## Source And Runtime Discipline

- Keep ownership explicit: lifecycle, services, persistence, AppMessage, and dispatch belong to `src/c/ataglance.c`; runtime facts belong to `watchface_runtime_boundary.c`; the live surface and feature lifecycle belong to `watchface.c`; feature modules own their layers, source state, refresh, and destruction.
- Keep feature APIs narrow. Do not pass watchface or layout structs by value, retain `WatchfaceSurface*` in feature modules, or make a feature depend on a broader global update type when a narrower input exists.
- Use fixed-size buffers, `snprintf`, integer geometry, capability guards, and paired resource cleanup. Keep allocation, network, JSON, heavy formatting, and layout calculations out of layer update procedures.
- Render through `watchface_refresh()`. Palette changes repaint; new data targets the affected strata. Required text updates must not depend on optional icons.
- Read [RuntimeArchitecture](RuntimeArchitecture.md) and [WatchfaceImplementationFlow](WatchfaceImplementationFlow.md) before changing ownership or lifecycle boundaries.

## C, JavaScript, And Tooling Rules

For C and headers, use the repository `.clang-format`, keep lines at or below 100 characters where practical, and prefer direct auditable code. Check existing helpers before introducing a new abstraction.

For PebbleKit JS, keep the runtime-compatible CommonJS style, validate AppMessage payloads, guard optional APIs, and keep network and fallback behavior explicit. Prettier covers the JavaScript and JSON source surface.

For Python and shell changes, use the repository YAPF and shfmt configuration. Keep the shell-to-Python QA handoff narrow: zsh owns environment preparation, and Python owns harness plan state, execution, reports, and screenshots.

## Visual And Glyph Changes

For layout, palette, glyph, or visible-state changes, validate the actual render path and inspect emulator screenshots. Use [UserInterface](UserInterface.md) for accepted visual evidence and [VisualVocabulary](VisualVocabulary.md) for shared visual meaning.

For broad glyph exploration, use `glyph-lab` before production porting:

1. State the glyph decision and relevant runtime states.
2. Review color and black-and-white variants.
3. Review at least emery, flint, chalk, and gabbro.
4. Port only the selected drawing rules into the production module.
5. Validate the final watchface layout and update visual evidence when it changes.

## Documentation Changes

Give each document one independent question, one canonical owner, and one clear audience. Use [DocumentationOntology](DocumentationOntology.md) to route concepts and [DocumentationContracts](DocumentationContracts.md) to maintain `Adjacent`, `Read Next`, and ownership metadata. Link to a canonical document instead of duplicating its content.

Keep Markdown prose paragraphs on logical lines so the editor or viewport controls visual wrapping. Preserve structural breaks for headings, lists, tables, blockquotes, code fences, and intentional hard breaks.

## Review And Commit

Before committing:

- run the narrow validation path required by [Validation](Validation.md)
- run `npm run check` when the change touches code, tooling, or documentation workflows
- run `git diff --check`
- inspect the final diff and stage only intended files
- inspect screenshots or runtime evidence for visible changes
- state any unchecked validation path or known limitation

The pre-commit hook is the final local gate, not a replacement for a build, emulator scenario, screenshot review, or release signoff when those are required by the change.

## Read Next

- [BuildandInstall](BuildandInstall.md) for build, install, recovery, and compile-database operations.
- [Validation](Validation.md) for validation-path selection and evidence.
- [RuntimeArchitecture](RuntimeArchitecture.md) for runtime ownership and lifecycle.
- [SettingsandConfiguration](SettingsandConfiguration.md) for settings and message contracts.
- [QA README](../qa/README.md) for harness operation and artifacts.
