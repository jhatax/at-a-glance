from __future__ import annotations

from dataclasses import asdict, dataclass
from datetime import datetime, UTC
import json
import os
from pathlib import Path
from typing import Any, TypedDict

from plans import PlanDefinition, expected_screenshot_count


def timestamp_utc() -> str:
  return datetime.now(UTC).strftime("%Y-%m-%dT%H:%M:%SZ")


def generate_run_id() -> str:
  return f"{datetime.now(UTC).strftime('%Y%m%dT%H%M%SZ')}-pid{os.getpid()}"


@dataclass(frozen=True)
class ExecutionContext:
  artifact_root: Path
  commands_log_path: Path
  report_json_path: Path
  report_md_path: Path
  screenshots_dir: Path
  capture_screenshots: bool
  started_at: str


class StepResult(TypedDict):
  name: str
  status: str
  screenshot_paths: list[str]


@dataclass
class ScreenshotsInfo:
  expected: int
  captured: int
  paths: list[str]

  @classmethod
  def from_dict(cls, data: dict[str, Any]) -> ScreenshotsInfo:
    paths = data.get("paths", [data["path"]] if data.get("path") else [])
    if not isinstance(data.get("expected"), int) or not isinstance(data.get("captured"), int):
      raise ValueError("Invalid screenshot counts in report.json")
    if not isinstance(paths, list) or not all(isinstance(path, str) for path in paths):
      raise ValueError("Invalid screenshot paths in report.json")
    steps: list[QAStepContext] = []
    for step in steps_data:
      if not isinstance(step, dict):
        raise ValueError("Invalid step context in report.json")
      steps.append(QAStepContext.from_dict(step))

    return cls(
        expected=data["expected"],
        captured=data["captured"],
        paths=paths,
    )


@dataclass
class StepArtifact:
  artifact_identity: str
  capability: str
  status: str
  emulator: str
  display: str
  inputs: dict[str, str]
  screenshot: ScreenshotsInfo

  @classmethod
  def from_dict(cls, data: dict[str, Any]) -> StepArtifact:
    required_strings = ("artifact_identity", "capability", "status", "emulator", "display")
    if not all(isinstance(data.get(key), str) for key in required_strings):
      raise ValueError("Invalid step artifact strings in report.json")
    if not isinstance(data.get("inputs"), dict) or not all(
        isinstance(key, str) and isinstance(value, str) for key, value in data["inputs"].items()):
      raise ValueError("Invalid step artifact inputs in report.json")
    if not isinstance(data.get("screenshot"), dict):
      raise ValueError("Invalid step artifact screenshot in report.json")
    return cls(
        artifact_identity=data["artifact_identity"],
        capability=data["capability"],
        status=str(data["status"]),
        emulator=data["emulator"],
        display=data["display"],
        inputs=data["inputs"],
        screenshot=ScreenshotsInfo.from_dict(data["screenshot"]),
    )


class ResolvedContext(TypedDict):
  capture_screenshots: bool
  emulators: list[str]


@dataclass
class QARunContext:
  run_id: str
  plan: str
  status: str
  started_at: str
  artifacts: dict[str, str]
  root: str
  resolved: ResolvedContext


@dataclass
class QAStepContext:
  artifact_identity: str
  artifact: StepArtifact

  @classmethod
  def from_dict(cls, data: dict[str, Any]) -> QAStepContext:
    if not isinstance(data.get("artifact_identity"), str):
      raise ValueError("Invalid step identity in report.json")
    if not isinstance(data.get("artifact"), dict):
      raise ValueError("Invalid step artifact in report.json")
    return cls(
        artifact_identity=data["artifact_identity"],
        artifact=StepArtifact.from_dict(data["artifact"]),
    )


@dataclass
class QARunPayload:
  run: QARunContext
  steps: list[QAStepContext]

  def as_dict(self) -> dict[str, Any]:
    return asdict(self)

  @classmethod
  def from_dict(cls, data: dict[str, Any]) -> QARunPayload:
    run_data = data.get("run")
    steps_data = data.get("steps")
    if not isinstance(run_data, dict) or not isinstance(steps_data, list):
      raise ValueError("Invalid QA report structure in report.json")
    required_run_strings = ("run_id", "plan", "status", "started_at", "root")
    if not all(isinstance(run_data.get(key), str) for key in required_run_strings):
      raise ValueError("Invalid run context in report.json")
    if not isinstance(run_data.get("artifacts"), dict) or not all(
        isinstance(key, str) and isinstance(value, str)
        for key, value in run_data["artifacts"].items()):
      raise ValueError("Invalid run artifacts in report.json")
    resolved = run_data.get("resolved")
    if (not isinstance(resolved, dict) or not isinstance(resolved.get("capture_screenshots"), bool)
        or not isinstance(resolved.get("emulators"), list)
        or not all(isinstance(emulator, str) for emulator in resolved["emulators"])):
      raise ValueError("Invalid resolved run context in report.json")
    return cls(
        run=QARunContext(
            run_id=run_data["run_id"],
            plan=run_data["plan"],
            status=run_data["status"],
            started_at=run_data["started_at"],
            artifacts=run_data["artifacts"],
            root=run_data["root"],
            resolved=resolved,
        ),
        steps=steps,
    )


def _write_json(path: Path, payload: dict[str, Any]) -> None:
  path.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")


