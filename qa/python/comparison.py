from __future__ import annotations

from dataclasses import dataclass
import json
from pathlib import Path
from typing import Any


@dataclass(frozen=True)
class RunArtifact:
    run_id: str
    root: Path
    scenario: str
    status: str
    started_at: str
    summary: dict[str, int]
    assertions: list[dict[str, str]]
    stages: list[dict[str, str]]
    steps: dict[str, dict[str, Any]]
    artifacts: dict[str, str]


def compare_runs(selectors: list[str], qa_root: Path) -> Path:
    if not 2 <= len(selectors) <= 3:
        raise ValueError("compare requires two or three run selectors")

    runs_root = qa_root / "qa-runs"
    comparisons_root = qa_root / "comparisons"
    comparisons_root.mkdir(parents=True, exist_ok=True)

    runs = [load_run_artifact(selector, runs_root) for selector in selectors]
    payload = build_comparison_payload(runs, qa_root)

    output_dir = comparisons_root / "__vs__".join(run.run_id for run in runs)
    output_dir.mkdir(parents=True, exist_ok=True)

    json_path = output_dir / "comparison.json"
    md_path = output_dir / "comparison.md"
    json_path.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
    md_path.write_text(render_comparison_markdown(payload), encoding="utf-8")
    return output_dir


def resolve_report_path(selector: str, qa_root: Path) -> Path:
    runs_root = qa_root / "qa-runs"
    run_root = resolve_run_root(selector, runs_root)
    report_path = run_root / "report.md"
    if not report_path.is_file():
        raise ValueError(f"Run '{selector}' is missing report.md")
    return report_path


def load_run_artifact(selector: str, runs_root: Path) -> RunArtifact:
    root = resolve_run_root(selector, runs_root)
    report_path = root / "report.json"
    if not report_path.is_file():
        raise ValueError(f"Run '{selector}' is missing report.json")

    payload = json.loads(report_path.read_text(encoding="utf-8"))
    return RunArtifact(
        run_id=payload["run_id"],
        root=root,
        scenario=payload.get("scenario", "unknown"),
        status=payload.get("status", "unknown"),
        started_at=payload.get("started_at", ""),
        summary=payload.get("summary", {}),
        assertions=payload.get("assertions", []),
        stages=payload.get("stages", []),
        steps=payload.get("steps", {}),
        artifacts=payload.get("artifacts", {}),
    )


def resolve_run_root(selector: str, runs_root: Path) -> Path:
    selector_path = Path(selector)
    if selector_path.exists():
        return selector_path.resolve()

    candidate = runs_root / selector
    if candidate.is_dir():
        return candidate.resolve()

    raise ValueError(f"Unknown run selector '{selector}'")


def build_comparison_payload(runs: list[RunArtifact], qa_root: Path) -> dict[str, Any]:
    all_keys: set[str] = set()
    for run in runs:
        all_keys.update(run.steps.keys())

    step_rows: list[dict[str, Any]] = []
    for artifact_identity in sorted(all_keys):
        step_rows.append(
            {
                "artifact_identity": artifact_identity,
                "entries": [step_entry_to_payload(run.steps.get(artifact_identity)) for run in runs],
            }
        )

    return {
        "runs": [run_to_payload(run, qa_root) for run in runs],
        "step_rows": step_rows,
    }


def run_to_payload(run: RunArtifact, qa_root: Path) -> dict[str, Any]:
    return {
        "run_id": run.run_id,
        "scenario": run.scenario,
        "status": run.status,
        "started_at": run.started_at,
        "summary": run.summary,
        "assertions": run.assertions,
        "stages": run.stages,
        "steps": run.steps,
        "artifacts": run.artifacts,
        "root": path_relative_to_root(run.root, qa_root),
    }


def step_entry_to_payload(step: dict[str, Any] | None) -> dict[str, Any] | None:
    if step is None:
        return None
    screenshot = step.get("screenshot", {})
    return {
        "artifact_identity": step.get("artifact_identity", ""),
        "capability": step.get("capability", ""),
        "status": step.get("status", "unknown"),
        "emulator": step.get("emulator", ""),
        "display": step.get("display", ""),
        "inputs": step.get("inputs", {}),
        "screenshot": {
            "expected": screenshot.get("expected", 0),
            "captured": screenshot.get("captured", 0),
            "path": screenshot.get("path", ""),
        },
        "logs": step.get("logs", []),
    }


