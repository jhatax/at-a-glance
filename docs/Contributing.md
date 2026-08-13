# Contributing

This document is the contributor workflow for _At A Glance_: prepare the repository, make a narrow source or documentation change, validate the affected path, and submit evidence with the change.

## Adjacent

- [BuildandInstall](BuildandInstall.md) owns build, installation, recovery, and editor-tooling operations.
- [Validation](Validation.md) owns validation-path selection and evidence expectations.
- [RAID Log](RAID_LOG.md) records accepted engineering decisions and active review context.

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
brew install node python uv pre-commit yapf shfmt clang-format
```

Verify the tools needed for the path you intend to run:

```sh
node --version
npm --version
uv --version
uv run --python 3.13 python --version
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

2. Read the source owner, runtime boundary, and relevant [RAID Log](RAID_LOG.md) decisions before editing. Keep one coherent slice, preserve unrelated user changes, and do not reopen an accepted direction without fresh contrary evidence.

3. Reconcile every affected producer and consumer:

   - Settings, AppMessage, resources, capabilities, generated files, source, QA commands, and documentation change together when their contract changes.
   - Use [SettingsandConfiguration](SettingsandConfiguration.md) for the settings and message-key checklist.

4. Choose the narrowest validation path that proves the change. Use [Validation](Validation.md) for the change-type matrix, [BuildandInstall](BuildandInstall.md) for build or install operations, and [QA README](../qa/README.md) for plan execution.

5. Review the live diff, generated artifacts, and documentation before staging. Do not treat an old QA run or stale document as a source-of-truth substitute.

## Editor And Local Checks

Editor support includes:

- clangd
- format-on-save
- Prettier
- Python unittest discovery
- Harness tests
- Explicit emulator QA

Regenerate `compile_commands.json` from a fresh build when editor diagnostics are stale. Do not edit compiler flags by hand.

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

Documentation formatting commands:

- `npm run docs:reflow`: Repair artificial prose breaks across tracked Markdown files.
- `npm run docs:check`: Verify the prose-wrapping rule without rewriting files.
- Prettier: Uses `proseWrap: "never"`; tables, headings, lists, blockquotes, and code fences retain their structural breaks.

The checked-in hooks have these responsibilities:

- Pre-commit: Format staged files, run staged JavaScript linting, check Markdown, and run `npm run test:harness`. It does not launch emulators.
- Pre-push: Run the unit tests, build the watchface, and execute the screenshot-producing `pre-push-gate` suite.
- `pre-push-gate`: Aggregate the `visual-refresh` Matrix across Emery, Chalk, and Flint in white and black display modes.
- Broader coverage: Keep emulator and pre-release Matrices explicit because they install, reset, and exercise stateful devices.
- Validation guidance: Use the named plans documented by [Validation](Validation.md) when broader runtime evidence is required.

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

Documentation rules:

- Give each document one independent question.
- Give each document one canonical owner.
- Give each document one clear audience.
- Use [DocumentationOntology](DocumentationOntology.md) to route concepts.
- Use [DocumentationContracts](DocumentationContracts.md) to maintain `Adjacent`, `Read Next`, and ownership metadata.
- Link to the canonical document instead of duplicating its content.

Keep Markdown prose paragraphs on logical lines so the editor or viewport controls visual wrapping. Preserve structural breaks for headings, lists, tables, blockquotes, code fences, and intentional hard breaks.

For QA grammar, resolver, execution, or report changes:

- Update the owning QA document.
- Update [settled decisions](../qa/settled-decisions.md) only when the change establishes a new durable contract.
- Keep [QA README](../qa/README.md), [WritingTestCasesAndPlans](../qa/docs/WritingTestCasesAndPlans.md), and [QAHarnessImplementationFlow](../qa/docs/QAHarnessImplementationFlow.md) consistent with the live handoff.

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
