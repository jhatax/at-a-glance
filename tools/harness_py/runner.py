#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

from config import PLANS_ROOT, QA_ROOT, REPO_ROOT
from comparison import compare_runs, view_run
from execution import run_plan_execution
from runtime import create_execution_context
from plans import PlanDefinition, load_plan


def _print_plan_maybe_confirm(plan: PlanDefinition, confirm: bool = False) -> bool:
  print(f"Plan: {plan.name}")
  print(f"Screenshots: {plan.screenshots_policy}")
  print("Resolved execution plan:")
  for index, step in enumerate(plan.steps.values(), start=1):
    fields = " ".join(f"{key}={value}" for key, value in step.fields.items())
    print(f"{index}. STEP {step.capability} {fields} -> {step.artifact_identity}")

  if confirm:
    print("Execute this plan? [y/N] ", end="", flush=True)
    reply = input().strip().lower()
    return reply in {"y", "yes"}

  # user-confirmation was not needed
  return True


def handle_plan_dryrun(plan_name: str) -> int:
  _print_plan_maybe_confirm(load_plan(plan_name, PLANS_ROOT))
  return 0


def handle_build(verbose: bool, log_path: str | None) -> int:
  from pebble import PebbleAdapter

  adapter = PebbleAdapter(print)
  try:
    adapter.build(verbose, Path(log_path) if log_path else None)
  finally:
    adapter.close()
  return 0


def handle_install(emulators: list[str]) -> int:
  from pebble import PebbleAdapter

  adapter = PebbleAdapter(print)
  try:
    pbw_path = REPO_ROOT / "build" / "at-a-glance.pbw"
    adapter.install_emulators(emulators, pbw_path)
  finally:
    adapter.close()
  return 0


def handle_qa_inspection(args: argparse.Namespace) -> int:
  if args.inspection_command == "runs":
    return cmd_list_runs()
  if args.inspection_command == "view-run":
    return cmd_view_run(args.run_selector)
  if args.inspection_command == "compare":
    return cmd_compare_runs(args.run_selectors)
  if args.inspection_command == "validate":
    return cmd_validate_plan(args.plan_name)
  if args.inspection_command == "dryrun":
    return handle_plan_dryrun(args.plan_name)
  return 1


def validate_plan(target: str) -> PlanDefinition:

  path = Path(target)
  target_parent = PLANS_ROOT
  # handle file-path
  if path.is_file():
    target_parent = path.parent

  return load_plan(target, target_parent)


def cmd_plan_exec(action: str, plan_name: str) -> int:
  if action not in {"run-scenario", "force-scenario"}:
    raise ValueError(f"Unsupported scenario-exec action '{action}'")

  plan = validate_plan(plan_name)
  if action == "run-scenario" and not _print_plan_maybe_confirm(plan, True):
    print("Plan execution cancelled.", file=sys.stderr)
    return 1

  context = create_execution_context(QA_ROOT, plan)
  return run_plan_execution(context, plan)


def cmd_validate_plan(target: str) -> int:
  plan = validate_plan(target)

  print("QA plan validation: passed")
  print(f"Plan: {plan.name}")
  print(f"File: {plan.path}")
  print("Parse: passed")
  print("Includes: resolved")
  print(f"Concrete steps: valid ({len(plan.steps)})")
  return 0


def _open_hint(path: Path) -> str:
  return f"open {path}"


def cmd_compare_runs(run_selectors: list[str]) -> int:
  report_path = compare_runs(run_selectors)
  print(report_path)
  print(_open_hint(report_path))
  return 0


def cmd_view_run(run_selector: str) -> int:
  report_path = view_run(run_selector)
  print(report_path)
  print(_open_hint(report_path))
  return 0


def cmd_list_runs() -> int:
  runs_root = QA_ROOT / "qa-runs"
  run_payloads: list[dict[str, str]] = []

  print("Run ID\tStarted\tStatus\tPlan")
  for run_dir in runs_root.iterdir():
    if not run_dir.is_dir():
      continue
    report_path = run_dir / "report.json"
    if not report_path.is_file():
      continue
    payload = json.loads(report_path.read_text(encoding="utf-8"))
    run = payload.get("runs", [{}])[0]
    run_payloads.append({
        "run_id": run.get("run_id", run_dir.name),
        "started_at": run.get("started_at", ""),
        "status": run.get("status", "unknown"),
        "plan": run.get("plan", "unknown"),
    })

  run_payloads.sort(key=lambda payload: payload["started_at"], reverse=True)
  for payload in run_payloads[:10]:
    print("\t".join([
        payload["run_id"],
        payload["started_at"],
        payload["status"],
        payload["plan"],
    ]))
  return 0


def main(argv: list[str]) -> int:
  try:
    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers(dest="command", required=True)

    qa_inspection = subparsers.add_parser("qa-inspection")
    qa_inspection_subparsers = qa_inspection.add_subparsers(dest="inspection_command",
                                                            required=True)

    qa_inspection_subparsers.add_parser("runs")

    view_run = qa_inspection_subparsers.add_parser("view-run")
    view_run.add_argument("run_selector")

    compare_inspection = qa_inspection_subparsers.add_parser("compare")
    compare_inspection.add_argument("run_selectors", nargs="+")

    validate_inspection = qa_inspection_subparsers.add_parser("validate")
    validate_inspection.add_argument("plan_name")

    dryrun_inspection = qa_inspection_subparsers.add_parser("dryrun")
    dryrun_inspection.add_argument("plan_name")

    scenario_exec = subparsers.add_parser("scenario-exec")
    scenario_exec.add_argument("action", choices=("run-scenario", "force-scenario"))
    scenario_exec.add_argument("--qaplan-name", dest="plan_name", required=True)

    build = subparsers.add_parser("build")
    build.add_argument("--verbose", action="store_true")
    build.add_argument("--log-path")

    install = subparsers.add_parser("install")
    install.add_argument("--emulators", nargs="+", required=True)

    args = parser.parse_args(argv)

    if args.command == "qa-inspection":
      return handle_qa_inspection(args)
    if args.command == "scenario-exec":
      return cmd_plan_exec(args.action, args.plan_name)
    if args.command == "build":
      return handle_build(args.verbose, args.log_path)
    if args.command == "install":
      return handle_install(args.emulators)
    return 1
  except ValueError as exc:
    print(f"Error: {exc}", file=sys.stderr)
    return 2


if __name__ == "__main__":
  raise SystemExit(main(sys.argv[1:]))
