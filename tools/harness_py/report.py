from __future__ import annotations

from datetime import datetime
from html import escape
from pathlib import Path

from runtime import QARunPayload, QAStepContext


def render_report(payloads: list[QARunPayload], heading: str) -> str:
  runs = [payload.run for payload in payloads]
  lines = [
      heading,
      "",
      "| Detail | " + " | ".join(f"Run-{i + 1} Output" for i in range(len(runs))) + " |",
      "| --- | " + " | ".join("---:" for _ in runs) + " |",
      _row("**Date**", [_format_date(run.started_at) for run in runs]),
      _row("**Status**", [run.status for run in runs]),
      _row("**QA Plan Executed**", [f"`{run.plan}`" for run in runs]),
      _row("**Screenshots**", [_screenshot_summary(payload) for payload in payloads]),
      _row("**Steps Run**", [str(len(payload.steps)) for payload in payloads]),
  ]

  step_rows = [payload.steps for payload in payloads]
  for index in range(max((len(steps) for steps in step_rows), default=0)):
    entries = [steps[index] if index < len(steps) else None for steps in step_rows]
    lines.append(_row(f"**Step-{index + 1}**", [_step_cell(entry) for entry in entries]))

  lines.extend([
      _row("**Logs**", [_artifact_link(run.artifacts.get("commands_log", "")) for run in runs]),
      _row("**JSON**", [_artifact_link(run.artifacts.get("report_json", "")) for run in runs]),
  ])
  return "\n".join(lines)


def _screenshot_summary(payload: QARunPayload) -> str:
  expected = captured = 0
  for step in payload.steps:
    expected += step.artifact.screenshot.expected
    captured += step.artifact.screenshot.captured
  return f"Expected: **{expected}**; Captured: **{captured}**"


def _step_cell(step: QAStepContext | None) -> str:
  if not step:
    return "missing step"
  entry = step.artifact
  lines = [
      entry.capability,
      f"Emulator: {entry.emulator or 'none'}",
      f"Theme: {entry.display or 'none'}",
  ]
  lines.extend(f"{key}: {value}" for key, value in entry.inputs.items())
  for index, path in enumerate(entry.screenshot.paths, start=1):
    image_path = escape(str(path), quote=True)
    lines.append(f'<img src="{image_path}" alt="Screenshot {index}">')
  return "<br>".join(lines)


def _artifact_link(path: str) -> str:
  if not path:
    return "none"
  return f"[{Path(path).name}]({path})"


def _row(label: str, values: list[str]) -> str:
  return "| " + " | ".join([label, *values]) + " |"


def _format_date(value: str) -> str:
  timestamp = datetime.fromisoformat(value.replace("Z", "+00:00"))
  return timestamp.astimezone().strftime("%B %-d, %Y at %-I:%M:%S %p %Z")