def path_relative_to_root(path: Path, qa_root: Path) -> str:
    try:
        return str(path.relative_to(qa_root.parent))
    except ValueError:
        return str(path)


def render_comparison_markdown(payload: dict[str, Any]) -> str:
    runs = payload["runs"]
    lines = [
        "# QA Comparison Report",
        "",
        "| Detail | " + " | ".join(f"Run-{index + 1} Output" for index in range(len(runs))) + " |",
        "| :--- | " + " | ".join(":---" for _ in runs) + " |",
        _comparison_row("Date", [f"`{run['started_at']}`" for run in runs]),
        _comparison_row("Status", [run["status"] for run in runs]),
        _comparison_row("Scenario", [f"`{run['scenario']}`" for run in runs]),
        _comparison_row("Assertions", [_assertion_summary(run["assertions"]) for run in runs]),
        _comparison_row("Stages Executed", [_stage_summary(run["stages"]) for run in runs]),
        _comparison_row("Steps", [_step_summary(run["summary"]) for run in runs]),
        _comparison_row("Commands Log", [_file_link(run["artifacts"].get("commands_log", "")) for run in runs]),
        _comparison_row("Logs", [_logs_summary(run["artifacts"].get("logs_dir", "")) for run in runs]),
        _comparison_row("Run JSON", [_file_link(run["artifacts"].get("run_json", "")) for run in runs]),
        _comparison_row("Report JSON", [_file_link(run["artifacts"].get("report_json", "")) for run in runs]),
    ]
    lines.extend(_comparison_step_rows(payload["step_rows"]))
    lines.append("")
    return "\n".join(lines)


def _assertion_summary(assertions: list[dict[str, str]]) -> str:
    if not assertions:
        return "none"
    return "<br>".join(f"`{item['status']}` `{item['name']}`" for item in assertions)


def _stage_summary(stages: list[dict[str, str]]) -> str:
    if not stages:
        return "none"
    return "<br>".join(f"`{item['status']}` `{item['name']}`" for item in stages)


def _step_summary(summary: dict[str, Any]) -> str:
    if not summary:
        return "none"
    return "<br>".join(
        [
            f"`steps` `{summary.get('steps', 0)}`",
            f"`assertions` `{summary.get('assertions', 0)}`",
            f"`screenshots` `{summary.get('screenshots', 0)}`",
        ]
    )


def _logs_summary(logs_dir: str) -> str:
    if not logs_dir:
        return "none"
    logs_root = Path(logs_dir)
    if not logs_root.exists():
        return "none"
    log_paths = sorted(logs_root.glob("*.log"))
    if not log_paths:
        return _file_link(str(logs_root), "logs/")
    links = "<br>".join(f"&nbsp;&nbsp;- {_file_link(str(path))}" for path in log_paths)
    return f"{_file_link(str(logs_root), 'logs/')}<br>{links}"


def _file_link(path_str: str, label: str | None = None) -> str:
    if not path_str:
        return "none"
    path = Path(path_str)
    return f"<a href=\"{path}\">{label or path.name}</a>"


def _comparison_row(label: str, values: list[str]) -> str:
    return "| " + " | ".join([label, *values]) + " |"


def _comparison_step_rows(rows: list[dict[str, Any]]) -> list[str]:
    rendered: list[str] = []
    for row in rows:
        rendered.append(
            _comparison_row(
                f"Step: {row['artifact_identity']}",
                [_comparison_step_cell(entry, row["artifact_identity"]) for entry in row["entries"]],
            )
        )
    return rendered


def _comparison_step_cell(entry: dict[str, Any] | None, artifact_identity: str) -> str:
    if not entry:
        return "missing step"

    screenshot = entry.get("screenshot", {})
    logs = entry.get("logs", [])
    parts = [
        f"`{entry.get('status', 'unknown')}`",
        f"`{entry.get('capability', '')}`",
        f"emulator `{entry.get('emulator', '')}`" if entry.get("emulator") else "emulator none",
        f"display `{entry.get('display', '')}`" if entry.get("display") else "display none",
        f"screenshot expected `{screenshot.get('expected', 0)}` captured `{screenshot.get('captured', 0)}`",
    ]
    if logs:
        parts.append("<br>".join(_file_link(path) for path in logs))
    return "<br>".join(parts)
