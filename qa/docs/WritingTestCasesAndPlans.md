# Writing Test Cases And Plans

This document describes the plan grammar used by the QA harness.

## Adjacent

- [QA README](../README.md) for commands and artifacts.
- [Validation](../../docs/Validation.md) for contributor validation paths.

## Read Next

- [QAHarnessImplementationFlow](QAHarnessImplementationFlow.md) for runtime ownership and handoffs.

## Scenario

```text
PREAMBLE
  SCENARIO weather-clear-day
  SCREENSHOTS yes
  EMULATORS emery
END

EXECUTE
  STEP weather display=white temp=539 code=0 is_day=1
END
```

A scenario declares one screenshot policy, one or more emulators, and ordered steps. The parser turns each `STEP` into a `ParsedStep`; the resolver expands it once per emulator into a `PlanStep`.

## Suite

```text
PREAMBLE
  SUITE run-subset
END

MEMBERS
  INCLUDE SCENARIO weather-clear-day
  INCLUDE SCENARIO battery-charging
END
```

A suite is ordered composition. It has no screenshot policy, emulator list, or step entries. It includes scenarios or nested suites. The parser recursively resolves nested suites into an ordered list of `ParsedScenario` objects, preserving member order. Each included scenario keeps its own emulator and screenshot policy. The resolver then combines their expanded steps into the identity-keyed plan collection.

## Grammar rules

- `#` starts a comment; the remainder of the line is ignored.
- `PREAMBLE`, `EXECUTE`, and `MEMBERS` are blocks terminated by `END`.
- `SCENARIO` and `SUITE` names use lowercase letters, digits, and hyphens.
- `SCREENSHOTS` accepts `yes` or `no`.
- `EMULATORS` accepts one or more supported emulator names.
- `STEP` declares one capability and its fields.
- `INCLUDE SCENARIO` and `INCLUDE SUITE` are legal only in `MEMBERS`.
- Include cycles are rejected.
- A scenario must have at least one step; a suite must have at least one member.

## Steps and arguments

Supported capabilities are `weather`, `battery`, and `health`.

Every supported field is required, unknown fields are rejected, duplicate fields are rejected, and values are kept as strings by the parser. Capability-specific construction converts the values to the types required by execution.

The order of `field=value` arguments is not strict. These two steps are equivalent:

```text
STEP battery display=white level=19 charging=0
STEP battery charging=0 level=19 display=white
```

Argument names, required-key membership, and values determine the step. The parser and report validation use the capability’s required field set; they do not require a particular argument order. The resolver reads the named fields when it constructs the typed step and computes its identity; no input ordering contract exists.

### Weather

```text
STEP weather display=<mode> temp=<celsius-tenths> code=<weather-code> is_day=<0|1>
```

Fields are `display`, `temp`, `code`, and `is_day`.

### Battery

```text
STEP battery display=<mode> level=<percent> charging=<0|1>
```

Fields are `display`, `level`, and `charging`.

### Health

```text
STEP health display=<mode> bpm=<value> steps=<value>
```

Fields are `display`, `bpm`, and `steps`.

## Screenshot policy

`SCREENSHOTS yes` sets `capture_screenshots` on every expanded step and gives each such step an expected count of one. `SCREENSHOTS no` gives each step an expected count of zero. Captured counts start at zero and are incremented by execution when a screenshot is captured. Expected and captured counts are not part of identity; the stable `shots` marker is included only when screenshots are required.

## Identity and de-duplication

The resolver expands each scenario step for each selected emulator. The resulting `PlanDefinition.steps` is an insertion-ordered dictionary keyed by artifact identity. Equal identities are de-duplicated. Identity includes capability, concrete execution values, emulator, and the optional `shots` marker. Scenario names and screenshot counts are not identity inputs.

## Authoring workflow

1. Create `qa/plans/<name>.scenario` or `qa/plans/<name>.suite`.
2. Follow the block grammar and use supported names and fields.
3. Validate it with `./aag-build-qa.sh --validate <name>`.
4. Use `--dry-run` to inspect the resolved plan.
5. Run the plan with `--exec-plan <name>` or `--exec-plan <name> --force`.
6. Inspect the generated `report.json`, `summary.md`, `commands.log`, and screenshots.

## Fixtures and tests

Valid grammar fixtures live under `qa/fixtures/scenarios/`; rejected plans live under `qa/fixtures/invalid-scenarios/`; invalid report fixtures exercise deserialization boundaries under `qa/fixtures/invalid-reports/`. Run the harness tests from the repository root:

```sh
PYTHONPATH=tools/harness_py python3 -m unittest discover -s tools/harness_py -p 'test_*.py'
```

The maintained plans under `qa/plans/` include `canary`, `dev-smoke`, emulator suites, and `pre-release-gate`.
