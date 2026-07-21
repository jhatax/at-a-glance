# QA Harness Decision Ledger

This ledger records deliberate architecture decisions for the At A Glance QA harness and Validation System.

It answers four questions:

1. what decision was made
2. why it was made
3. what it replaces
4. what must remain true going forward

**Adjacent**

- Canonical validation contract: [Validation.md](Validation.md)
- Canonical build contract: [Build.md](BuildandInstall.md)
- QA implementation map: [../qa/README.md](../qa/README.md)

## Scope

This ledger covers:

- validation-system architecture
- harness language/runtime choices
- artifact-location decisions
- profile and scenario ownership decisions

This ledger does not own:

- contributor workflow
- runtime architecture of the watch face
- implementation details of `qa/`

## Target End State

The agreed end state is:

- a thin public harness entrypoint
- a Python-owned validation core
- durable artifacts under `qa/qa-runs/`
- constrained scenario/profile definitions
- local-first validation that remains comparison-ready

## Non-Negotiable Invariants

- Validation artifacts must be durable across clean builds.
- Validation artifacts must remain comparable with prior runs.
- Stage ownership must stay explicit.
- Profile ownership must stay explicit.
- Scenario grammar must remain constrained and documented.
- Policy must not drift into `qa/README.md`.

## Decisions

### QA-001

- Decision: Split the combined build-and-validation document into separate
  canonical build and validation docs.
- Status: accepted
- Date: 2026-07-08
- Why: build architecture and validation architecture are different concepts and
  were creating a single-document gravity well.
- Consequences:
  - `docs/Build.md` owns build, install, compile database generation, and
    editor-tooling support
  - `docs/Validation.md` owns validation behavior, artifacts, profiles, and
    evidence expectations
  - `qa/README.md` remains an implementation map only
- Replaces: `docs/Build_and_Validation.md`
- Source documents:
  - [Build.md](BuildandInstall.md)
  - [Validation.md](Validation.md)
  - [../qa/README.md](../qa/README.md)

### QA-002

- Decision: Keep `aag-build-qa.sh` as the public entrypoint.
- Status: accepted
- Date: 2026-07-08
- Why: a stable local entrypoint is useful, even if the implementation language
  changes underneath it.
- Consequences:
  - the entrypoint may remain a thin wrapper
  - implementation details must live below the entrypoint
  - the entrypoint does not justify keeping core logic in shell
- Replaces: none
- Source documents:
  - [Build.md](BuildandInstall.md)
  - [Validation.md](Validation.md)

### QA-003

- Decision: Validation artifacts live under `qa/qa-runs/`, not under `build/`.
- Status: accepted
- Date: 2026-07-08
- Why: `build/` is not durable. Validation evidence must survive clean builds
  and remain available for run-to-run comparison.
- Consequences:
  - each run writes to `qa/qa-runs/<run-id>/`
  - reports, logs, and screenshots are durable by default
  - later comparison work can rely on prior-run availability
- Replaces: earlier artifact placement under `build/qa/`
- Source documents:
  - [Validation.md](Validation.md)
  - [../qa/README.md](../qa/README.md)
  - [../qa/qa-runs/README.md](../qa/qa-runs/README.md)

### QA-004

- Decision: Scenario and profile definitions use a constrained grammar and
  fixed vocabulary.
- Status: accepted
- Date: 2026-07-08
- Why: profile behavior must be data-driven and reviewable, not hidden inside
  ad hoc shell branching.
- Consequences:
  - scenario files define curated profile behavior
  - grammar expansion must be deliberate and documented
  - profile coverage evolves by editing scenario definitions, not by rewriting
    family loops
- Replaces: implicit profile behavior buried in raw test loops
- Source documents:
  - [Validation.md](Validation.md)
  - [../qa/README.md](../qa/README.md)

### QA-005

- Decision: Limit the current scenario creation round to no more than four
  scenarios.
- Status: accepted
- Date: 2026-07-08
- Why: the current goal is to establish ownership and grammar cleanly, not to
  explode coverage before the system core is stable.
- Consequences:
  - the current scenario set stays deliberately small
  - additional scenarios require explicit follow-on decisions
- Replaces: uncontrolled scenario growth during initial implementation
- Source documents:
  - [Validation.md](Validation.md)

### QA-006

- Decision: Restart Milestone 4 in Python rather than extending the shell-owned
  validation core.
- Status: accepted
- Date: 2026-07-08
- Why: Milestone 4 moved the harness past simple orchestration into structured
  parsing, lifecycle management, artifact emission, and report generation.
  Those responsibilities are a poor fit for shell and a good fit for Python.
- Consequences:
  - the shell entrypoint may remain as a wrapper
  - the validation core should move to Python
  - milestones 5 and 6 should build on the Python core, not on expanded zsh
  - the current scenario vocabulary and durable artifact contract must survive
    the rewrite
- Replaces: continuing Milestone 4 by deepening the zsh implementation
- Source documents:
  - [Validation.md](Validation.md)
  - [Build.md](BuildandInstall.md)

### QA-007

- Decision: Apply the same escalation rubric to harness architecture that is
  applied to product code: two distinct failures in the same slice means stop
  and reassess.
- Status: accepted
- Date: 2026-07-08
- Why: repeated friction across lifecycle, parsing, durability, or reporting is
  evidence that the implementation shape may be wrong, not just that one more
  fix is needed.
- Consequences:
  - repeated harness failures must trigger an explicit hand-raise
  - architectural mismatch is a first-class finding
  - momentum does not justify continuing down a failing path
- Replaces: implicit “keep fixing locally” behavior
- Source documents:
  - [Validation.md](Validation.md)

### QA-008

- Decision: Replace trap-based run finalization with explicit top-level
  closeout.
