# QA Harness

The QA harness validates At A Glance with named scenarios, Matrices, and suites. The public entrypoint is `../aag-build-qa.sh`.

## Plan execution flow

Plan execution crosses the shell/Python boundary once:

```text
shell
  -> parse and validate the request
  -> perform wrapper-owned emulator cleanup
  -> hand the plan name to Python

Python
  -> parse the plan
  -> resolve the support matrix
  -> execute steps and capture screenshots
  -> write command logs, JSON reports, and Markdown reports
```

Build, install, validation, view, and compare commands use their own Python or shell handlers. They do not pass through every phase shown above.

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
./aag-build-qa.sh --exec <name>
./aag-build-qa.sh --exec <name> --dry-run
./aag-build-qa.sh --exec <name> --force
./aag-build-qa.sh --validate <name-or-path>
./aag-build-qa.sh --runs
./aag-build-qa.sh --view <run-id-or-path>
./aag-build-qa.sh --compare <run-a> [run-b ...]
```

`--exec-plan` remains an accepted alias for `--exec`.

Use [WritingTestCasesAndPlans](docs/WritingTestCasesAndPlans.md) to author plans. Use [Validation](../docs/Validation.md) to choose contributor validation paths.

## Boundary

Boundary contract:

- Shell: Prepare the environment and hand one validated command to Python.
- Python: Own the selected QA operation from plan loading through execution or inspection and report output.
- Implementation details: See [QAHarnessImplementationFlow](docs/QAHarnessImplementationFlow.md) for ownership, typed handoffs, and failure behavior.

## Run artifacts

Each run is stored under `qa/qa-runs/<run-id>/`:

- `report.json` is the canonical report.
- `summary.md` is rendered from `report.json` and shows each step's pass/fail result.
- Plan validation prints executable steps first, followed by discarded items. Discards require operator confirmation before execution.
- `commands.log` contains operator-visible command and execution records.
- `screenshots/` contains captured screenshots when the selected plan requests them.

Comparisons are stored under `qa/comparisons/`. When generated, `comparison.json` and `comparison.md` are derived from the selected runs’ canonical `report.json` files, with each step's pass/fail result shown per run.

The only run identity invariant is that `run_id` equals the output folder name.

## Build and compile database

Build behavior:

- Commands: Use [BuildandInstall](../docs/BuildandInstall.md).
- Environment: The harness routes build requests through the Pebble Tool environment.
- Compile database: Verbose builds may generate an optional `compile_commands.json`.
- Build result: Failure to produce compile-database entries does not change the build result.

Verbose builds write complete output and any Python traceback to `build.log`. Console and QA-log failures show only extracted compiler diagnostics. Non-verbose builds do not create `build.log` and use the normal harness failure path.

## Tests

Run the harness tests from the repository root:

```sh
PYTHONPATH=tools/harness_py uv run --python 3.13 python -m unittest discover -s tools/harness_py/unittests -p 'test_*.py'
```

Test and fixture locations:

- Plan fixtures and invalid report fixtures: `tools/harness_py/unittests/fixtures/`
- Canonical reusable step files and executable Matrix plans: `qa/plans/`
- Changed execution path: Also requires Python compilation, shell syntax checks, `git diff --check`, and a real scenario or Matrix run that exercises the path.

## Adjacent

- [Validation](../docs/Validation.md) for contributor validation choices and evidence review.
- [Contributing](../docs/Contributing.md) for repository workflow and review discipline.

## Read Next

- [WritingTestCasesAndPlans](docs/WritingTestCasesAndPlans.md) for plan grammar and fixtures.
- [QAHarnessImplementationFlow](docs/QAHarnessImplementationFlow.md) for typed phase contracts and function ownership.
