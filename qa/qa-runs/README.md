# QA Run Artifacts

This directory is the durable home for validation-run artifacts.

Each harness run creates:

- `qa/qa-runs/<run-id>/run.json`
- `qa/qa-runs/<run-id>/commands.log`
- `qa/qa-runs/<run-id>/report.json`
- `qa/qa-runs/<run-id>/report.md`
- `qa/qa-runs/<run-id>/logs/`
- `qa/qa-runs/<run-id>/screenshots/`

Why this directory exists:

- run outputs must not depend on `build/`
- run outputs must survive `pebble clean`
- run outputs must be comparable to prior runs when reviewing drift