- Status: accepted
- Date: 2026-07-08
- Why: trap-driven finalization hides lifecycle ownership inside shell control
  flow, is easy to misread in zsh, and is a poor fit for a harness that must
  remain maintainable as reporting, assertions, and comparison outputs grow.
- Consequences:
  - the top-level harness flow must call closeout explicitly
  - shell trap behavior must not be the source of truth for run completion
  - report emission must happen at a deliberate, readable lifecycle boundary
  - future validation features must build on explicit closeout, not on trap
    choreography
- Replaces: EXIT-trap-based finalization as the intended lifecycle shape
- Source documents:
  - [Validation.md](Validation.md)
  - [Build.md](BuildandInstall.md)

### QA-009

- Decision: Keep Milestone 5 assertions irreducibly simple and scoped to the
  watchface validation contract.
- Status: accepted
- Date: 2026-07-09
- Why: the harness needs explicit pass/fail judgments for release confidence,
  but it must not grow into a generic assertion studio with SaaS-style rule
  sprawl.
- Invariants:
  - assertions must stay tied to the harness contract, not become a general
    rules engine
  - assertions may cover only:
    - stage success
    - scenario completion
    - artifact presence
    - expected screenshot count
    - manual signoff placeholders in release-grade reports
  - assertion outcomes must be explicit in run reports and drive run status
  - assertions must remain profile-aware and artifact-aware, not environment-
    agnostic abstractions
  - assertion logic must prefer direct data from the current run over duplicate
    parallel configuration
  - new assertion categories require an explicit decision, not opportunistic
    feature creep
- Covered outcomes:
  - did the expected stages complete correctly
  - did the expected named checkpoints complete
  - does the run contain the required evidence files
  - did the run produce the expected screenshot count for the profile
  - what manual release-review items remain for a human
- Explicitly out of scope:
  - arbitrary user-authored assertion DSLs
  - nested rule composition engines
  - cross-run policy evaluation
  - pixel-diff gating
  - general-purpose workflow approvals
  - SaaS-style compliance or observability assertions
- Replaces: any implicit assumption that Milestone 5 should expand into a
  generic validation framework
- Source documents:
  - [Validation.md](Validation.md)

### QA-010

- Decision: Keep derived release assertions narrow: required screenshot counts
  apply only after successful scenario execution, and manual signoff blocks
  appear only for `pre-release-gate`.
- Status: accepted
- Date: 2026-07-09
- Why: screenshot-count assertions are evidence checks for completed profile
  runs, not a second failure source when a run never reached scenario
  execution. Manual signoff belongs only to release-grade profiles.
- Consequences:
  - failed pre-scenario runs do not accumulate synthetic screenshot-count
    failures
  - `canary` and `dev-smoke` stay free of release-only signoff noise
  - release reports still carry the human review checklist even when the run
    fails early
- Replaces: implicit behavior around when derived assertions should fire
- Source documents:
  - [Validation.md](Validation.md)

### QA-011

- Decision: Python is the only scenario engine. Shell does not own a parallel
  scenario parser.
- Status: accepted
- Date: 2026-07-09
- Why: duplicated grammar and alias expansion across shell and Python had
  already drifted and created two conflicting sources of truth.
- Consequences:
  - profile parsing, alias resolution, and grammar validation live in
    `qa/python/scenarios.py`
  - shell consumes emitted step data only
  - invalid scenario fixtures are validated through `qa/runner.py validate-file`
  - future scenario changes must update the Python parser and the canonical
    documentation, not revive shell-side parsing
- Replaces: the unused shell-owned scenario parser
- Source documents:
  - [Validation.md](Validation.md)
  - [../qa/README.md](../qa/README.md)

### QA-012

- Decision: Run reports and comparison reports use one shared table-first
  information hierarchy.
- Status: accepted
- Date: 2026-07-09
- Why: the report must minimize cognitive load. A single-run summary and a
  multi-run comparison should present the same fields in the same order so
  comparison becomes a simple join, not a second reporting system.
- Consequences:
  - `summary.md` is the canonical single-run summary shape
  - `comparison.md` reuses the same row order and expands only by adding run
    columns
  - report rows remain stable: date, status, scenario, assertions, stages,
    logs, and screenshots
  - screenshots render inline in the report rather than being summarized by
    abstract comparison categories
  - logs are surfaced as direct links, not interpreted prose summaries
  - future report changes must preserve row-order stability unless an explicit
    decision changes the contract
- Replaces: earlier comparison concepts that emphasized bespoke output over a
  shared summary-table shape
- Source documents:
  - [Validation.md](Validation.md)
  - [../qa/README.md](../qa/README.md)

### QA-013

- Decision: Run selection remains operator-directed and non-interpretive.
- Status: accepted
- Date: 2026-07-09
- Why: the harness should surface reality precisely and let the operator decide
  what to inspect. It should not rank runs, auto-pick a “best” prior run, or
  impose a retention policy as part of the comparison contract.
- Consequences:
  - `--runs` lists the 10 most recent runs using each run's recorded start time
  - operators choose run IDs or explicit paths for view and comparison
  - a single selector resolves to the existing summary report rather than
    erroring out
  - comparison accepts two or three selected runs; it does not infer intent
    beyond what the operator selected
  - the harness does not auto-select “last known success” or any equivalent
    heuristic
- Replaces: implicit ideas about ranking, auto-selection, or harness-owned run
  retention behavior
- Source documents:
  - [Validation.md](Validation.md)
  - [Build.md](BuildandInstall.md)

### QA-014

- Decision: Report and comparison generation must degrade gracefully when
  artifacts are incomplete.
- Status: accepted
- Date: 2026-07-09
- Why: missing screenshots or path-shape differences are review facts, not
  reasons for the report viewer itself to fail.
