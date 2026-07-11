# QA Harness Tortoise Implementation Plan

## Mode Contract

- Mode: TORTOISE IMPLEMENTATION.
- Work style: one narrow slice at a time.
- Source of truth: `qa/README.md`, `qa/QAScenarioGrammar.md`, and `qa/QA_Harness_Decision_Ledger.md`.
- Boundary: implement only the named slice unless the user explicitly expands scope.
- Stop rule: after each slice is implemented and validated, stop and report the handoff state.
- No legacy support: remove obsolete matrix/checkpoint/profile behavior when a slice reaches that boundary.
- No shell `eval`.
- No long argument trains: use intentional zsh globals declared in one shared state file.
- No parser grammar redesign while working shell/runtime/report slices.
- No report rewrite until artifact identity and scenario execution are coherent.

## Product Decisions

- The harness is a sourced zsh system with one shared execution context.
- Globals are acceptable when they are explicit, declared up front, and owned.
- User-input defaults are established once and replaced only when user input or resolved scenario state differs.
- Empty bootstrap values preserve existing defaults.
- Python owns structured scenario parsing and bootstrap data creation.
- zsh owns CLI parsing, command classification, lifecycle dispatch, and explicit state application.
- The zsh/Python bridge must use a constrained internal protocol, not shell assignment text.
- JSON is the Python-side structured format.
- zsh should consume a strict allowlisted stream derived from JSON, such as TSV records emitted by Python.

## Slice 1: Centralize State Declarations

### Inputs

- `aag-build-qa.sh`
- `qa/lib/runtime.sh`
- current CLI globals
- current runtime globals
- current command classification globals

### Coding Outcome

- Add `qa/lib/state.sh`.
- Move intentional shared globals into `qa/lib/state.sh`.
- Use `typeset -g` and `typeset -ga` consistently.
- Source `qa/lib/state.sh` before modules that read or mutate shared state.
- Keep behavior unchanged.

### Guardrails

- Do not change parser behavior.
- Do not change scenario execution behavior.
- Do not change report behavior.
- Do not introduce `eval`.
- Do not introduce function argument trains.

### Validation Output

- `zsh -n aag-build-qa.sh` passes.
- Read-only commands still exit before bootstrap:
  - `--help`
  - `--runs`
  - `--view`
  - `--compare`
  - `--validate-scenario`
  - `-n` / `--dry-run`
- No new `qa/qa-runs/<run-id>/` directory is created by read-only commands.

### Handoff

- State ownership is centralized.
- Existing behavior is unchanged.
- Next slice can replace bootstrap state transfer without hunting scattered globals.

## Slice 2: Remove Shell `eval` From Bootstrap

### Inputs

- `qa/python/runtime.py`
- `qa/runner.py`
- `qa/lib/runtime.sh`
- `qa/lib/state.sh`

### Coding Outcome

- Stop emitting shell assignment text from Python bootstrap.
- Emit structured bootstrap state from Python.
- Add a strict zsh bootstrap state reader.
- Apply only known keys through an explicit `case`.
- Reject unknown bootstrap keys.
- Preserve defaults when array values are empty.

### Guardrails

- No shell `eval`.
- No hand-parsed JSON in zsh.
- No new external dependency such as `jq`.
- Use Python for JSON parsing or JSON-to-TSV emission.
- zsh assignment must remain explicit and allowlisted.

### Validation Output

- `zsh -n aag-build-qa.sh` passes.
- `python3 -m py_compile qa/runner.py qa/python/runtime.py` passes.
- A bootstrapped run sets:
  - `QA_RUN_ID`
  - artifact paths
  - `QA_CAPTURE_SCREENSHOTS`
  - `EMULATORS` when provided
  - `TESTS` when provided
- Empty `EMULATORS` / `TESTS` values preserve defaults.
- `rg -n "eval" aag-build-qa.sh qa/lib qa/stages qa/tests` returns no runtime bridge `eval`.

### Handoff

- zsh/Python state transfer is explicit and safe.
- Next slice can introduce current-step globals without inheriting unsafe bridge behavior.

## Slice 3: Introduce Current-Step Globals

### Inputs

- `qa/lib/state.sh`
- `qa/lib/runtime.sh`
- parser step output from `qa/python/scenarios.py`

### Coding Outcome

- Add `STEP_*` globals in `qa/lib/state.sh`, including:
  - `STEP_CAPABILITY`
  - `STEP_ARTIFACT_IDENTITY`
  - `STEP_EMULATOR`
  - `STEP_DISPLAY`
  - weather fields
  - battery fields
  - health fields
- Add a function to clear current step state.
- Add a function to load one concrete step into `STEP_*`.

### Guardrails

- No long argument lists.
- No matrix globals for concrete scenario steps.
- No artifact/report changes yet.

### Validation Output

- Step state can be cleared deterministically.
- One concrete parsed step can populate `STEP_*`.
- Invalid or unknown step fields are rejected by existing parser validation.

### Handoff

- Scenario execution has a shared current-step state model.
- Next slice can change Python step emission and shell step loading together.

## Slice 4: Change Python Step Emission

### Inputs

- `qa/runner.py`
- `qa/python/scenarios.py`
- parser validation fixtures

### Coding Outcome

- Replace old `emit-steps` output shape.
- Emit concrete step records from parsed scenario steps.
- Include `artifact_identity` and normalized concrete fields.
- Use a strict, documented internal record shape.

### Guardrails

- Do not reintroduce checkpoint ids.
- Do not emit matrix CSV fields.
- Do not emit family/core/profile aliases.
- Keep parser grammar unchanged.

### Validation Output