# Environment "context" with applicable policies and output paths
# One-time initialization and path creation for artifacts
def create_execution_context(QA_DIR: Path, plan: PlanDefinition) -> ExecutionContext:
  artifact_root = QA_DIR / "qa-runs" / generate_run_id()
  commands_log_path = artifact_root / "commands.log"
  report_json_path = artifact_root / "report.json"
  report_md_path = artifact_root / "summary.md"

  artifact_root.mkdir(parents=True, exist_ok=True)
  commands_log_path.write_text("", encoding="utf-8")
  capture_screenshots = plan.screenshots_policy == "yes"
  mandatory_screenshots = any(
      expected_screenshot_count(step.capability, False) > 0 for step in plan.steps.values())
  screenshots_dir = artifact_root / "screenshots"
  if capture_screenshots or mandatory_screenshots:
    screenshots_dir.mkdir(parents=True, exist_ok=True)

  started_at = timestamp_utc()
  return ExecutionContext(
      artifact_root=artifact_root,
      commands_log_path=commands_log_path,
      report_json_path=report_json_path,
      report_md_path=report_md_path,
      screenshots_dir=screenshots_dir,
      capture_screenshots=capture_screenshots,
      started_at=started_at,
  )


def _print_closeout(payload: QARunPayload, summary_path: Path) -> None:
  run = payload.run
  passed = sum(step.artifact.status == "passed" for step in payload.steps)
  failed = len(payload.steps) - passed
  summary = ""
  if summary_path.is_file():
    summary = f"Summary report: {summary_path}"

  print(f"Run status: {run.status}")
  print(f"Plan: {run.plan}")
  print(f"Steps: {passed} passed, failed: {failed}")
  if summary:
    print(summary)


def _emit_reports(payload: QARunPayload, json_path: Path, summary_path: Path) -> None:

  from report import render_report

  payload_dict = payload.as_dict()
  _write_json(json_path, payload_dict)
  summary_written: bool = False
  try:
    summary_path.write_text(render_report([payload], "# QA Summary Report"), encoding="utf-8")
    summary_written = True
  except OSError:
    # report.json is durable; view-run can regenerate summary.md.
    pass

  if summary_written:
    _print_closeout(payload, summary_path)


def _build_artifacts_set(context: ExecutionContext) -> dict[str, str]:

  return {
      "root": str(context.artifact_root),
      "commands_log": str(context.commands_log_path),
      "report_json": str(context.report_json_path),
      "summary_md": str(context.report_md_path),
      "screenshots_dir": str(context.screenshots_dir),
  }


def finalize(
    context: ExecutionContext,
    plan: PlanDefinition,
    exit_status: int,
    step_results: list[StepResult],
) -> int:

  # Create artifacts
  artifacts = _build_artifacts_set(context)
  step_artifacts = _build_step_artifacts(plan, step_results, context.capture_screenshots)

  screenshots_required = any(step.screenshot.expected > 0 for step in step_artifacts.values())
  status = ("passed" if exit_status == 0 and not _missing_artifacts(artifacts, screenshots_required)
            and _required_screenshots_match(context.capture_screenshots, step_artifacts) else
            "failed")

  qa_run_payload = QARunPayload(
      run=QARunContext(
          run_id=context.artifact_root.name,
          plan=plan.name,
          status=status,
          started_at=context.started_at,
          artifacts=artifacts,
          root=str(context.artifact_root),
          resolved={
              "capture_screenshots": context.capture_screenshots,
              "emulators": plan.emulators,
          },
      ),
      steps=[
          QAStepContext(
              artifact_identity=step_artifact.artifact_identity,
              artifact=step_artifact,
          ) for step_artifact in step_artifacts.values()
      ],
  )
  _emit_reports(qa_run_payload, context.report_json_path, context.report_md_path)

  return 0 if status == "passed" else 1


def _build_step_artifacts(plan: PlanDefinition, step_results: list[StepResult],
                          capture_screenshots: bool) -> dict[str, StepArtifact]:

  results_by_identity = {str(item["name"]): item for item in step_results}

  step_rows: dict[str, StepArtifact] = {}
  for step in plan.steps.values():
    step_identity = step.artifact_identity
    result = results_by_identity.get(step_identity)
    if result is None:
      result = StepResult(
          name=step_identity,
          status="unknown",
          screenshot_paths=[],
      )
    screenshot_paths = result["screenshot_paths"]
    step_rows[step_identity] = StepArtifact(
        artifact_identity=step_identity,
        capability=step.capability,
        status=result["status"],
        emulator=step.fields.get("emulator", ""),
        display=step.fields.get("display", ""),
        inputs={
            key: value
            for key, value in step.fields.items() if key.strip() not in {"emulator", "display"}
        },
        screenshot=ScreenshotsInfo(
            expected=expected_screenshot_count(step.capability, capture_screenshots),
            captured=len(screenshot_paths),
            paths=screenshot_paths,
        ),
    )

  return step_rows


def _missing_artifacts(artifacts: dict[str, str], capture_screenshots: bool) -> list[str]:
  required = ["root", "commands_log"]

  if capture_screenshots:
    required.append("screenshots_dir")

  missing: list[str] = []
  for key in required:
    path = artifacts[key]
    if not Path(path).exists():
      missing.append(path)

  return missing


def _required_screenshots_match(capture_screenshots: bool, step_rows: dict[str,
                                                                           StepArtifact]) -> bool:

  expected = sum(step.screenshot.expected for step in step_rows.values())
  captured = sum(step.screenshot.captured for step in step_rows.values())
  if expected:
    return captured >= expected
  if not capture_screenshots:
    return True
  return captured >= expected