- Consequences:
  - missing screenshot assets are reported inline in report/comparison output
  - remaining screenshots continue to render
  - path-based run selectors are supported alongside run IDs
  - report generation favors surfacing missing evidence over aborting on the
    first artifact problem
  - robustness of the viewer path is part of the validation-system contract
- Replaces: brittle comparison behavior that assumed all indexed screenshot
  assets were present and repo-local
- Source documents:
  - [Validation.md](Validation.md)
  - [../qa/README.md](../qa/README.md)

### QA-015

- Decision: Adopt the final block-structured, watch-face-specific scenario
  grammar built from `PREAMBLE`, `EXECUTE`, concrete `STEP`s, and `INCLUDE`.
- Status: accepted
- Date: 2026-07-10
- Why: the prior grammar mixed scenario names, emulator aliases, parser
  routing, matrix expansion, checkpoint identity, and artifact identity. That
  made scenario files hard to reason about and invited drift.
- Consequences:
  - `PREAMBLE` owns scenario metadata only
  - `EXECUTE` owns ordered work only
  - `STEP` means one concrete validation action with concrete values
  - `INCLUDE` concatenates another scenario's `EXECUTE` entries
  - artifact identity is generated from concrete step values
  - scenario names do not appear in screenshot identity
  - user-authored checkpoint ids, artifact ids, and generic alias labels are
    out of scope
  - the harness must show the resolved execution plan before running composed
    scenarios
  - `-n` / `--dry-run` prints the resolved execution plan and exits without
    executing
  - `--force` skips the confirmation prompt and executes
  - `--nuclear` remains explicit and has no short flag
  - the parser must conform to this grammar
- Replaces: `STEP <family> <checkpoint-id> [key=value ...]`, user-authored
  checkpoint ids, overloaded aliases such as `core`, and scenario/emulator
  alias name reuse.
- Source documents:
  - [../qa/QASystemDesign.md](../qa/QASystemDesign.md)
  - [Validation.md](Validation.md)

### QA-016

- Decision: Adopt STEPWISE mode for QA grammar and harness design work.
- Status: accepted
- Date: 2026-07-10
- Why: the prior implementation optimized for fast internal representation and
  generic extensibility despite explicit constraints for irreducible
  simplicity, watch-face-specific usability, and narrow scope.
- Mode contract:
  - start with the smallest concrete unit
  - prove what the unit is and is not before composing it
  - use product vocabulary, not parser vocabulary
  - do not introduce aliases, ids, presets, or configuration channels until
    the product need is explicit
  - keep each channel to one responsibility
  - derive artifact identity from concrete execution facts
  - stop when design terms require long explanation
  - no shortcut implementation for expediency
- Replaces: framework-shaped grammar design, hidden parser concepts, and
  opportunistic generality.
- Source documents:
  - [../qa/QASystemDesign.md](../qa/QASystemDesign.md)

### QA-017

- Decision: Scenario validation is exposed through the harness ABI.
- Status: accepted
- Date: 2026-07-10
- Why: operators should not need to know or call the Python implementation
  entrypoint directly to validate scenario grammar and composition.
- Consequences:
  - the public harness owns a scenario-validation command
  - validation reports whether the scenario parses, includes resolve, and
    concrete steps are valid
  - include cycles are validation failures
  - invalid `STEP` fields are validation failures
  - parser-validation fixtures remain focused on accepted and rejected grammar
    behavior
  - direct `python3 qa/runner.py ...` calls are implementation details, not the
    operator contract
- Replaces: treating parser validation as a Python-only maintainer command.
- Source documents:
  - [../qa/QASystemDesign.md](../qa/QASystemDesign.md)
  - [../qa/README.md](../qa/README.md)

### QA-018

- Decision: Classify harness commands as read-only or run-producing before
  bootstrap.
- Status: accepted
- Date: 2026-07-10
- Why: commands that inspect existing state must not create new run artifacts,
  while commands that execute validation must enter the run lifecycle
  deliberately.
- Consequences:
  - read-only commands exit before run bootstrap
  - read-only commands include help, run listing, report viewing, comparison,
    scenario validation, and dry-run plan printing
  - run-producing commands are the only commands that create `qa/qa-runs/<run-id>/`
  - `--runs` and `--compare` do not bootstrap merely because they touch run
    artifacts
  - command classification is explicit state, not incidental shell flow
- Replaces: implicit bootstrap behavior scattered across command branches.
- Source documents:
  - [../qa/QASystemDesign.md](../qa/QASystemDesign.md)
  - [Build.md](BuildandInstall.md)

### QA-019

- Decision: Every run-producing command that bootstraps must finish with
  operator-visible report closeout.
- Status: accepted
- Date: 2026-07-10
- Why: a validation run is not useful if the operator has to hunt for the
  summary after success or failure.
- Consequences:
  - summary generation belongs to run closeout
  - finalizer output must not be silently swallowed by shell redirection
  - successful and failed runs both surface the resulting summary
  - the operator should either see the summary or receive a friendly next-step
    command such as `open qa/qa-runs/<run-id>/summary.md` as a convenience for
    locating and opening the generated report
  - read-only commands that do not bootstrap do not generate new summaries
  - comparison and view commands remain read-only report consumers
- Replaces: creating `report.md` as a hidden artifact with no end-of-run
  operator guidance.
- Source documents:
  - [../qa/QASystemDesign.md](../qa/QASystemDesign.md)
  - [../qa/README.md](../qa/README.md)

### QA-021

- Decision: The canonical report payload is the singular summary-table model
  represented by `comparison.md`; JSON is its authoritative source of truth.
- Status: accepted
- Date: 2026-07-13
- Why: a single-run summary and a multi-run comparison are the same report
  structure with different numbers of run columns. Combining or rendering
  reports from Markdown would require reconstructing structure that JSON keeps
  explicit.
