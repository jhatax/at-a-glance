# Contributing

Use this workflow to prepare the repository, make a focused change, validate the affected path, and provide review evidence.

## Adjacent

- [BuildandInstall](BuildandInstall.md) owns build, installation, recovery, and editor-tooling operations.
- [Validation](Validation.md) owns validation-path selection and evidence expectations.
- [RAID Log](RAID_LOG.md) records accepted engineering decisions and active review context.

## Prepare The Repository

Clone the repository, install JavaScript dependencies, and enable the checked-in Git hooks:

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

Regenerate `compile_commands.json` with `./aag-build-qa.sh --build-clean` when editor diagnostics are stale. This clean verbose build captures the compiler commands needed by clangd.

Run local checks from the repository root:

```sh
npm run format:check
npm run docs:check
npm run test:harness
npm run test:native
npm run test:js
npm run check
```

`npm run check` formats source files before running checks and tests. Review its changes before staging.

`npm run test:js` uses Node's built-in test runner. The tests in `tests/js/` load PKJS source with isolated GPS, network, and timer mocks. They require no Pebble build, emulator, network connection, or real timer delays.

Use one harness test or test class when narrowing a change:

```sh
PYTHONPATH=tools/harness_py uv run --python 3.13 python -m unittest unittests.test_harness_correctness.HarnessCorrectnessTests.test_named_scenario_loads_and_expands
```

Documentation formatting commands:

- `npm run docs:reflow`: Repair artificial prose breaks across tracked Markdown files.
- `npm run docs:check`: Verify the prose-wrapping rule without rewriting files.
- Prettier: Uses `proseWrap: "never"`; tables, headings, lists, blockquotes, and code fences retain their structural breaks.

The checked-in hooks have these responsibilities:

- Pre-commit:
  - Format staged files.
  - Format staged JavaScript with Prettier.
  - Check Markdown.
  - Run `npm run test:harness`, `npm run test:native`, and `npm run test:js`.
  - Do not launch emulators.
- Pre-push:
  - Run the harness, native C, and JavaScript unit tests.
  - Build the watchface.
- Manual pre-push visual QA: `./aag-build-qa.sh --exec pre-push-gate --force`
- Pre-release QA: `./aag-build-qa.sh --exec pre-release-gate`
- Validation guidance: Use the named plans documented by [Validation](Validation.md) when broader runtime evidence is required.

Pre-commit restages the full contents of files that were staged when the hook started. Review those files before committing.

## Source And Runtime Discipline

- Follow the ownership boundaries in [RuntimeArchitecture](RuntimeArchitecture.md) and the source map in [WatchfaceImplementationFlow](WatchfaceImplementationFlow.md). Review those boundaries before moving behavior between files.
- Keep feature APIs narrow. Do not pass watchface or layout structs by value, retain `WatchfaceSurface*` in feature modules, or make a feature depend on a broader global update type when a narrower input exists.
- Use fixed-size buffers, `snprintf`, integer geometry, capability guards, and paired resource cleanup. Keep allocation, network, JSON, heavy formatting, and layout calculations out of layer update procedures.
- Render through `watchface_refresh()`. Palette changes repaint; new data targets the affected strata. Required text updates must not depend on optional icons.

## C, JavaScript, And Tooling Rules

For C and headers, use the repository `.clang-format`, keep lines at or below 100 characters where practical, and prefer direct auditable code. Check existing helpers before introducing a new abstraction.

For PebbleKit JS, use runtime-compatible CommonJS modules, validate AppMessage payloads, guard optional APIs, and handle network failures explicitly. Prettier formats JavaScript and JSON.

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

- Give each document one independent question, canonical purpose, and clear audience.
- Use [DocumentationOntology](DocumentationOntology.md) to route concepts.
- Use [DocumentationContracts](DocumentationContracts.md) to maintain `Adjacent`, `Read Next`, and ownership metadata.
- Link to the canonical document instead of duplicating its content.

Keep each prose paragraph on one logical line so the editor controls wrapping. Preserve structural breaks for headings, lists, tables, blockquotes, code fences, and intentional hard breaks.

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

- [RuntimeArchitecture](RuntimeArchitecture.md) for runtime ownership and lifecycle.
- [SettingsandConfiguration](SettingsandConfiguration.md) for settings and message contracts.
- [QA README](../qa/README.md) for harness operation and artifacts.
