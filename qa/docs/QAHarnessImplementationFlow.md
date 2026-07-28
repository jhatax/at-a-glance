# QA Harness Implementation Flow

This document records the live Python runtime flow. [QA README](../README.md) is the operator entry point; this document owns the implementation map. The phases are ordered and each handoff has one owner, one input contract, and one output contract. The phase table gives the conceptual flow, the command-handoff table names the live functions, and the report contract records the canonical output boundary.

## Adjacent

- [QA README](../README.md) for public commands and run artifacts.
- [WritingTestCasesAndPlans](WritingTestCasesAndPlans.md) for grammar and fixtures.
- [Validation](../../docs/Validation.md) for contributor validation paths.

## Read Next

- [WatchfaceImplementationFlow](../../docs/WatchfaceImplementationFlow.md) for the watch face implementation map.

## Implementation and flow guardrails

1. Every function has one responsibility and one clear handoff.
2. Every handoff names its input, output, owner, and failure behavior.
3. Every function in the execution path is live code; dead wrappers are removed.
4. A layer does not reconstruct or own another layer’s state.
5. A changed execution or artifact handoff has focused tests and a real lifecycle validation.
6. New abstractions require a demonstrated ownership conflict or repeated duplication.

## Four runtime phases

| Phase | Caller | Callee | Input | Output | Owner |
| --- | --- | --- | --- | --- | --- |
| Parse | `ataglanceharness.py` | `qaplanparser.py` | Resolved scenario or suite path | `ParsedScenario` or `ParsedSuite` | Parser grammar, document validation, recursive suite-member resolution, and source order |
| Resolve | `ataglanceharness.py` | `qaplanresolver.py` | Parsed document | `PlanDefinition` with identity-keyed `PlanStep` objects | Resolver expansion, concrete fields, screenshot directives, expected counts, de-duplication |
| Execute | `ataglanceharness.py` | `qaplanexecutor.py` and `pebbleadapter.py` | `PlanDefinition` and run context | Per-step results and captured screenshot paths | Executor iteration/dispatch; adapter transport and evidence |
| Finalize | `qaplanexecutor.py` | `qaharnessruntime.py` and `qareportrenderer.py` | Plan facts and step results | `report.json`, `summary.md`, terminal closeout | Runtime aggregation/serialization; renderer Markdown |

### Parse

`qaplanparser.py` parses scenario and suite files. `ParsedStep` contains a capability, a boolean screenshot directive, and string fields. `ParsedScenario` contains its name, screenshot policy, emulators, and ordered steps. `ParsedSuite` contains its name and an ordered list of parsed scenarios. The parser recursively resolves nested suite members into `ParsedScenario` objects and preserves their source order. `qaplanresolver.py` owns top-level name/path resolution, then receives those parsed scenarios and expands them into executable steps. The parser does not create `PlanStep` objects, access Pebble, execute commands, or write reports.

### Resolve

`qaplanresolver.py` converts parsed documents into executable objects. `PlanDefinition.steps` is an insertion-ordered `dict[str, PlanStep]`; the key is the stable step identity and duplicate identities are de-duplicated. `PlanStep` carries capability inputs, emulator, display, `capture_screenshots`, `expected_screenshots`, and `captured_screenshots`. Expected counts are calculated when steps are created. Identity includes the stable step values and the optional `shots` marker, but never expected or captured counts.

### Execute

`qaplanexecutor.py` creates `ExecutionState`, installs the plan’s emulators, and iterates over `plan.steps.values()`. Each step is dispatched by concrete type to weather, battery, or health operations. A requested screenshot is captured through `PebbleAdapter`; successful capture appends its path to the step result and updates the step’s captured count. A failed step is recorded as failed and execution continues with the remaining steps. The executor closes the adapter before finalization.

`PebbleAdapter` owns libpebble2 transport, emulator installation, AppMessage operations, battery state, screenshots, build operations, and command logging. The executor owns operation order and step status; it does not construct shell command lines or launch a second Pebble process.

### Finalize

`qaharnessruntime.py` sums step facts, builds `QAStepContext` rows and one `QARunContext`, writes canonical `report.json`, renders `summary.md`, and prints closeout information. `QARunContext` contains plan identity, run identity, output paths, resolved totals, and step outputs. `QAStepContext` contains one step identity, capability, emulator, step arguments, status, and screenshot context. `qareportrenderer.py` renders Markdown from those typed report objects. `qaresultinspector.py` loads `report.json` for view and compare operations and regenerates Markdown when the derived file is absent.

## Command handoffs

| Function | Owns | Accepts | Returns or hands off |
| --- | --- | --- | --- |
| `parse_args()` | Shell syntax and command selection | User arguments | Validated shell command state or failure |
| `validate_config()` | Shell cross-field and environment checks | Parsed shell state | Shell success or failure |
| `handle_qa_plan_execution()` | One-way shell-to-Python execution handoff | Validated scenario-exec state | Python process status |
| `main()` | Python CLI parsing and dispatch | Python `argv` | Selected handler’s integer status |
| `handle_plan_exec()` | Scenario-exec action validation | Action and plan name | `resolve_and_execute_plan()` status |
| `resolve_and_execute_plan()` | Confirmation policy and lifecycle call | Action and plan name | Execution status |
| `load_and_validate_plan()` | Target resolution and plan construction | Name/path | `PlanDefinition` or `ValueError` |
| `create_harness_context()` | Run folders and output paths | Capture-screenshots boolean | `HarnessRuntimeContext` |
| `run_plan_execution()` | Installation, iteration, adapter close, finalization call | `PlanDefinition` | Final process status |
| `_execute_step()` | One step’s dispatch, capture, and result record | `ExecutionState`, `PlanStep` | Appended `StepResult`; step capture facts |
| `finalize()` | Aggregate status and report emission | Context, plan, exit status, step results | `report.json`, `summary.md`, integer status |
| `handle_view_run()` | One-run inspection | Run selector | Existing or regenerated `summary.md` |
| `handle_compare_runs()` | Multi-run inspection | One to five selectors | Existing or regenerated `comparison.md` |

## Report contract

```text
PlanDefinition
  -> qaplanexecutor.py records StepResult values
  -> qaharnessruntime.py builds QARunContext and QAStepContext
  -> report.json
  -> qareportrenderer.py
  -> summary.md or comparison.md
```

`report.json` is the durable source of truth. Markdown is output-only and is never read to recover execution facts.

## Validation gates

These are the evidence gates for a QA-harness change. Use [Validation](../../docs/Validation.md) for the repository-wide choice of validation path.

- Compile the Python harness and check shell syntax.
- Run the harness unit tests and `git diff --check`.
- For an execution change, run a real screenshot-enabled scenario and inspect its `report.json`, `summary.md`, and `commands.log`.
- For a reporting change, remove or rename a derived Markdown file, run view or compare, and confirm the regenerated Markdown matches the canonical JSON facts.
- For a parser or resolver change, validate accepted plans, rejected fixtures, include cycles, suite order, and identity de-duplication.