- Consequences:
  - `report.json` stores the canonical payload for one run
  - `summary.md` renders that payload with one run column
  - `comparison.json` stores the canonical payload for selected runs
  - `comparison.md` renders that payload with one column per run
  - `detailed-run-report.md` remains the diagnostic, run-specific report
  - terminal closeout renders a human-readable text summary from the same
    canonical payload
  - comparison combines JSON payloads before Markdown rendering; it never
    combines or parses existing Markdown reports
  - `view-run` reads an existing `summary.md` or generates it from the run's
    canonical JSON payload and stores it in that run directory
- Replaces: separate legacy single-run and comparison report schemas, and
  closeout output that points at the detailed report as the summary.
- Source documents:
  - [ReportGeneration.md](ReportGeneration.md)
  - [../qa/README.md](../qa/README.md)

### QA-020

- Decision: Artifact identity is owned by the individual concrete step.
- Status: accepted
- Date: 2026-07-10
- Why: the revised grammar makes a concrete `STEP` the smallest executable
  validation unit. Screenshots, logs, indexes, summaries, and comparisons must
  attach to that unit rather than to scenario names, profiles, families, or
  checkpoint ids.
- Consequences:
  - artifact identity is generated from normalized concrete `STEP` values
  - included scenarios contribute their concrete steps without preserving a
    separate composite identity layer
  - scenario names may organize execution but do not identify screenshots
  - checkpoint ids, families, profiles, aliases, and matrix expansions are not
    artifact keys
  - report and comparison code must be updated where it still joins artifacts
    through old checkpoint or matrix fields
  - summaries should help the operator inspect step-level outcomes without
    exposing harness-internal jargon
- Replaces: artifact organization by scenario/checkpoint/family/profile-shaped
  keys.
- Source documents:
  - [../qa/QASystemDesign.md](../qa/QASystemDesign.md)

### QA-021

- Decision: The zsh harness is a sourced script family with explicit shared
  state, not a chain of long argument-passing functions.
- Status: accepted
- Date: 2026-07-10
- Why: the harness is intentionally implemented as zsh modules sourced into one
  execution context. Passing many repeated arrays and state values through
  every function obscures ownership and works against that design.
- Consequences:
  - CLI parse state lives in explicit globals
  - shared mutable state should be declared up front in one state owner
  - functions receive arguments only for values that genuinely differ from the
    established shared state
  - empty-safe behavior preserves defaults when user input is omitted
  - zsh shorthand and idioms should be used consistently across shell modules
  - maintainability means a new maintainer can find state ownership without
    tracing argument trains through the whole harness
- Replaces: over-parameterized shell function chains and scattered global
  declarations.
- Source documents:
  - [Build.md](BuildandInstall.md)
  - [../qa/README.md](../qa/README.md)

### QA-022

- Decision: Shell/Python connectivity must use a structured, explicit bridge
  with no shell `eval`.
- Status: accepted
- Date: 2026-07-10
- Why: Python should own structured data creation, but zsh should not execute
  Python-emitted shell assignment text. The bridge must be readable,
  allowlisted, and safe to maintain.
- Consequences:
  - Python bootstrap output stops being shell code
  - JSON is sufficient as the Python-side structured representation
  - zsh does not hand-parse JSON and does not require an external dependency
    such as `jq`
  - Python may convert JSON to a strict allowlisted stream for zsh consumption
  - zsh applies known keys through explicit `case` handling
  - unknown bridge keys are rejected
  - empty array values preserve previously established default globals
  - shell `eval` is forbidden in the harness bridge
- Replaces: `eval` of Python-emitted shell assignments and ad hoc CSV array
  reconstruction.
- Source documents:
  - [Build.md](BuildandInstall.md)
  - [../qa/QASystemDesign.md](../qa/QASystemDesign.md)

### QA-023

- Decision: Maintainability documentation must grow by narrow ownership, not by
  expanding a single giant build document.
- Status: accepted
- Date: 2026-07-10
- Why: implementation decisions are only maintainable when a future maintainer
  can find the contract, the state owner, and the execution handoff without
  reverse-engineering shell and Python code.
- Consequences:
  - the decision ledger owns accepted architecture decisions
  - the tortoise implementation plan owns ordered implementation slices and
    handoff criteria
  - code should use expressive names and short invariant comments where the
    ownership rule is not obvious from the function body
  - `QABuildSystem.md` or equivalent build documentation should not absorb all
    harness design detail
  - new maintainer guidance should be concise and linked to the canonical owner
    rather than duplicated across documents
- Replaces: relying on undocumented Python and shell implementation detail as
  the maintainer guide.
- Source documents:
  - [Build.md](BuildandInstall.md)
  - [../qa/README.md](../qa/README.md)
  - [../qa/QA_Tortoise_Implementation_Plan.md](../qa/QA_Tortoise_Implementation_Plan.md)

### QA-024

- Decision: Collapse the public QA design document into the QA subsystem
  manual and the scenario grammar guide.
- Status: accepted
- Date: 2026-07-10
- Why: `qa/README.md` and `qa/QASystemDesign.md` were converging on the same
  job. Keeping both would create duplicate implementation guidance and make the
  validation docs harder to maintain.
- Consequences:
  - `docs/Validation.md` becomes the contributor-facing validation front door
  - `qa/README.md` owns QA System architecture, lifecycle, command classes,
    artifact flow, report flow, and implementation layout
  - `qa/QAScenarioGrammar.md` owns scenario grammar, concrete step shape,
    artifact identity, and scenario-authoring guidance
  - old decision entries are preserved as historical records rather than
    rewritten as current navigation
