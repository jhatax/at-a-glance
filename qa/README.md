# QA Harness

The QA harness validates *At A Glance* based on steps defined in named scenarios and suites. The public entrypoint is `../aag-build-qa.sh`.

## Command Flow

The shell entrypoint parses and validates the request, performs wrapper-owned reset and cleanup, and invokes Python. Python owns plan loading, step execution, screenshots, logs, JSON reports, Markdown reports, and exit status.

```text
aag-build-qa.sh
  -> parse and validate the request
  -> reset the selected emulator state
  -> runner.py scenario-exec ...
       -> load the scenario or suite
       -> expand ordered concrete steps
       -> execute each step
       -> finalize report.json and summary.md
  -> clean up wrapper-owned state
```

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

Use [WritingTestCasesAndPlans](docs/WritingTestCasesAndPlans.md) for scenario and suite authoring. Use [Validation](../docs/Validation.md) to select the right validation path for a product change.

## Ownership

`aag-build-qa.sh` owns the public shell boundary and wrapper lifecycle.

`runner.py` owns Python command dispatch.

`python/scenarios.py` owns grammar parsing, policy validation, include resolution, concrete steps, and artifact identity.

`python/execution.py` owns Pebble commands, step execution, screenshots, and command output.

`python/runtime.py` owns run context, final facts, canonical JSON, and run closeout.

`python/report.py` owns the shared Markdown renderer.

`python/comparison.py` owns run lookup, one-run summary lookup, and multi-run comparison orchestration.

## Run Artifacts

Each run is stored under `qa/qa-runs/<run-id>/`:

- `report.json`: canonical finalized payload
- `summary.md`: operator summary rendered from `report.json`
- `commands.log`: aggregate command output
- `logs/`: command logs retained by the execution flow
- `screenshots/`: captured screenshots when enabled

Comparison artifacts are stored under `qa/comparisons/`. Reports link to local artifacts and place screenshots beside their corresponding step rows.

## Automation Unit Tests

Run the harness correctness tests from the repository root:

```sh
python3 -m unittest discover -s qa/tests -p 'test_*.py'
```

The repo contains grammar fixtures used by unit tests under `qa/fixtures/`.

## Further Reading

- [WritingTestCasesAndPlans](docs/WritingTestCasesAndPlans.md) for authoring and fixtures.
- [QA_Harness_Implementation_Flow](docs/QA_Harness_Implementation_Flow.md) for
  function ownership and execution flow.
- [Validation](../docs/Validation.md) for contributor validation
  choices and evidence review.
- [Contributing](../docs/Contributing.md) for repository workflow.
