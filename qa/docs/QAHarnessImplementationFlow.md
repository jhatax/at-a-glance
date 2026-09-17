# QA Harness Implementation Flow

This document records the live Python runtime flow.

- Operator entry point: [QA README](../README.md).
- This document: Owns the implementation map.
- Handoffs: Each phase has one owner, one input contract, and one output contract.
- Phase table: Shows the conceptual flow.
- Command-handoff table: Names the live functions.
- Report contract: Records the canonical output boundary.

## Adjacent

- [QA README](../README.md) for public commands and run artifacts.
- [Settled decisions](../settled-decisions.md) for decisions that must not be relitigated without new contrary evidence.
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
| Parse | `ataglanceharness.py` | `qaplanparser.py` | Plan path | Grammar values, `ParsedStep` list, and parser discards | Parser grammar, block boundaries, source order, and include references |
| Resolve | `ataglanceharness.py` | `qaplanresolver.py` | Parsed document | `PlanDefinition` with identity-keyed `PlanStep` objects | Resolver expansion, concrete fields, screenshot directives, expected counts, de-duplication |
| Execute | `ataglanceharness.py` | `qaplanexecutor_threaded.py` and `pebbleadapter.py` | `PlanDefinition` | Per-step results, buffered logs, and captured screenshot paths | Concurrent executor scheduling and result collection; adapter transport and evidence |
| Finalize | `qaplanexecutor_threaded.py` | `qaharnessruntime.py` and `qareportrenderer.py` | Plan facts, accumulated results, and runtime paths | `report.json`, `summary.md`, terminal closeout | Runtime aggregation/serialization; renderer Markdown |

### Parse

```text
qaplanparser.py
  scenario / Matrix / steps -> grammar values, ParsedStep records, discards
  Suite -> direct ordered member references
  Suite + discovery dictionary -> newly discovered members only

qaplanresolver.py
  Suite members -> ordered, unique leaf Scenarios and Matrices
  parsed plan values -> supported (emulator, display) tuples
  tuples + parsed steps + screenshot policy -> executable PlanStep objects

Parser does not create PlanStep objects, access Pebble, execute commands, or
write reports.
```

### Resolve

`qaplanresolver.py` owns the following resolution flow:

```text
_expand_suite_members()
  seed root Suite
  walk the growing ordered member list
  dictionary de-duplicates (kind, name) identities
  repeated and cyclic references add no new traversal entry

SUPPORT_MATRIX
  is the single support oracle for emulators, displays, and capabilities

PlanDefinition.execution_configs
  ordered supported (emulator, display) tuples

PlanDefinition.steps
  insertion-ordered step-identity -> PlanStep mapping
  duplicate step identities are de-duplicated

Resolver
  converts parsed strings into typed steps
  records unsupported configurations and construction failures as discards
  applies screenshot policy and calculates expected counts
  hands PlanDefinition to the Plan Execution Runtime
```

### Execute

```text
ataglanceharness.py
  -> resolve_and_execute_plan_concurrently()
  -> execute_plan_concurrently()

qaplanexecutor_threaded.py
  -> enumerate the canonical PlanDefinition.steps sequence and assign step_number
  -> group steps by emulator while preserving each lane's source order
  -> create one worker thread for each emulator
  -> install each emulator before starting its worker
  -> each worker executes its assigned steps serially
       -> create one connection for the step
       -> PlanStep.run()
       -> call the capability-specific PebbleEmulatorConnection operation
       -> capture requested screenshots through the supplied callback
       -> return one ConcurrentStepResult containing the result and that step's logs
  -> process completed results on one result-processor thread
       -> write each completed step's logs as one block
       -> update captured screenshot totals
       -> append the raw result for report finalization
  -> join workers, drain the results queue, stop the result processor, and kill emulators
  -> finalize the complete plan once
```

**Screenshot behavior**

- Typed `PlanStep` objects own capability dispatch through `run()`.
- Weather, battery, health, location, Bluetooth, and `all` steps call their corresponding `PebbleAdapter` operation.
- Bluetooth captures once before changing connection state and once after.

**Adapter and executor boundaries**

- `PebbleAdapter` owns libpebble2 transport, emulator installation and restart, emulator cleanup, AppMessage operations, battery state, screenshots, and builds.
- `PebbleAdapter.create_connection()` owns connection startup and WebSocket close. It serializes Pebble Tool emulator-registry operations with its registry lock.
- `PebbleEmulatorConnection` appends operation evidence to the step-local log list.
- The concurrent executor owns emulator-lane order, worker lifecycle, result collection, step status, and durable command-log writes.
- The executor does not construct shell command lines or launch a second Pebble process.