- Replaces: `qa/QASystemDesign.md` as a separate public implementation document.
- Source documents:
  - [../docs/Validation.md](../docs/Validation.md)
  - [../qa/README.md](../qa/README.md)
  - [../qa/QAScenarioGrammar.md](../qa/QAScenarioGrammar.md)

### QA-025

- Decision: Classify public harness commands by responsibility as
  `qa-inspection`, `scenario-exec`, and `env-prep`.
- Status: accepted
- Date: 2026-07-13
- Why: the earlier read-only/run-producing/direct split was technically useful
  but did not process cleanly once scenario validation and dry-run planning were
  recognized as execution-adjacent rather than scenario-executing. The command
  family should describe the responsibility being invoked, not the incidental
  side effects of one implementation stage.
- Consequences:
  - `qa-inspection` owns run listing, existing-run viewing, comparison,
    scenario validation, and dry-run plan inspection
  - `scenario-exec` owns only scenario execution actions:
    `run-scenario` and `force-scenario`
  - `env-prep` owns environment preparation and direct Pebble actions,
    including `--help`
  - `validate` and `dryrun` inspect scenario execution inputs without
    executing a scenario
  - the term `bootstrap` is not part of the public command model
  - command parsing records the selected family and action explicitly before
    validation or dispatch
- Replaces: read-only/direct/run-producing as the public taxonomy, and
  bootstrap-centered command wording.
- Source documents:
  - [../qa/README.md](../qa/README.md)
  - [../qa/stages/parsearguments.sh](../qa/stages/parsearguments.sh)
  - [../qa/lib/validate.sh](../qa/lib/validate.sh)

### QA-026

- Decision: `qa-inspection` has one shell entrypoint and one grouped Python
  entrypoint.
- Status: accepted
- Date: 2026-07-13
- Why: duplicate runner commands and shell-side dispatch paths made inspection
  behavior harder to reason about and invited stale compatibility code. The
  public harness should hand inspection requests to Python once using normal
  command-line arguments.
- Consequences:
  - shell invokes `runner.py qa-inspection <inspection-command> ...`
  - inspection subcommands are `runs`, `view-run`, `compare`, `validate`, and
    `dryrun`
  - top-level Python aliases for those inspection actions are not part of the
    supported ABI
  - `view-run` is implemented as the one-selector form of compare behavior, so
    report formatting changes stay shared
  - `qa-inspection` must not create a new `qa/qa-runs/<run-id>/`
  - inspection command status is relayed as the harness exit status
- Replaces: duplicate top-level runner commands such as scenario validation,
  dry-run plan printing, run listing, and run comparison aliases.
- Source documents:
  - [../qa/stages/inspectqa.sh](../qa/stages/inspectqa.sh)
  - [../qa/runner.py](../qa/runner.py)

### QA-027

- Decision: Once control crosses from zsh to Python for a command family, state
  transfer uses argv semantics, not a payload or hidden bridge.
- Status: accepted
- Date: 2026-07-13
- Why: the harness is narrow enough that building intermediate payloads for
  command selection is fragile and over-engineered. The shell has already
  parsed and validated the public request, so Python should receive the action
  and arguments directly.
- Consequences:
  - do not build JSON or environment payloads merely to tell Python which
    inspection or scenario-execution command to run
  - do not revive Python-emitted shell state as the handoff model
  - the shell/Python handoff for `qa-inspection` and `scenario-exec` is
    one-way command invocation
  - payload-shaped code may still exist only as transitional scenario-exec
    scaffolding until the Python execution engine replaces it
  - removal of payload/state bridge code should happen as the owning command
    family is migrated
- Replaces: payload handoff and bootstrap-record transfer as the intended
  shell/Python command-dispatch model.
- Source documents:
  - [../qa/stages/inspectqa.sh](../qa/stages/inspectqa.sh)
  - [../qa/executescenarios.md](../qa/executescenarios.md)
  - [../qa/runner.py](../qa/runner.py)

### QA-028

- Decision: Scenario execution logging and reporting belong to Python alone.
- Status: accepted
- Date: 2026-07-13
- Why: splitting scenario lifecycle evidence between shell and Python creates
  ambiguous ownership for command logs, step results, screenshots, run status,
  and reports. Scenario execution is the validation core and should have one
  writer for its artifacts.
- Consequences:
  - Python owns scenario loading, include expansion, run directory creation,
    command execution, screenshot capture, command logs, per-command logs,
    `run.json`, `report.json`, `summary.md`, and `detailed-run-report.md`
  - shell may parse, validate, route, perform wrapper-owned reset and cleanup,
    invoke Python, and relay status
  - shell must not own scenario step logging, command logging, test logging,
    screenshot indexing, report assembly, or scenario state transfer
  - legacy shell lifecycle helpers are removal targets once
    `runner.py scenario-exec ...` is implemented
  - `runner.py scenario-exec run-scenario <name>` and
    `runner.py scenario-exec force-scenario <name>` are the target Python ABI
- Replaces: split scenario logging where shell records scenario stages and
  Python later finalizes reports from shell state.
- Source documents:
  - [../qa/executescenarios.md](../qa/executescenarios.md)
  - [../qa/stages/executescenarios.sh](../qa/stages/executescenarios.sh)
  - [../qa/python/runtime.py](../qa/python/runtime.py)

### QA-029

- Decision: Scenario reset and cleanup are symmetrical `aag-build-qa.sh`
  wrapper responsibilities around Python-owned scenario execution.
- Status: accepted
- Date: 2026-07-13
- Why: reset and cleanup are the same class of responsibility: top-level
  harness lifecycle around the scenario execution call. Hiding either inside
  selected-test execution or Python scenario execution makes the control flow
  harder to audit and blurs wrapper lifecycle with scenario artifact ownership.
