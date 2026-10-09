from __future__ import annotations

import json
from pathlib import Path
from typing import Final

from qaharnessconfig import QA_ROOT, QARUNS_ROOT
from qaharnessruntime import ConsolidatedQARunOutputs
from qareportrenderer import generate_report, render_report_top

COMPARISONS_ROOT: Final = QA_ROOT / "comparisons"
MAX_COMPARISON_RUNS: Final = 5
_initialized: bool = False


def handle_compare_runs(run_selectors: list[str]) -> int:
  from qaplanexecutor_threaded import OUTPUT_SEPARATOR as divider
  print(divider)
  report_path = _compare_runs(run_selectors)
  print(divider)
  print(f"Report available here:\n{report_path}")
  return 0


def handle_view_run(run_selector: str) -> int:
  return handle_compare_runs([run_selector])


def _compare_runs(selectors: list[str]) -> Path:

  global _initialized

  if not 1 <= len(selectors) <= MAX_COMPARISON_RUNS:
    raise ValueError(f"Compare requires <1 to {MAX_COMPARISON_RUNS}> run selectors")

  if not QARUNS_ROOT.is_dir():
    raise ValueError(f"There are no qa-runs to compare '{QARUNS_ROOT}'")

  if len(selectors) == 1:
    return _view_run(selectors[0])

  if not _initialized:
    if not COMPARISONS_ROOT.exists():
      COMPARISONS_ROOT.mkdir(parents=True, exist_ok=True)
    _initialized = True

  # if one of the specified runs cannot be found, an error is raised
  paths: list[Path] = [_resolve_run_root(selector, QARUNS_ROOT) for selector in selectors]

  output_dir: Path = COMPARISONS_ROOT / "__vs__".join((path.stem) for path in paths)

  if not output_dir.exists():
    output_dir.mkdir(parents=True, exist_ok=True)

  # always load payloads so that we can print summary to the console
  payloads = [_load_qarun(path) for path in paths]

  comp_path = output_dir / "comparison.md"
  if comp_path.is_file():
    print("== Report found, printing key info and sharing location ==")
    print(render_report_top(payloads, "# QA Summary Report"))
  else:
    print("== No report available, generating comparison and sharing file-path ==")
    with output_dir / "comparison.json".open("w", encoding="utf-8") as handle:
      json.dump({"runs": [payload.as_dict() for payload in payloads]}, handle, indent=2)
      handle.write("\n")

    with comp_path.open("w", encoding="utf-8") as handle:
      handle.write(f"{generate_report(payloads, '# QA Summary Report')}\n")
  return comp_path


def write_summary_report(run_root: Path, summary_path: Path) -> str:
  run = _load_qarun(run_root)
  _report = f"{generate_report([run], '# QA Summary Report')}\n"
  with summary_path.open("w", encoding="utf-8") as handle:
    handle.write(_report)
  return _report


def _view_run(selector: str) -> Path:
  run_root = _resolve_run_root(selector, QARUNS_ROOT)
  summary_path = run_root / "summary.md"
  if summary_path.is_file():
    print("== Report found, sharing file-path ==")
  else:
    print("== No report available, generating summary and sharing location ==")
    write_summary_report(run_root, summary_path)
  return summary_path


def _load_qarun(root: Path) -> ConsolidatedQARunOutputs:
  report_path = root / "report.json"
  if not report_path.is_file():
    raise ValueError(f"'{root}' is missing report.json")
  return ConsolidatedQARunOutputs.from_dict(json.loads(report_path.read_text(encoding="utf-8")))


def _resolve_run_root(selector: str, runs_root: Path) -> Path:
  selector_path = Path(selector)
  if selector_path.is_dir():
    return selector_path.resolve()
  candidate = runs_root / selector
  if candidate.is_dir():
    return candidate.resolve()
  raise ValueError(f"Unknown run selector '{selector}'")
