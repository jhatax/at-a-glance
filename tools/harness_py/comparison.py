from __future__ import annotations

import json
from pathlib import Path
from typing import Final

from config import QA_ROOT
from report import render_report
from runtime import QARunPayload

COMPARISONS_ROOT: Final = QA_ROOT / "comparisons"
RUNS_ROOT: Final = QA_ROOT / "qa-runs"
MAX_COMPARISON_RUNS: Final = 5
_initialized: bool = False


def compare_runs(selectors: list[str]) -> Path:

  global _initialized
  if not 1 <= len(selectors) <= MAX_COMPARISON_RUNS:
    raise ValueError(f"compare requires one to {MAX_COMPARISON_RUNS} run selectors")

  if not RUNS_ROOT.is_dir():
    raise ValueError(f"There are no qa-runs to compare '{RUNS_ROOT}'")

  if len(selectors) == 1:
    return view_run(selectors[0])

  if not _initialized:
    if not COMPARISONS_ROOT.exists():
      COMPARISONS_ROOT.mkdir(parents=True, exist_ok=True)
    _initialized = True

  # if one of the specified runs cannot be found, an error is raised
  paths: list[Path] = [resolve_run_root(selector, RUNS_ROOT) for selector in selectors]

  output_dir: Path = COMPARISONS_ROOT / "__vs__".join((path.stem) for path in paths)

  if output_dir.exists():
    # This comparison has already been done before
    comp_path = output_dir / "comparison.md"
    if comp_path.is_file():
      return comp_path

  if not output_dir.exists():
    output_dir.mkdir(parents=True, exist_ok=True)

  payloads = [load_qarun_payload(path) for path in paths]
  (output_dir / "comparison.json").write_text(json.dumps({"payloads": payloads}, indent=2) + "\n",
                                              encoding="utf-8")
  (output_dir / "comparison.md").write_text(render_report(payloads, "# QA Comparison Report"),
                                            encoding="utf-8")
  return output_dir / "comparison.md"


def view_run(selector: str) -> Path:
  run_root = resolve_run_root(selector, RUNS_ROOT)
  summary_path = run_root / "summary.md"
  if summary_path.is_file():
    return summary_path
  payload = load_qarun_payload(run_root)
  summary_path.write_text(render_report([payload], "# QA Summary Report"), encoding="utf-8")
  return summary_path


def load_qarun_payload(root: Path) -> QARunPayload:
  report_path = root / "report.json"
  if not report_path.is_file():
    raise ValueError(f"'{root}' is missing report.json")
  payload = json.loads(report_path.read_text(encoding="utf-8"))
  if not isinstance(payload, dict) or "run" not in payload or "steps" not in payload:
    raise ValueError(f"'{root}' has a non-canonical report.json")
  return QARunPayload.from_dict(payload)


def resolve_run_root(selector: str, runs_root: Path) -> Path:
  selector_path = Path(selector)
  if selector_path.is_dir():
    return selector_path.resolve()
  candidate = runs_root / selector
  if candidate.is_dir():
    return candidate.resolve()
  raise ValueError(f"Unknown run selector '{selector}'")