- Consequences:
  - `aag-build-qa.sh` owns the wrapper sequence:
    parse, validate, reset, invoke Python scenario execution, cleanup, exit
  - reset is the visible precondition before Python scenario execution
  - cleanup is the visible postcondition after Python scenario execution
  - `handle_qa_plan_execution` should invoke scenario execution, not prepare or
    clean up the environment implicitly
  - Python owns the scenario execution span between reset and cleanup
  - Python owns scenario artifacts; shell-owned cleanup must not assemble or
    mutate scenario evidence
- Replaces: resetting emulator state inside `handle_qa_plan_execution` and treating
  cleanup as an incidental or Python-owned scenario concern.
- Source documents:
  - [../aag-build-qa.sh](../aag-build-qa.sh)
  - [../qa/stages/reset.sh](../qa/stages/reset.sh)
  - [../qa/stages/executescenarios.sh](../qa/stages/executescenarios.sh)

### QA-030

- Decision: The harness exit status is the command family's exit status and
  must be relayed consistently.
- Status: accepted
- Date: 2026-07-13
- Why: reports are the evidence source for scenario execution, but the wrapper
  still participates in normal command-line composition. Pipes, scripts, and
  local automation need the process status to reflect the command that was
  requested.
- Consequences:
  - `qa-inspection`, `env-prep`, and `scenario-exec` statuses are all
    meaningful
  - cleanup and finalization must not obscure a failing primary command
  - when the primary command succeeds and cleanup/finalization fails, the
    wrapper may return the cleanup/finalization failure
  - scenario-execution reports remain the detailed evidence, while the process
    exit status remains the shell contract
- Replaces: treating the scenario report as sufficient reason to de-emphasize
  the wrapper exit code.
- Source documents:
  - [../aag-build-qa.sh](../aag-build-qa.sh)
  - [../qa/executescenarios.md](../qa/executescenarios.md)

### QA-031

- Decision: Emulator selection moves to scenario-level grammar. Steps do not
  own emulator selection.
- Status: accepted
- Date: 2026-07-13
- Why: step-level emulator fields force the execution layer to infer which
  emulators must be launched, installed, reset, and cleaned up from arbitrary
  step mixtures. That leaks lifecycle planning into per-step execution, makes
  scenario startup harder to reason about, and weakens screenshot determinism.
- Consequences:
  - scenario metadata must declare the emulator set once for the whole scenario
  - execution can plan emulator launch, install, reset, cleanup, and artifact
    expectations before the first step runs
  - steps execute within the scenario's declared emulator scope
  - scenarios that require more than one emulator express that at the scenario
    level, not by scattering emulator ownership across steps
  - step-level `emulator` ownership is removed from the grammar
- Replaces: step-level emulator selection as the source of scenario execution
  scope.
- Source documents:
  - [../qa/QAScenarioGrammar.md](../qa/QAScenarioGrammar.md)
  - [../qa/executescenarios.md](../qa/executescenarios.md)
  - [../qa/python/scenarios.py](../qa/python/scenarios.py)

### QA-032

- Decision: Adopt a strict `suite -> scenario -> step` ownership model.
- Status: accepted
- Date: 2026-07-13
- Why: treating scenario composition as though a scenario were a step collapses
  ownership boundaries and makes recursion harder to reason about. Suites need
  ordered composition, scenarios need concrete emulator-scoped execution, and
  steps need concrete capability values.
- Consequences:
  - a suite may include scenarios and other suites
  - a scenario may include only steps
  - a step never includes anything
  - suite recursion is legal, but suite cycles are validation errors
  - the top-level release target is a suite, not a scenario
- Replaces: scenario-level include composition that blurred suite ownership with
  scenario execution.
- Source documents:
  - [../qa/QAScenarioGrammar.md](../qa/QAScenarioGrammar.md)
  - [../qa/python/scenarios.py](../qa/python/scenarios.py)
  - [../qa/plans/pre-release-gate.suite](../qa/plans/pre-release-gate.suite)

### QA-033

- Decision: Summary and comparison reports are operator-facing evidence tables.
  They use `QA Plan Executed` terminology, keep aggregate screenshot counts at
  the top, and render each screenshot beside its step detail.
- Status: accepted
- Date: 2026-07-14
- Why: operators need the plan result and product evidence first. Harness
  stages, aggregate step counts, and passed assertion inventories add noise to
  successful runs.
- Consequences:
  - the top table rows are Date, Status, QA Plan Executed, and Screenshots
  - screenshot counts use `Expected` and `Captured` values with emphasis
  - step detail includes capability, emulator, theme, inputs, and its inline
    screenshot
  - failed assertions appear only when present and below step evidence
  - logs and JSON render as local Markdown links so operators can open them
    directly from the report
  - inline screenshots may use embedded image tags when that is the most
    reliable way to render the captured evidence in place
  - artifact identities use one underscore between components
- Replaces: harness-first summary rows, report-level screenshot galleries, and
  HTML artifact links.
- Source documents:
  - [../qa/python/comparison.py](../qa/python/comparison.py)
  - [../qa/python/scenarios.py](../qa/python/scenarios.py)

### QA-034

- Decision: The harness records validation facts directly and does not use a
  generic assertion framework or interpret manual signoff criteria.
- Status: accepted
- Date: 2026-07-14
- Why: artifact presence, screenshot counts, step counts, and missing-step
  details are concrete run facts. A generic assertion abstraction obscures
  those facts and implies release-quality interpretation that belongs to the
  operator.
- Consequences:
  - unused assertion transport and assertion report fields are deleted
  - screenshot expectations are represented by `SCREENSHOTS_PER_STEP`
  - reports show enabled state, expected and captured counts, and missing step
    identities directly
  - manual signoff policy is not emitted by the harness
  - policy constants use `Final`; functions compute results from inputs
