# QA Harness Implementation Flow

This document is the function-level implementation map for the QA harness.

## Architectural Goals

1. Every function has a documented place in the execution flow.
2. Every function has one responsibility and one clear handoff.s
3. Every function is in-use; no dead-code.

## Ownership Boundary

```text
aag-build-qa.sh
  -> parse and validate the public request
  -> route one command family

runner.py
  -> dispatch one Python command

scenarios.py
  -> parse grammar and resolve concrete steps

execution.py
  -> execute concrete steps and record execution facts

runtime.py
  -> create run context, finalize facts, build canonical payload

report.py
  -> render one or many canonical payloads as summary/comparison Markdown

comparison.py
  -> load canonical JSON, resolve runs, and orchestrate report output
```

The JSON report payload is the durable source of truth after finalization used by summary and comparison reports to surface information to the operator.

## Public Command Flow

### Scenario execution

```text
aag-build-qa.sh
  -> parse_args()
  -> validate_config()
  -> handle_qa_plan_execution()
       -> runner.py scenario-exec run-scenario|force-scenario
            -> load_plan()
            -> create_execution_context()
            -> run_plan_execution()
                 -> _run_step()
                      -> execute capability-specific command
                      -> gather command and screenshot evidence
                          -> finalize()
                              -> build QARunPayload
                              -> write report.json
                              -> write summary.md
                              -> _print_closeout()
```

### Inspections

```text
aag-build-qa.sh --view <run>
  -> parse_args()
  -> handle_qa_inspections()
  -> runner.py qa-inspection view-run <run>
      -> view_run()
        -> resolve_run_root()
        |-> return existing summary.md, or
        |-> render new summary.md
          -> load_qarun_payload()
          -> render_report()
```

### Run comparison

```text
aag-build-qa.sh --compare <run> ...
  -> parse_args()
  -> handle_qa_inspections()
  -> runner.py qa-inspection compare <runs>
    -> compare_runs()
      |-> return existing comparison.md, or
      |-> render new comparison.md
        -> load_qarun_payload() for each run
        -> write comparison.json
        -> render_report() -> comparison.md
```

## Function Responsibility Map

### Entrypoint and dispatch

| Function | Owns | Returns or hands off |
| --- | --- | --- |
| `main()` | Python command dispatch and argument parsing | One command handler result |
| `handle_qa_inspection()` | Inspection subcommand dispatch | One inspection handler result |
| `cmd_plan_exec()` | Load and execute one named plan | Process status |
| `cmd_view_run()` | Resolve one run and print its summary path | Process status |
| `cmd_compare_runs()` | Resolve selected runs and print comparison path | Process status |

### Scenario resolution

| Function | Owns | Must not own |
| --- | --- | --- |
| `load_plan()` | Resolve one named scenario or suite | Execution or artifact writes |
| `_parse_scenario_source()` | Parse one scenario file | Pebble commands |
| `_parse_suite_source()` | Parse one suite file | Step execution |
| `_expand_step_template()` | Expand one template for the selected emulators | Report rendering or filesystem writes |
| `_resolve_target_definition()` | Compose scenario or suite definitions into concrete steps | Pebble commands or report writes |
| `_artifact_identity()` | Derive one concrete step identity | Report rendering or filesystem writes |

### Run lifecycle

| Function | Owns | Output |
| --- | --- | --- |
| `create_execution_context()` | Create run paths and the screenshot directory | `ExecutionContext` |
| `run_plan_execution()` | Execute each resolved concrete step | Execution status |
| `_run_step()` | Select the executor for one concrete capability | Step result or failure |
| `_run_logged_command()` | Execute one external command and record its log | Command result and log |
| `_capture_screenshot()` | Capture one screenshot for one step | Screenshot path and step result |
| `finalize()` | Resolve final facts and emit run artifacts | `report.json`, `summary.md` |
| `_print_closeout()` | Print finalized run facts for the terminal | Plain-text closeout |

### Canonical report flow

| Function | Owns | Invariant |
| --- | --- | --- |
| `StepArtifact` | Name one concrete step’s execution facts | Emulator, display, inputs, status, and screenshot facts stay together |
| `ScreenshotsInfo` | Name one step’s screenshot facts | Expected, captured, and path stay together |
| `QARunPayload` | Name the canonical run payload | Payload has `runs` and `step_rows` |
| `QARunPayload.from_dict()` | Rebuild a typed payload when a caller needs one | JSON is read; Markdown is never parsed |
| `QARunPayload.as_dict()` | Convert the typed payload to JSON/renderer data | Only plain dictionaries and lists cross the output boundary |
| `finalize()` | Build and serialize one finalized run payload | Writes `report.json` and `summary.md` |
| `load_qarun_payload()` | Read and check one canonical `report.json` | Returns a canonical dictionary |
| `render_report()` | Render one or many canonical payloads | One table model serves summary and comparison |
| `view_run()` | Return an existing `summary.md` or render it from `report.json` | Never creates a comparison directory |
| `compare_runs()` | Return cached comparison or render selected payloads | Never reads Markdown as input |

## One Function, One Responsibility Review

For every function touched during implementation, answer these questions before keeping it:

- What single fact or transition does this function own?
- What is its input contract?
- What is its output contract?
- Which layer owns the next transition?
- Can its name be replaced by a direct call without losing clarity?
- Does it both interpret data and write artifacts? If so, split it.
- Does it both select a command and execute it? If so, split it.
- Does it accept arguments that do not vary its responsibility? Remove them.

The canonical QA Run report boundary is deliberately split into three operations:

```text
QARunPayload
  -> as_dict()
  -> write report.json
  -> render_report()
  -> write summary.md or comparison.md
```

Payload construction, Markdown rendering, and file writing have separate responsibilities. `report.json` is the read source for view and compare.

## Validation Gates

### Harness Correctness Tests

The executable harness-correctness tests live under `qa/harness-unit-tests/`. They exercise the public Python validation path against named and direct scenario/suite inputs, plus the rejection fixtures. They consume grammar fixtures to validate the harness's capabilities and grammar.

From the repository root, run the complete set in one command:

```sh
python3 -m unittest discover -s qa/harness-unit-tests -p 'test_*.py'
```

The calling convention is:

```sh
python3 -m unittest discover -s <test-directory> -p '<test-file-pattern>'
```

Run the focused module when iterating on this harness path:

```sh
python3 -m unittest -v qa/harness-unit-tests/test_harness_correctness.py
```

The test command must be run from the repository root so the test imports and fixture paths resolve consistently.

Each implementation slice is complete only when its ownership is documented and exercised:

1. Static: compile Python and run shell syntax checks.
2. Function: test the changed function with its narrow input contract.
3. Artifact: verify expected files and canonical JSON shape.
4. End to end: run a real screenshot-enabled scenario.
5. Composition: compare two or more real canonical run payloads.

The final evidence is the run directory and comparison directory.