- Emitted steps from `qa/fixtures/scenarios/run-them-all.scenario` are flat and ordered.
- Included scenario steps preserve their concrete artifact identities.
- Include cycles still fail.
- Invalid STEP fields still fail.

### Handoff

- Shell can consume one concrete step at a time.
- Next slice can add concrete capability runners.

## Slice 5: Add Concrete Capability Runners

### Inputs

- `qa/tests/weather.sh`
- `qa/tests/battery.sh`
- `qa/tests/health.sh`
- `qa/lib/appmessage.sh`
- `qa/lib/pebble.sh`
- `STEP_*` globals

### Coding Outcome

- Add one concrete-step runner per supported capability:
  - `qa_test_weather_step_run`
  - `qa_test_battery_step_run`
  - `qa_test_health_step_run`
- Each runner reads `STEP_*`.
- Each runner executes one concrete validation action.
- Each runner captures the artifact for that concrete step when screenshots are enabled.

### Guardrails

- No matrix loops for scenario execution.
- No 8-10 argument function chains.
- No checkpoint ids.
- No legacy smoke/profile behavior.

### Validation Output

- Shell syntax passes.
- Each runner has one obvious capability-specific path.
- Dry or stubbed validation shows the expected command sequence for one concrete step where possible.

### Handoff

- Concrete capability execution exists.
- Next slice can switch scenario execution to dispatch through `STEP_CAPABILITY`.

## Slice 6: Switch Scenario Execution to Concrete Steps

### Inputs

- `qa/lib/runtime.sh`
- `qa/runner.py emit-steps`
- concrete capability runners

### Coding Outcome

- Update `qa_run_selected_scenario` to:
  - read one concrete step record
  - clear current step state
  - load `STEP_*`
  - dispatch by `STEP_CAPABILITY`
- Remove old scenario matrix routing.

### Guardrails

- No legacy matrix execution for scenarios.
- No checkpoint id path.
- No parser redesign.
- No report changes in this slice.

### Validation Output

- A grammar-valid scenario expands and dispatches concrete steps in order.
- Unknown capability fails clearly.
- Read-only scenario validation remains read-only.

### Handoff

- Scenario execution aligns with final grammar.
- Next slice can repair artifact identity and screenshot indexing.

## Slice 7: Rebase Artifacts on Step Identity

### Inputs

- `qa/lib/runtime.sh`
- screenshot capture functions
- screenshot index writer
- `STEP_ARTIFACT_IDENTITY`

### Coding Outcome

- Screenshot filenames start from `STEP_ARTIFACT_IDENTITY`.
- Screenshot index records use step identity as the join key.
- Scenario name remains run context, not artifact identity.
- Remove checkpoint id from artifact identity.

### Guardrails

- Do not include dates, run ids, scenario names, or free-form labels in step identity.
- Missing screenshots should remain visible evidence, not renderer failures.
- No report rewrite beyond what is necessary to keep index parsing coherent.

### Validation Output

- One concrete weather step produces the expected artifact identity.
- Screenshot index links the captured artifact to the step identity.
- No old checkpoint id columns are required for new scenario artifacts.

### Handoff

- Artifact identity is coherent.
- Report work can safely use step identity as the primary key.

## Slice 8: Clean Test Files Into One Family Shape

### Inputs

- `qa/tests/weather.sh`
- `qa/tests/battery.sh`
- `qa/tests/health.sh`
- `qa/tests/smoke.sh`

### Coding Outcome

- Remove obsolete matrix/checkpoint scenario paths.
- Keep only current supported product paths.
- Make weather, battery, and health files structurally parallel.
- Remove unsupported smoke scenario behavior if no longer part of the product grammar.

### Guardrails

- No legacy support.
- No broad unrelated shell refactors.
- No argument trains.
- Preserve zsh idioms.

### Validation Output

- `zsh -n aag-build-qa.sh` passes.
- Supported scenario capabilities still dispatch.
- Unsupported capability paths fail clearly.

### Handoff

- Shell capability runners read as one family.
- Report closeout can be implemented against coherent execution/artifact state.

## Slice 9: Report Closeout

### Inputs

- `aag-build-qa.sh`
- `qa/lib/runtime.sh`
- `qa/python/runtime.py`
- coherent step-based artifacts

### Coding Outcome

- After a bootstrapped run finalizes, print:
  - run status
  - summary report path
  - `./aag-build-qa.sh --view <run-id>`
- Print closeout for both success and failure after bootstrap.
- Do not print run closeout for read-only commands.

### Guardrails

- Report closeout must not lie.
- Print report path only if report exists.
- If finalization fails, say so clearly.
- Do not create reports for read-only commands.

### Validation Output

- Success after bootstrap prints report path and view command.
- Failure after bootstrap prints report path and view command when report exists.
- Failure before bootstrap prints only the direct error.
- Read-only commands do not print run closeout.

### Handoff

- Operator has a clear next step after every real test-run.
- Summary report is discoverable without hunting in `qa/qa-runs`.

## Slice 10: Maintainer Documentation

### Inputs

- `qa/README.md` and `qa/QAScenarioGrammar.md`
- implementation state after slices 1-9

### Coding Outcome

- Add one compact maintainer-facing section for harness command classes and lifecycle boundaries.
- Document:
  - read-only commands
  - run-producing commands
  - bootstrap rule
  - report closeout rule
  - zsh/Python bridge rule

### Guardrails

- No broad docs rewrite.
- Do not duplicate code-level detail.
- Keep the section compact and source-of-truth oriented.

### Validation Output

- Documentation accurately matches implemented behavior.
- Section is short enough to be maintained.

### Handoff

- New maintainers can understand the harness shape without reverse-engineering shell control flow.