- Replaces: QA-021 assertion and narrow-release-assertion consequences where
  they describe the active implementation.
- Source documents:
  - [../qa/python/runtime.py](../qa/python/runtime.py)
  - [../qa/python/scenarios.py](../qa/python/scenarios.py)

### QA-035

- Decision: `runner.py scenario-exec` is the sole supported scenario execution
  path for release. Transitional bridge commands and their implementation are
  not part of the harness contract.
- Status: accepted
- Date: 2026-07-14
- Why: the active execution path already owns scenario loading, step
  execution, logging, screenshot capture, finalization, and reporting. Keeping
  the old bootstrap, emit-steps, environment-finalize, and report-resolution
  bridges creates dead entrypoints and duplicate lifecycle models.
- Consequences:
  - `bootstrap`, `bootstrap_records`, `emit-steps`, and `finalize_from_env` are
    deletion candidates before GitHub release
  - `resolve_report_path` is removed when no supported caller remains
  - release validation covers the public shell entrypoint and
    `runner.py scenario-exec` only
  - documentation must describe current modules and commands, not transitional
    bridge names
- Source documents:
  - [../qa/runner.py](../qa/runner.py)
  - [../qa/QAHarnessImplementationFlow.md](../qa/QAHarnessImplementationFlow.md)

### QA-036

- Decision: QA documentation has one current implementation map. `qa/README.md`
  owns the subsystem manual, `qa/QAScenarioGrammar.md` owns authoring, and
  `qa/QAHarnessImplementationFlow.md` owns function-level implementation
  flow and deletion audits. General build and validation docs link to those
  owners instead of repeating QA file inventories.
- Status: accepted
- Date: 2026-07-14
- Why: the shell-to-Python migration removed the former `qa/stages/`,
  `qa/tests/`, and several `qa/lib/` modules, while multiple docs still name
  them as active code. Repeated inventories have drifted from the repository.
- Consequences:
  - stale paths and obsolete command names are documentation defects
  - current docs describe `argumentparser.sh`, `commandhandler.sh`,
    `pebbleadapter.sh`, `runtimehelper.sh`, `runtimevalidator.sh`, and the
    Python modules that are actually present
  - release-prep doc cleanup removes duplicated QA implementation inventories
    from `docs/Build.md`, `docs/SourceMap.md`, and `docs/Validation.md`
  - deleted transitional docs and fixtures are not restored for compatibility
- Source documents:
  - [../qa/README.md](../qa/README.md)
  - [../qa/QAScenarioGrammar.md](../qa/QAScenarioGrammar.md)
  - [../qa/QAHarnessImplementationFlow.md](../qa/QAHarnessImplementationFlow.md)

### QA-037

- Decision: The canonical Markdown renderer accepts a list of canonical
  payloads and a report title. A one-payload summary and an N-payload
  comparison use the same renderer; one-payload compare routes to `view-run`
  and returns the existing summary without creating a comparison artifact.
- Status: accepted
- Date: 2026-07-14
- Why: summary and comparison reports share one table contract. Payload count
  determines columns; the call-site title determines only the report heading.
- Consequences:
  - report formatting has one implementation
  - `report.json` remains the durable input for report regeneration
  - one-selector inspection does not create a new comparison directory

### QA-038

- Decision: `execute_next_pebble_action()` retains its environment-preparation
  guard as defensive protection, even though current callers already belong to
  that command path.
- Status: accepted
- Date: 2026-07-14
- Why: the guard prevents accidental execution if the helper is called from a
  future or unexpected command path.

### QA-039

- Decision: Compile database generation is optional enrichment. Failure after
  the fallback attempt does not fail QA execution.
- Status: accepted
- Date: 2026-07-14
- Why: the compile database is useful for development tooling but is not part
  of the QA run contract or report source of truth.
- Consequences:
  - `generate_compile_commands_db()` may report the build result while leaving
    compile database generation unsuccessful
  - QA execution and validation remain independent of compile database output

### QA-040

- Decision: Shell review uses zsh semantics exclusively. The harness requires
  zsh, and unquoted array expansion in zsh loop contexts is valid for the
  current array contracts.
- Status: accepted
- Date: 2026-07-14
- Why: Bash portability concerns are outside the harness contract and create
  false cleanup findings.
- Consequences:
  - `for item in $ARRAY` is not treated as a defect solely because it is not
    Bash-portable
  - `return $?` after a guarded command is a style consideration, not a shell
    correctness finding
  - shell audits must use zsh syntax, expansion, and status semantics

### QA-041

- Decision: The Python parser owns the supported emulator policy and rejects
  emulators outside the current harness set at the grammar boundary.
- Status: accepted
- Date: 2026-07-15
- Why: emulator selection is part of the scenario contract. Accepting an
  unknown emulator during parsing would defer a known invalid request to the
  execution layer.
- Consequences:
  - `SUPPORTED_EMULATORS` is a `Final` `frozenset`
  - scenario and suite resolution use the same validated emulator policy
  - adding an emulator is one explicit policy change in Python
- Source:
  - [../qa/python/scenarios.py](../qa/python/scenarios.py)

### QA-042

- Decision: Display-mode values are validated during parsing and execution
  translates only values from the immutable display-mode policy.
- Status: accepted
- Date: 2026-07-15
- Why: a manually constructed plan must not bypass the grammar and cause an
  arbitrary display value to be sent to the emulator.
- Consequences:
  - unsupported display values fail with a validation error
  - `DISPLAY_MODE_VALUES` is a `Final` `MappingProxyType`
  - execution uses the policy lookup directly and has no unknown-value
    pass-through
- Source:
  - [../qa/python/scenarios.py](../qa/python/scenarios.py)
  - [../qa/python/execution.py](../qa/python/execution.py)
  - [../qa/tests/test_harness_correctness.py](../qa/tests/test_harness_correctness.py)

