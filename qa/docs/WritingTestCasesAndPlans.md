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
  SCREENSHOTS yes
  EMULATORS emery
  DISPLAYS white
END

EXECUTE
  STEP weather temp=265 code=0 is_day=1
END
```

A scenario is a filename-identified plan.

- Preamble: Declares one screenshot policy, one or more emulators, and one or more displays.
- Execute block: Contains ordered inline steps.
- Parser: Turns each `STEP` into a capability plus raw fields.
- Resolver: Creates supported `(emulator, display)` configurations, applies the screenshot policy, and expands each step across those configurations.

## Matrix

```text
PREAMBLE
  SCREENSHOTS yes
  EMULATORS emery chalk flint
  DISPLAYS white black
END

STEPS
  smoke.steps
END
```

A Matrix is a filename-identified plan that reuses one or more `.steps` files against its own PREAMBLE.

- Matrix PREAMBLE: The only source of emulator, display, and screenshot policy.
- Resolver: Reduces those values to supported execution tuples.
- Steps files: Are parsed in reference order.
- Expansion: Applies the screenshot policy and expands every parsed step across the supported tuples.
- `.tuples` files: Are not part of the grammar.

Steps files contain only an `EXECUTE` block; lines before `EXECUTE` and after its `END` are ignored:

```text
EXECUTE
  STEP weather temp=265 code=0 is_day=1
  STEP battery level=80 charging=1
END
```

## Suite

```text
MEMBERS
  INCLUDE SCENARIO canary
  INCLUDE MATRIX visual-refresh
  INCLUDE SUITE focused-capabilities
END
```

A suite is ordered aggregation.

- Suite contents: Members only. Suites have no screenshot policy, emulator list, display list, or step entries.
- Members: Scenarios, Matrices, or nested suites.
- Resolution: Each member is resolved independently, preserving its own tuples and screenshot policy.
- Result: Executable steps are merged into the suite's identity-keyed plan collection.

## Grammar rules

- Lines containing `#` are ignored entirely. A `#` is never an inline comment marker.
- The grammar vocabulary is explicit. Unsupported directives and non-grammar lines are ignored.
- `PREAMBLE`, `EXECUTE`, and `MEMBERS` are blocks terminated by `END`.
- Everything after a block's `END` is ignored.
- `SCREENSHOTS` accepts `yes` or `no`.
- `EMULATORS` accepts one or more supported emulator names: `aplite`, `basalt`, `chalk`, `diorite`, `emery`, `flint`, or `gabbro`.
- `DISPLAYS` accepts one or more known display names: `white`, `black`, `celeste`, or `oxford`. `celeste` and `oxford` are supported only on `chalk`, `emery`, and `gabbro`.
- `STEP` declares one capability and its fields.
- `INCLUDE SCENARIO`, `INCLUDE MATRIX`, and `INCLUDE SUITE` are legal only in `MEMBERS`.
- Suites contain only `MEMBERS`; preamble, execute, and other blocks in a suite are ignored.
- Include cycles are ignored; a previously discovered Suite, Scenario, or Matrix is not added again.
- A scenario or Matrix must have at least one executable step; a suite must have at least one member.

## Steps and arguments

Supported capabilities are `weather`, `battery`, `health`, `location`, `bluetooth`, and `all`.

| Capability | Arguments | Value constraints or notes |
| --- | --- | --- |
| `weather` | `temp`, `code`, `is_day` | `temp` is in celsius-tenths; `is_day` is `0` or `1`. |
| `battery` | `level`, `charging` | `level` is `0` through `100`; `charging` is `0` or `1`. |
| `health` | `bpm`, `steps` | Health metric values are supplied as integers. |
| `location` | `location` | `location` is text sent through the location AppMessage field. |
| `bluetooth` | `connected` | `connected` is `0` or `1`. |
| `all` | `temp`, `code`, `is_day`, `bpm`, `steps`, `level`, `charging`, `location`, `connected` | Supplies weather, health, battery, location, and Bluetooth values; supported on health-capable emulators. |

Field rules:

- Every supported field is required.
- Unknown fields are rejected.
- Duplicate fields are rejected.
- The parser keeps values as strings.
- The parser owns grammar checks.
- The resolver converts values to execution types and discards a step when typed construction rejects them.
- Specify strings containing spaces with quotes.

The order of `field=value` arguments is not strict. These two steps are equivalent:

```text
STEP battery level=19 charging=0
STEP battery charging=0 level=19
```

