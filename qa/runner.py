#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

from python.comparison import compare_runs, resolve_report_path
from python.scenarios import load_scenario, load_scenario_file


def cmd_bootstrap() -> int:
    from python.runtime import bootstrap, bootstrap_records
    from python.state import read_cli_state_from_env

    cli_state = read_cli_state_from_env()
    scenario = None
    if cli_state.scenario_name:
        scenario = load_scenario(cli_state.scenario_name, cli_state.qa_dir / "scenarios")

    result = bootstrap(cli_state, scenario)
    for key, value in bootstrap_records(result):
        print(f"{key}\t{value}")
    return 0


def cmd_emit_steps(path: str) -> int:
    payload = json.loads(Path(path).read_text(encoding="utf-8"))
    for step in payload.get("steps", []):
        fields = step.get("fields", {})
        if not isinstance(fields, dict):
            fields = {}

        record = [
            f"capability={step.get('capability', '')}",
            f"artifact_identity={step.get('artifact_identity', '')}",
        ]
        for key, value in fields.items():
            record.append(f"{key}={value}")
        print("\t".join(record))
    return 0


def cmd_finalize() -> int:
    from python.runtime import finalize_from_env

    return finalize_from_env()


def cmd_validate_file(path: str) -> int:
    load_scenario_file(Path(path))
    return 0


def _resolve_scenario_validation_target(target: str, scenarios_dir: Path) -> tuple[Path, str | None]:
    path = Path(target)
    if path.is_file():
        return path, None

    scenario_path = scenarios_dir / f"{target}.scenario"
    if scenario_path.is_file():
        return scenario_path, target

    raise ValueError(f"Unknown scenario validation target '{target}'")


def cmd_validate_scenario(target: str, scenarios_dir: str) -> int:
    path, expected_name = _resolve_scenario_validation_target(target, Path(scenarios_dir))
    scenario = load_scenario_file(path, expected_scenario_name=expected_name)

    print(f"Scenario validation: passed")
    print(f"Scenario: {scenario.name}")
    print(f"File: {path}")
    print("Parse: passed")
    print("Includes: resolved")
    print(f"Concrete steps: valid ({len(scenario.steps)})")
    return 0


def cmd_print_plan(scenario_name: str, scenarios_dir: str) -> int:
    scenario = load_scenario(scenario_name, Path(scenarios_dir))
    print(f"Scenario: {scenario.name}")
    print(f"Screenshots: {scenario.screenshots_policy}")
    print("Resolved execution plan:")
    for index, step in enumerate(scenario.steps, start=1):
        fields = " ".join(f"{key}={value}" for key, value in step.fields.items())
        print(f"{index}. STEP {step.capability} {fields} -> {step.artifact_identity}")
    return 0


def _open_hint(path: Path) -> str:
    return f"open {path}"


def cmd_compare_runs(run_selectors: list[str]) -> int:
    qa_root = Path(__file__).resolve().parent
    if len(run_selectors) == 1:
        report_path = resolve_report_path(run_selectors[0], qa_root)
        print(f"One run selector provided. Using the existing summary report.")
        print(report_path)
        print(_open_hint(report_path))
        return 0

    output_dir = compare_runs(run_selectors, qa_root)
    report_path = output_dir / "comparison.md"
    print(report_path)
    print(_open_hint(report_path))
    return 0


def cmd_report(run_selector: str) -> int:
    return cmd_compare_runs([run_selector])


def cmd_list_runs() -> int:
    qa_root = Path(__file__).resolve().parent
    runs_root = qa_root / "qa-runs"
    run_payloads: list[dict[str, str]] = []

    print("Run ID\tStarted\tStatus\tScenario")
    for run_dir in runs_root.iterdir():
        if not run_dir.is_dir():
            continue
        report_path = run_dir / "report.json"
        if not report_path.is_file():
            continue
        payload = json.loads(report_path.read_text(encoding="utf-8"))
        run_payloads.append(
            {
                "run_id": payload.get("run_id", run_dir.name),
                "started_at": payload.get("started_at", ""),
                "status": payload.get("status", "unknown"),
                "scenario": payload.get("scenario", "unknown"),
            }
        )

    run_payloads.sort(key=lambda payload: payload["started_at"], reverse=True)
    for payload in run_payloads[:10]:
        print("\t".join([payload["run_id"], payload["started_at"], payload["status"], payload["scenario"]]))
    return 0


def main(argv: list[str]) -> int:
    try:
        parser = argparse.ArgumentParser()
        subparsers = parser.add_subparsers(dest="command", required=True)

        subparsers.add_parser("bootstrap")

        emit_steps = subparsers.add_parser("emit-steps")
        emit_steps.add_argument("path")

        subparsers.add_parser("finalize")

        validate_file = subparsers.add_parser("validate-file")
        validate_file.add_argument("path")

        validate_scenario = subparsers.add_parser("validate-scenario")
        validate_scenario.add_argument("target")
        validate_scenario.add_argument("scenarios_dir")

        print_plan = subparsers.add_parser("print-plan")
        print_plan.add_argument("scenario_name")
        print_plan.add_argument("scenarios_dir")

        subparsers.add_parser("list-runs")

        report = subparsers.add_parser("report")
        report.add_argument("run_selector")

        compare = subparsers.add_parser("compare-runs")
        compare.add_argument("run_selectors", nargs="+")

        args = parser.parse_args(argv)

        if args.command == "bootstrap":
            return cmd_bootstrap()
        if args.command == "emit-steps":
            return cmd_emit_steps(args.path)
        if args.command == "finalize":
            return cmd_finalize()
        if args.command == "validate-file":
            return cmd_validate_file(args.path)
        if args.command == "validate-scenario":
            return cmd_validate_scenario(args.target, args.scenarios_dir)
        if args.command == "print-plan":
            return cmd_print_plan(args.scenario_name, args.scenarios_dir)
        if args.command == "list-runs":
            return cmd_list_runs()
        if args.command == "report":
            return cmd_report(args.run_selector)
        if args.command == "compare-runs":
            return cmd_compare_runs(args.run_selectors)
        return 1
    except ValueError as exc:
        print(f"Error: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