### QA-043

- Decision: Scenario and suite file types are resolved through one immutable
  suffix policy, and a bare target name is rejected when both file types exist.
- Status: accepted
- Date: 2026-07-15
- Why: file suffix is the information that distinguishes a scenario from a
  suite. Treating both as the same bare name would silently execute the wrong
  target.
- Consequences:
  - `.scenario` and `.suite` are the only accepted plan suffixes
  - `_resolve_target_path()` returns the resolved path and semantic target kind
  - callers provide an explicit suffix when a name is ambiguous
  - `PlanRef`, `SuiteRef`, and numeric kind tags are not part of the model
- Source:
  - [../qa/python/scenarios.py](../qa/python/scenarios.py)

### QA-044

- Decision: A QA run has one aggregate `commands.log`; the harness does not
  create one log file per command.
- Status: accepted
- Date: 2026-07-15
- Why: the aggregate log preserves complete command output while avoiding
  artifact directories that grow linearly with every plan step.
- Consequences:
  - command execution appends to the run-level `commands.log`
  - per-command log naming, labels used only for naming, and command-count
    artifacts are not part of the run contract
  - reports expose the aggregate log path
- Source:
  - [../qa/python/runtime.py](../qa/python/runtime.py)
  - [../qa/lib/runtimehelper.sh](../qa/lib/runtimehelper.sh)

### QA-045

- Decision: Parser representation remains limited to the small parsed plan
  and parsed suite records required by the grammar and include resolution.
- Status: accepted
- Date: 2026-07-15
- Why: fixture-backed tests prove the narrow grammar and include-cycle behavior;
  removing the active records would collapse parsing and resolution into one
  harder-to-read responsibility without removing behavior.
- Consequences:
  - `ParsedPlanFile` and `ParsedSuiteFile` remain active parser outputs
  - `load_suite_file()`, `PlanRef`, and `SuiteRef` remain deleted/absent
  - parser simplification must preserve named/direct targets, validation
    errors, and include-cycle detection covered by fixtures
- Source:
  - [../qa/python/scenarios.py](../qa/python/scenarios.py)
  - [../qa/tests/test_harness_correctness.py](../qa/tests/test_harness_correctness.py)

### QA-046

- Decision: Comparison accepts one to five run selectors.
- Status: accepted
- Date: 2026-07-16
- Why: one comparison should remain readable and useful for operator review.
  Five runs provide a practical comparison limit without imposing an
  unbounded, horizontally unwieldy report.
- Consequences:
  - Python enforces the same one-to-five selector contract as the shell
    entrypoint
  - cached comparisons return the existing `comparison.md` path
  - expanding the limit requires an explicit product decision
- Replaces: the earlier three-run shell limit and Python's unbounded selector
  acceptance.
- Source:
  - [../qa/python/comparison.py](../qa/python/comparison.py)
  - [../qa/lib/runtimevalidator.sh](../qa/lib/runtimevalidator.sh)

### QA-047

- Decision: Summary and comparison reports show one row per concrete step, with
  step detail and its screenshot kept in the corresponding run-output cell.
- Status: accepted
- Date: 2026-07-16
- Why: a report-level screenshot gallery overwhelms operators on large plans.
  Keeping evidence beside its step preserves scanability for three-step and
  one-hundred-step plans alike.
- Consequences:
  - the top table shows Date, Status, QA Plan Executed, Screenshots, and Steps
    Run
  - screenshot counts use `Expected` and `Captured` values
  - each step row shows capability, emulator, theme, input values, and its
    inline screenshot when one was captured
  - Logs and JSON are detail rows rendered as Markdown links using local paths
  - inline screenshots use image tags so the Markdown report displays the
    captured evidence in place; this supersedes QA-033 only where it said
    image tags were prohibited
  - the shared renderer preserves the source step order and handles both
    single-run summaries and multi-run comparisons
- Replaces: aggregate step-detail output and report-level screenshot placement.
- Source:
  - [../qa/python/report.py](../qa/python/report.py)
  - [../qa/python/comparison.py](../qa/python/comparison.py)

### QA-048

- Decision: Parser policy is declared once as immutable module-level data in
  `scenarios.py` and enforced at the grammar boundary.
- Status: accepted
- Date: 2026-07-16
- Why: accepted vocabulary and file-type rules are decisions, not computed
  results. Dispersing them through parser branches creates policy drift and
  makes valid input harder to see and review.
- Consequences:
  - policy constants use `Final` with an immutable value such as `frozenset`
    or `MappingProxyType`
  - parser functions validate input against those policies while parsing
  - functions remain responsible for computations and resolution from parsed
    inputs; they do not encode a second copy of policy
  - current parser policies include supported capabilities, screenshot
    policies, display modes, emulators, capability field order, and accepted
    `.scenario`/`.suite` suffixes
  - adding a supported value is an explicit policy change in one named
    constant, with fixture coverage for the resulting grammar behavior
- Replaces: scattered literal policy checks and execution-layer pass-through
  of values that the parser could reject earlier.
- Source:
  - [../qa/python/scenarios.py](../qa/python/scenarios.py)
  - [../qa/python/execution.py](../qa/python/execution.py)
  - [../qa/tests/test_harness_correctness.py](../qa/tests/test_harness_correctness.py)

## Open Follow-On Decisions

- Whether the shell wrapper remains permanent or is later removed.
- How to retire remaining old matrix/checkpoint/profile code as each narrow
  slice reaches it.
- How far CI hardening should go beyond the current local-first contract.
- How to retire the remaining legacy scenario-exec bridge commands after
  `runner.py scenario-exec ...` owns the full execution lifecycle.
