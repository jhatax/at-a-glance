# QA Harness Decision Ledger

This ledger records deliberate architecture decisions for the At A Glance QA
harness and Validation System.

Use it to answer four questions:

- what decision was made
- why it was made
- what it replaces
- what must remain true going forward

This is a decision ledger, not an implementation map.

- Canonical validation contract: [Validation.md](Validation.md)
- Canonical build contract: [Build.md](Build.md)
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
  - [Build.md](Build.md)
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
  - [Build.md](Build.md)
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
  - [Build.md](Build.md)

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
  - [Build.md](Build.md)

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
  appear only for `release-core` and `release-full`.
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
  - `report.md` is the canonical single-run summary shape
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
  - [Build.md](Build.md)

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
  - [Build.md](Build.md)

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
    command such as `open qa/qa-runs/<run-id>/report.md`
  - read-only commands that do not bootstrap do not generate new summaries
  - comparison and view commands remain read-only report consumers
- Replaces: creating `report.md` as a hidden artifact with no end-of-run
  operator guidance.
- Source documents:
  - [../qa/QASystemDesign.md](../qa/QASystemDesign.md)
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
  - [Build.md](Build.md)
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
  - [Build.md](Build.md)
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
  - [Build.md](Build.md)
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

## Open Follow-On Decisions

- Whether the shell wrapper remains permanent or is later removed.
- How to retire remaining old matrix/checkpoint/profile code as each narrow
  slice reaches it.
- How to reshape reports after artifact identity is fully step-owned.
- How far CI hardening should go beyond the current local-first contract.