Argument names and required-key membership determine the parsed step. The parser and report validation do not require a particular argument order. The resolver reads named fields when it constructs the typed step and computes its identity; no input ordering contract exists.

`DISPLAYS` owns display selection for the entire scenario. The resolver uses the support matrix to discard displays unsupported by each requested emulator; display is never a STEP field.

### Weather

```text
STEP weather temp=<celsius-tenths> code=<weather-code> is_day=<0|1>
```

Fields are `temp`, `code`, and `is_day`.

### Battery

```text
STEP battery level=<percent> charging=<0|1>
```

Fields are `level` and `charging`.

`level` accepts `0` through `100`. `charging` accepts `0` or `1`.

### Health

```text
STEP health bpm=<value> steps=<value>
```

Fields are `bpm` and `steps`.

### Location

```text
STEP location location=<text>
```

The field is `location`. The location text is sent to the watch through the location AppMessage field.

### All

```text
STEP all bpm=101 steps=10500 level=90 charging=1 temp=300 code=1 is_day=0 location="San Francisco" connected=1
```

`all` step:

- Supplies weather, health, battery, location, and Bluetooth values in one step.
- Requires `temp`, `code`, `is_day`, `bpm`, `steps`, `level`, `charging`, `location`, and `connected`.
- Runs on health-capable emulators: `basalt`, `chalk`, `diorite`, `emery`, `flint`, and `gabbro`.

## Screenshot policy

Screenshot policy:

- `SCREENSHOTS yes`: Sets `capture_screenshots` on every expanded step.
- Expected counts: Ordinary screenshot-enabled capabilities expect one image. Bluetooth expects two because it captures before and after the connection-state change.
- `SCREENSHOTS no`: Gives each step an expected count of zero.
- Policy ownership: Screenshot policy is not a property of a `STEP` or `.steps` file. It is applied at Matrix or scenario expansion time.
- Captured counts: Start at zero and increment when execution captures a screenshot.
- Identity: Expected and captured counts are not identity inputs. The stable `shots` marker is included only when screenshots are required.

## Identity and de-duplication

Step expansion and identity:

```text
parsed step
  -> expand across each supported (emulator, display) tuple
  -> create an artifact identity
  -> insert into the insertion-ordered PlanDefinition.steps dictionary
  -> skip equal identities
```

Identity includes capability, concrete execution values, emulator, display, and the optional `shots` marker. Scenario names and screenshot counts are not identity inputs.

Discard and validation behavior:

- Unknown displays: Discarded during parsing.
- Known but unsupported displays: Discarded during resolution for the requested emulator.
- Unsupported capability/emulator combinations: Discarded during resolution.
- Invalid step expansions: Discarded during resolution.
- Operator output: Display discarded items after the executable steps.
- Empty configurations or steps: Fail plan validation.
- Any discard: Requires operator confirmation; `--force` does not bypass the prompt.

## Authoring workflow

1. Create `qa/plans/<name>.scenario`, `qa/plans/<name>.matrix`, or `qa/plans/<name>.suite`.
2. Put reusable `EXECUTE` blocks in `qa/plans/<name>.steps` files and reference them from Matrices.
3. Follow the block grammar and use supported names and fields.
4. Validate it with `./aag-build-qa.sh --validate <name>`.
5. Use `--dry-run` to inspect the resolved plan.
6. Run the plan with `--exec <name>` or `--exec <name> --force`. `--exec-plan` is an accepted alias for `--exec`.
7. Inspect the generated `report.json`, `summary.md`, `commands.log`, and screenshots.

## Fixtures and tests

Fixture locations:

- Valid grammar fixtures: `tools/harness_py/unittests/fixtures/scenarios/`
- Rejected plans: `tools/harness_py/unittests/fixtures/invalid-scenarios/`
- Invalid report fixtures: `tools/harness_py/unittests/fixtures/invalid-reports/`

Run the harness tests from the repository root:

```sh
PYTHONPATH=tools/harness_py uv run --python 3.13 python -m unittest discover -s tools/harness_py/unittests -p 'test_*.py'
```

Maintained canonical plans under `qa/plans/`:

- `canary`
- `qa-smoke`
- `visual-refresh`
- `pre-push-gate`
- `pre-release-gate`

Reusable steps live in `smoke.steps`, `visual-refresh.steps`, and `pre-release.steps`. Focused capability scenarios remain available where they provide targeted coverage.
