# QA Harness

The QA harness validates At A Glance with named scenarios and suites. The public entrypoint is `../aag-build-qa.sh`.

## Scenario execution flow

For scenario execution, the shell parses and validates the request, performs wrapper-owned emulator cleanup, and hands the plan name to Python. Python owns plan parsing, plan resolution, execution, screenshots, command logging, JSON reports, and Markdown reports. Build, install, validation, view, and compare commands use their own Python or shell handlers; they do not pass through every phase shown below.

```text
aag-build-qa.sh
  -> parse_args()
  -> validate_config()
  -> command handler
       -> ataglanceharness.py
            -> qaplanparser.py
            -> qaplanresolver.py
            -> qaplanexecutor.py
            -> qaharnessruntime.py
            -> qareportrenderer.py
```

The shell-to-Python handoff is one-way. Structured plan and run state stays in Python after the handoff; it is not serialized back into zsh.

## Commands

```sh
./aag-build-qa.sh --qaplan <name>
./aag-build-qa.sh --qaplan <name> --dry-run
./aag-build-qa.sh --qaplan <name> --force
./aag-build-qa.sh --validate <name-or-path>
./aag-build-qa.sh --runs
./aag-build-qa.sh --view <run-id-or-path>
./aag-build-qa.sh --compare <run-a> [run-b ...]
```

Use [WritingTestCasesAndPlans](docs/WritingTestCasesAndPlans.md) to author plans. Use [Validation](../docs/Validation.md) to choose contributor validation paths.

## Boundary

The shell prepares the environment and hands one validated command to Python. Python then owns the selected QA operation from plan loading through execution or inspection and report output. The implementation-level ownership map, typed handoffs, and failure behavior are maintained in [QAHarnessImplementationFlow](docs/QAHarnessImplementationFlow.md).

## Run artifacts

Each run is stored under `qa/qa-runs/<run-id>/`:

- `report.json` is the canonical report.
- `summary.md` is rendered from `report.json`.
- `commands.log` contains operator-visible command and execution records.
- `screenshots/` contains captured screenshots when the selected plan requests them.

Comparisons are stored under `qa/comparisons/`. A comparison writes `comparison.json` and `comparison.md`; both are derived from the selected runs’ canonical `report.json` files.

The run identity is derived from the run timestamp and process id. The output folder name is the run id, and deserialization checks that the stored timestamp, run id, and output folder still agree.

## Build and compile database

Use the build commands documented in [BuildandInstall](../docs/BuildandInstall.md). The harness routes build requests through the Pebble Tool environment; verbose builds may generate an optional `compile_commands.json`. Failure to produce compile-database entries does not change the build result.

## Tests

Run the harness tests from the repository root:

```sh
PYTHONPATH=tools/harness_py python3 -m unittest discover -s tools/harness_py -p 'test_*.py'
```

Plan fixtures and invalid report fixtures live under `qa/fixtures/`. A changed execution path also requires Python compilation, shell syntax checks, `git diff --check`, and a real scenario run that exercises the path.

## Adjacent

- [Validation](../docs/Validation.md) for contributor validation choices and evidence review.
- [Contributing](../docs/Contributing.md) for repository workflow and review discipline.

## Read Next

- [WritingTestCasesAndPlans](docs/WritingTestCasesAndPlans.md) for plan grammar and fixtures.
- [QAHarnessImplementationFlow](docs/QAHarnessImplementationFlow.md) for typed phase contracts and function ownership.
