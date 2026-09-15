#!/usr/bin/env python3
from __future__ import annotations

import argparse
import sys
from pathlib import Path

from qaresultinspector import handle_compare_runs, handle_view_run


def _handle_build(verbose: bool, log_path: str | None) -> int:
  from pebbleadapter import PebbleAdapter
  PebbleAdapter.build(verbose, Path(log_path) if log_path else None)
  return 0


def _handle_install(emulators: list[str]) -> int:
  from pebbleadapter import PebbleAdapter
  from qaharnessconfig import REPO_ROOT

  pbw_path = REPO_ROOT / "build" / "at-a-glance.pbw"
  PebbleAdapter(print).install_emulators(emulators, pbw_path)
  return 0


def _handle_kill(emulators: list[str]) -> int:
  from pebbleadapter import PebbleAdapter
  PebbleAdapter.kill_emulators(emulators)
  return 0


def handle_plan_exec(action: str, plan_name: str) -> int:
  from qaplanexecutor_threaded import resolve_and_execute_plan_concurrently
  # from qaplanexecutor import resolve_and_execute_plan
  if action not in {"run-scenario", "force-scenario"}:
    raise ValueError(f"Unsupported scenario-exec action '{action}'")

  return resolve_and_execute_plan_concurrently(action, plan_name)
  # return resolve_and_execute_plan(action, plan_name)


def handle_validate_plan(target: str) -> int:
  from qaplanresolver import load_and_validate_plan
  if not load_and_validate_plan(target):
    return 1
  else:
    return 0


def handle_plan_dryrun(plan_name: str) -> int:
  return handle_validate_plan(plan_name)


def handle_qa_inspection(args: argparse.Namespace) -> int:
  match args.inspection_command:
    case "view-run":
      return handle_view_run(args.run_selector)
    case "compare":
      return handle_compare_runs(args.run_selectors)
    case "validate":
      return handle_validate_plan(args.plan_name)
    case "dryrun":
      return handle_plan_dryrun(args.plan_name)
    case _:
      return 1


def main(argv: list[str]) -> int:
  try:
    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers(dest="command", required=True)

    qa_inspection = subparsers.add_parser("qa-inspection")
    qa_inspection_subparsers = \
     qa_inspection.add_subparsers(dest="inspection_command", required=True)

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

    install = subparsers.add_parser("kill")
    install.add_argument("--emulators", nargs="+", required=True)

    args = parser.parse_args(argv)

    match args.command:
      case "qa-inspection":
        return handle_qa_inspection(args)
      case "scenario-exec":
        return handle_plan_exec(args.action, args.plan_name)
      case "build":
        return _handle_build(args.verbose, args.log_path)
      case "install":
        return _handle_install(args.emulators)
      case "kill":
        return _handle_kill(args.emulators)
      case _:
        return 1 # Standard fallback if command is unrecognized

  except ValueError as exc:
    print(f"Error: {exc}", file=sys.stderr)
    return 2


if __name__ == "__main__":
  raise SystemExit(main(sys.argv[1:]))
