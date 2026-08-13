from __future__ import annotations

from datetime import datetime
from html import escape
from pathlib import Path

from qaharnessruntime import QARunContext, QAStepContext


def render_report_top(runs: list[QARunContext], heading: str) -> str:
  lines = [
      heading,
      "",
      "| Detail | " + " | ".join(f"Run-{i + 1} Output" for i in range(len(runs))) + " |",
      "| --- | " + " | ".join("---:" for _ in runs) + " |",
      _row("**Date**", [_format_date(run.started_at) for run in runs]),
      _row("**Status**", [run.status for run in runs]),
      _row("**QA Plan Executed**", [f"`{run.plan}`" for run in runs]),
      _row("**Screenshots**", [_screenshot_summary(run) for run in runs]),
      _row("**Steps Run**", [str(len(run.step_outputs)) for run in runs]),
  ]

  joined = "\n".join(lines)
  return joined


def _render_report_steps(runs: list[QARunContext]) -> str:
  step_rows = [run.step_outputs for run in runs]
  lines: list[str] = []
  _max: int = max(len(steps) for steps in step_rows)
  for index in range(_max):
    entries = [steps[index] if index < len(steps) else None for steps in step_rows]
    lines.append(_row(f"**Step-{index + 1}**", [_step_cell(entry) for entry in entries]))

  return "\n".join(lines)


def _render_report_outputs(runs: list[QARunContext]) -> str:
  lines = [
      _row("**Logs**", [_output_link(run.run_outputs.get("commands_log", "")) for run in runs]),
      _row("**JSON**", [_output_link(run.run_outputs.get("report_json", "")) for run in runs]),
  ]
  return "\n".join(lines)


def render_report(runs: list[QARunContext], heading: str) -> str:
  if not runs:
    raise ValueError("You must specify at least one run to render into report")
  top = render_report_top(runs, heading)
  steps = _render_report_steps(runs)
  outputs = _render_report_outputs(runs)
  return f"{top}\n{steps}\n{outputs}"


def _screenshot_summary(run: QARunContext) -> str:
  return (
      f"Expected: **{run.resolved['expected_screenshots']}**; "
      f"Captured: **{run.resolved['captured_screenshots']}**"
  )


def _step_cell(step: QAStepContext | None) -> str:
  if not step:
    return "missing step"
  lines = [
      f"Result: **{step.status.upper()}**",
      f"Emulator: {step.emulator or 'none'}",
  ]
  lines.extend(f"{key}: {value}" for key, value in step.step_args.items())
  for index, path in enumerate(step.screenshot_ctx.paths, start=1):
    image_path = escape(str(path), quote=True)
    lines.append(f'<img src="{image_path}" alt="Screenshot {index}">')
  return "<br>".join(lines)


def _output_link(path: str) -> str:
  if not path:
    return "none"
  return f"[{Path(path).name}]({path})"


def _row(label: str, values: list[str]) -> str:
  return "| " + " | ".join([label, *values]) + " |"


def _format_date(value: str) -> str:
  timestamp = datetime.fromisoformat(value.replace("Z", "+00:00"))
  return timestamp.astimezone().strftime("%B %-d, %Y at %-I:%M:%S %p %Z")