### Finalize

```text
qaharnessruntime.py
  -> create HarnessRuntime output paths before execution
  -> build QAStepOutput rows from the original PlanDefinition and raw step results
  -> build one ConsolidatedQARunOutputs value
  -> write report.json
  -> render summary.md
  -> print closeout information

qareportrenderer.py
  -> render Markdown from typed report objects
  -> include each step's pass/fail result
  -> supply step results for summary.md and comparison.md

qaresultinspector.py
  -> load report.json for view and compare
  -> create derived Markdown only when it is absent
```

**Report context contents**

- `HarnessRuntime`: Run paths and start time.
- `ConsolidatedQARunOutputs`: Plan identity, run identity, output paths, resolved totals, status, and step outputs.
- `QAStepOutput`: One step identity, capability, emulator, step arguments, status, and screenshot context.

## Command handoffs

| Function | Owns | Accepts | Returns or hands off |
| --- | --- | --- | --- |
| `parse_args()` | Shell syntax and command selection | User arguments | Validated shell command state or failure |
| `validate_config()` | Shell cross-field and environment checks | Parsed shell state | Shell success or failure |
| `handle_qa_plan_execution()` | One-way shell-to-Python execution handoff | Validated scenario-exec state | Python process status |
| `main()` | Python CLI parsing and dispatch | Python `argv` | Selected handler’s integer status |
| `handle_plan_exec()` | Scenario-exec action validation and concurrent-executor dispatch | Action and plan name | `resolve_and_execute_plan_concurrently()` status |
| `resolve_and_execute_plan_concurrently()` | Confirmation policy and concurrent lifecycle call | Action and plan name | Execution status |
| `load_and_validate_plan()` | Target resolution and plan construction | Name/path | `PlanDefinition` or `ValueError` |
| `create_harness_runtime()` | Run folders and output paths | Capture-screenshots boolean | `HarnessRuntime` |
| `stratify_steps_by_emulator()` | Canonical step numbering and emulator lanes | Identity-keyed steps | Emulator-keyed ordered step lists |
| `execute_plan_concurrently()` | Installation, worker startup, queue lifecycle, cleanup, and finalization call | `PlanDefinition` | Final process status |
| `execute_emulator_steps()` | One emulator lane | `EmulatorExecutionState` | One queued result per step |
| `_execute_emulator_step()` | One step’s dispatch, capture, local logs, and result record | Emulator state and `PlanStep` | `ConcurrentStepResult` |
| `process_step_results()` | Parent-owned result handling and command-log writes | Results queue | Accumulated results and screenshot totals |
| `PlanExecutionState.finalize()` | Aggregate status and report emission | Runtime, plan, and raw results | `report.json`, `summary.md`, integer status |
| `handle_view_run()` | One-run inspection | Run selector | Existing or regenerated `summary.md` |
| `handle_compare_runs()` | Multi-run inspection | One to five selectors | Existing or regenerated `comparison.md` |

## Report contract

```text
PlanDefinition
  -> qaplanexecutor_threaded.py records RawStepResult values
  -> qaharnessruntime.py builds ConsolidatedQARunOutputs and QAStepOutput values
  -> report.json
  -> qareportrenderer.py
  -> summary.md or comparison.md
```

`report.json` is the durable source of truth. Markdown is output-only and is never read to recover execution facts.

## Validation gates

These are the evidence gates for a QA-harness change. Use [Validation](../../docs/Validation.md) for the repository-wide choice of validation path.

- Compile the Python harness and check shell syntax.
- Run the harness unit tests and `git diff --check`.
- For a concurrent-execution change, run a real screenshot-enabled Matrix that exercises more than one emulator. Confirm each emulator lane remains serial, each step has one result, and inspect its `report.json`, `summary.md`, and `commands.log`.
- For a screenshot-policy change, verify the expected count for each capability, including two screenshots for Bluetooth.
- For a reporting change, remove or rename a derived Markdown file, run view or compare, and confirm the regenerated Markdown matches the canonical JSON facts.
- For a parser or resolver change, validate accepted plans, rejected fixtures, ignored cyclic includes, suite order, and identity de-duplication.
