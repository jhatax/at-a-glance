# QA Harness

Use the QA harness to run named scenarios, Matrices, and suites against Pebble emulators and inspect their results. Run `./aag-build-qa.sh` from the repository root.

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
            -> qaplanexecutor_threaded.py
            -> qaharnessruntime.py
            -> qareportrenderer.py
```

After handoff, Python owns plan state, execution, and reports. Shell receives the process status. [QAHarnessImplementationFlow](docs/QAHarnessImplementationFlow.md) maps the functions and failure paths.

## Commands

```sh
./aag-build-qa.sh --list-steps <name>
./aag-build-qa.sh --exec <name>
./aag-build-qa.sh --exec <name> --dry-run
./aag-build-qa.sh --exec <name> --force
./aag-build-qa.sh --validate <name-or-path>
./aag-build-qa.sh --runs
./aag-build-qa.sh --view <run-id-or-path>
./aag-build-qa.sh --compare <run-a> [run-b ...]
```

`--exec-plan` is an accepted alias for `--exec`. `-ls` is an accepted alias for `--list-steps`, which is an alias for `--validate`.

Use [WritingTestCasesAndPlans](docs/WritingTestCasesAndPlans.md) to author plans. Use [Validation](../docs/Validation.md) to choose contributor validation paths.

Plan validation prints executable steps, then discarded items. Discards trigger an execution prompt even with `--force`.

## Run artifacts

Each run is stored under `qa/qa-runs/<run-id>/`:

- `report.json` is the canonical report.
- `summary.md` is rendered from `report.json` and shows each step's pass/fail result.
- `commands.log` contains operator-visible command and execution records.
- `screenshots/` contains captured screenshots when the selected plan requests them.

Screenshot-enabled weather, battery, health, and location steps expect one image. Bluetooth and `all` steps expect two images, before and after changing connection state. A passed step confirms that its operations completed and its screenshot count matched; the operator reviews visual correctness.

Comparisons are stored under `qa/comparisons/`. When generated, `comparison.json` and `comparison.md` are derived from the selected runs’ canonical `report.json` files, with each step's pass/fail result shown per run.

## Build and compile database

The harness builds through Pebble Tool. Verbose builds may also generate `compile_commands.json`; missing compiler entries do not change the build result. [BuildandInstall](../docs/BuildandInstall.md) owns build commands, logs, diagnostics, and editor setup.

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
