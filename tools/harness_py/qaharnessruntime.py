from __future__ import annotations

import json
import os
from collections.abc import Callable
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Final, TypedDict, cast

from qaharnessconfig import QARUNS_ROOT
from qaplangrammar import QAPlanGrammar
from qaplanresolver import create_step_for_capability

ALLOWED_RESULTS: Final = frozenset({"passed", "failed"})
ANSI_RESET: Final = "\033[0m"
ANSI_BOLD: Final = "\033[1m"
ANSI_RED: Final = "\033[31m"
ANSI_GREEN: Final = "\033[32m"
ANSI_YELLOW: Final = "\033[33m"
ANSI_CYAN: Final = "\033[36m"

REPORT_STEP_SCHEMA: Final = {
    "weather": {
        "display": str,
        "temp": int,
        "code": int,
        "is_day": int
    },
    "location": {
        "display": str,
        "location": str
    },
    "bluetooth": {
        "display": str,
        "connected": int
    },
    "battery": {
        "display": str,
        "level": int,
        "charging": int,
        "orientation": int,
    },
    "health": {
        "display": str,
        "bpm": int,
        "steps": int
    },
    "all": {
        "display": str,
        "temp": int,
        "code": int,
        "is_day": int,
        "level": int,
        "charging": int,
        "orientation": int,
        "bpm": int,
        "steps": int,
        "location": str,
        "connected": int,
    },
}


@dataclass(frozen=True)
class HarnessRuntime:
  output_root: Path
  commands_log_path: Path
  report_json_path: Path
  report_md_path: Path
  screenshots_dir: Path
  started_at: str # Copied into ConsolidatedQARunOutputs.started_at during `finalize`

  def as_dict(self) -> dict[str, str]:
    return {
        "root": str(self.output_root),
        "commands_log": str(self.commands_log_path),
        "report_json": str(self.report_json_path),
        "summary_md": str(self.report_md_path),
        "screenshots_dir": str(self.screenshots_dir),
    }


class RawStepResult(TypedDict):
  step_result_id: str
  status: str
  screenshot_paths: list[str]
  step_number: int


@dataclass
class ScreenshotsContext:
  expected: int
  captured: int
  paths: list[str]

  def as_dict(self) -> dict[str, Any]:
    return {
        "expected": self.expected,
        "captured": self.captured,
        "paths": list(self.paths),
    }

  @classmethod
  def from_dict(cls, data: dict[str, Any]) -> ScreenshotsContext:
    if "paths" in data:
      paths = data["paths"]
    elif "path" in data:
      paths = [data["path"]]
    else:
      raise ValueError("Invalid screenshot paths in report")

    if (not isinstance(paths, list)) or (not all(
        (isinstance(path, str)) and (path) for path in paths)):
      raise ValueError("Invalid screenshot paths in report")

    required = ("captured", "expected")
    for key in required:
      if key not in data:
        raise ValueError(f"Missing '{key}' in report")
      else:
        _value = data[key]
        if not ((type(_value) is int) and (_value >= 0)):
          raise ValueError("Invalid screenshot counts in report")

    if (data["captured"] == len(paths)) and (data["captured"] <= data["expected"]):
      return cls(
          expected=data["expected"],
          captured=data["captured"],
          paths=paths,
      )
    else:
      raise ValueError("Captured screenshots mismatch with screenshots paths in report")


@dataclass
class QAStepOutput:
  step_id: str
  capability: str
  status: str
  emulator: str
  step_args: dict[str, Any]
  screenshot_ctx: ScreenshotsContext

  def as_dict(self) -> dict[str, Any]:
    return {
        "step_id": self.step_id,
        "capability": self.capability,
        "status": self.status,
        "emulator": self.emulator,
        "step_args": dict(self.step_args),
        "screenshot_ctx": self.screenshot_ctx.as_dict(),
    }

  @classmethod
  def from_dict(cls, data: dict[str, Any]) -> QAStepOutput | None:
    required = ("step_id", "capability", "status", "emulator")
    for key in required:
      if not ((key in data) and (data[key]) and (isinstance(data[key], str))):
        raise ValueError(f"Invalid or missing '{key}' in report")

    _capability: str = data["capability"]
    _emulator: str = data["emulator"]
    _step_args = data.get("step_args")
    if not isinstance(_step_args, dict):
      raise TypeError("Encountered non-canonical step arguments in report")
    if not (("screenshot_ctx" in data) and (isinstance(data.get("screenshot_ctx"), dict))):
      raise ValueError("Invalid screenshots info in report")
    _step_id = data["step_id"]
    _status: str = data["status"]
    screenshot_ctx = ScreenshotsContext.from_dict(data["screenshot_ctx"])
    if validate_report_step(_capability, _emulator, _step_args, _step_id, _status, screenshot_ctx):
      return cls(
          step_id=_step_id,
          capability=_capability,
          status=_status,
          emulator=_emulator,
          step_args=_step_args,
          screenshot_ctx=screenshot_ctx,
      )
    else:
      return None


def validate_report_step(
    capability: str,
    emulator: str,
    step_args: dict[str, Any],
    step_id: str,
    status: str,
    screenshot_ctx: ScreenshotsContext,
) -> bool:
  schema = REPORT_STEP_SCHEMA.get(capability)
  if schema is None or not QAPlanGrammar.is_valid_capability(capability):
    raise ValueError(f"Invalid capability: '{capability}' in report")
  if not QAPlanGrammar.is_valid_emulator(emulator):
    raise ValueError(f"Invalid emulator: '{emulator}' in report")
  if set(step_args) != set(schema):
    raise ValueError(
        f"Invalid report fields for '{capability}': expected {set(schema)}, received {set(step_args)}"
    )
  for name, expected_type in schema.items():
    value = step_args[name]
    if type(value) is not expected_type:
      raise ValueError(f"Invalid JSON type for report field '{name}'")
  if not QAPlanGrammar.is_valid_display_for_emulator(step_args["display"], emulator):
    raise ValueError(f"Invalid display: '{step_args['display']}' for '{emulator}' in report")
  if status not in ALLOWED_RESULTS:
    raise ValueError(f"Invalid step-status: '{status}' for '{step_id}' in report")
  expected_capture = screenshot_ctx.expected > 0
  step = create_step_for_capability(
      capability,
      expected_capture,
      emulator=emulator,
      **step_args,
  )
  if screenshot_ctx.expected != step.expected_screenshots or step.step_id != step_id:
    raise ValueError(f"Step identity or screenshot contract mismatch for '{step_id}'")

  return True


# This is always part of a large payload or dictionary, so TypedDict is the right choice
# for these known keys
class ResolvedScreenshotsContext(TypedDict):
  expected_screenshots: int
  captured_screenshots: int


def parse_resolved_screenshots_context(data: Any) -> ResolvedScreenshotsContext:
  if not isinstance(data, dict):
    raise TypeError("Invalid resolved run context in report")

  required = ("expected_screenshots", "captured_screenshots")
  for key in required:
    if key not in data:
      raise ValueError(f"Missing '{key}' in report")

  if type(data["expected_screenshots"]) is int \
      and type(data["captured_screenshots"]) is int:
    resolved = cast(ResolvedScreenshotsContext, data)
    if (resolved["captured_screenshots"] < 0) or \
        (resolved["expected_screenshots"] < 0) or \
        (resolved["captured_screenshots"] > resolved["expected_screenshots"]):
      raise ValueError("Invalid resolved run context in report")
    else:
      return resolved
  else:
    raise ValueError("Invalid resolved run context in report")


@dataclass
class ConsolidatedQARunOutputs:
  plan: str
  started_at: str
  output_folder: Path
  status: str
  resolved: ResolvedScreenshotsContext
  run_outputs: dict[str, str]
  step_count: int
  step_outputs: list[QAStepOutput]
  inform_operator: Callable[[str], None]

  @property
  def run_id(self) -> str:
    return self.output_folder.name

  def as_dict(self) -> dict[str, Any]:
    return {
        "plan": self.plan,
        "started_at": self.started_at,
        "output_folder": str(self.output_folder),
        "run_id": self.run_id,
        "status": self.status,
        "resolved": dict(self.resolved),
        "run_outputs": dict(self.run_outputs),
        "step_count": self.step_count,
        "step_outputs": [step.as_dict() for step in self.step_outputs],
    }

  @classmethod
  def from_dict(cls, data: Any) -> ConsolidatedQARunOutputs:
    if not isinstance(data, dict):
      raise TypeError("Cannot recreate run context from non-canonical report")

    # Validate primary configuration string keys
    required = ("plan", "status", "started_at", "output_folder", "run_id")
    for key in required:
      if key not in data or not (isinstance(data[key], str) and (data[key])):
        raise ValueError(f"Invalid or missing '{key}' in report")

    _output: Path = Path(data["output_folder"])
    if _output.name != data["run_id"]:
      raise ValueError(f"Run-id and output_folder '{_output.name}' are non-canonical")
    _started_at: str = data["started_at"]
    if data["status"] not in ALLOWED_RESULTS:
      raise ValueError(f"Invalid run status: '{data['status']}' in report")

    required = ("step_count", "run_outputs", "resolved", "step_outputs")
    for key in required:
      if key not in data or not data[key]:
        raise ValueError(f"Invalid or missing '{key}' in report")

    if type(data["step_count"]) is not int or data["step_count"] < 0:
      raise ValueError("Invalid 'step_count' in report")

    # Validate the nested outputs dictionary configuration
    run_outputs_data = data["run_outputs"]
    if not (isinstance(run_outputs_data, dict) and len(run_outputs_data)
            and all(isinstance(k, str) and (isinstance(v, str) and (v))
                    for k, v in run_outputs_data.items())):
      raise ValueError("Run outputs are non-canonical in report")

    steps_data = data["step_outputs"]
    if not isinstance(steps_data, list):
      raise TypeError("Encountered a non-canonical report")

    reconstructed_steps: list[QAStepOutput] = []
    for step_item in steps_data:
      if not isinstance(step_item, dict):
        raise TypeError("Non-canonical steps data in report")
      step_ctx: QAStepOutput = QAStepOutput.from_dict(step_item)
      reconstructed_steps.append(step_ctx)

    if data["step_count"] != len(reconstructed_steps):
      raise ValueError("Run outputs length is not equal to steps executed in report")
    expected_screenshots = sum(step.screenshot_ctx.expected for step in reconstructed_steps)
    captured_screenshots = sum(step.screenshot_ctx.captured for step in reconstructed_steps)
    resolved = parse_resolved_screenshots_context(data["resolved"])
    if resolved["expected_screenshots"] != expected_screenshots or \
        resolved["captured_screenshots"] != captured_screenshots:
      raise ValueError("Resolved screenshot totals do not match report steps")

    return cls(
        plan=data["plan"],
        started_at=_started_at,
        output_folder=_output,
        status=data["status"],
        resolved=resolved,
        run_outputs=cast(dict[str, str], run_outputs_data),
        step_count=data["step_count"],
        step_outputs=reconstructed_steps,
        inform_operator=None,
    )

  def print_closeout(self, log_path: Path, summary_path: Path) -> None:
    _passed = len(self.step_outputs)
    _failed = 0
    if self.status != "passed":
      _passed = sum(step.status == "passed" for step in self.step_outputs)
      _failed = len(self.step_outputs) - _passed

    _status_color = ANSI_GREEN if self.status == "passed" else ANSI_RED
    _failure_color = ANSI_GREEN if _failed == 0 else ANSI_RED
    _screenshot_color = ANSI_GREEN
    if self.resolved["captured_screenshots"] != self.resolved["expected_screenshots"]:
      _screenshot_color = ANSI_YELLOW

    output: str = (
        f"{ANSI_BOLD}QA RUN SUMMARY{ANSI_RESET}\n"
        f"Run status: {_status_color}{ANSI_BOLD}{self.status.upper()}{ANSI_RESET}\n"
        f"Plan: {ANSI_CYAN}{self.plan}{ANSI_RESET}\n"
        f"Steps: {ANSI_GREEN}{_passed} passed{ANSI_RESET}, "
        f"{_failure_color}{_failed} failed{ANSI_RESET}\n"
        f"Screenshots: {_screenshot_color}'{self.resolved['expected_screenshots']}' expected, "
        f"captured '{self.resolved['captured_screenshots']}'{ANSI_RESET}"
    )

    self.inform_operator(output)
    if summary_path.is_file():
      _summary = f"Summary report: {summary_path}"
      _divider = "=" * len(_summary)
      output = (
          f"{ANSI_CYAN}{_divider}{ANSI_RESET}\n"
          f"{ANSI_BOLD}{ANSI_CYAN}{_summary}{ANSI_RESET}\n"
          f"{ANSI_CYAN}{_divider}{ANSI_RESET}"
      )
    self.inform_operator(output)

  def emit_reports(self, log_path: Path, json_path: Path, summary_path: Path) -> None:
    from qareportrenderer import render_report

    json_path.write_text(json.dumps(self.as_dict(), indent=2) + "\n", encoding="utf-8")
    try:
      summary_path.write_text(render_report([self], "# QA Summary Report"), encoding="utf-8")
    except OSError:
      # report.json is durable; view-run can regenerate summary.md.
      pass
    finally:
      if summary_path.is_file():
        self.print_closeout(log_path, summary_path)


# Environment "context" with applicable policies and output paths
# One-time initialization and path creation for outputs
def create_harness_runtime(capture_screenshots: bool) -> HarnessRuntime:
  from datetime import datetime
  started_at = datetime.now().astimezone().strftime("%Y%m%dT%H%M%S")
  output_root = QARUNS_ROOT / f"{started_at}-pid{os.getpid()}"
  commands_log_path = output_root / "commands.log"
  report_json_path = output_root / "report.json"
  report_md_path = output_root / "summary.md"

  output_root.mkdir(parents=True, exist_ok=True)
  commands_log_path.write_text("", encoding="utf-8")
  screenshots_dir = output_root / "screenshots"
  if capture_screenshots:
    screenshots_dir.mkdir(parents=True, exist_ok=True)

  return HarnessRuntime(
      output_root=output_root,
      commands_log_path=commands_log_path,
      report_json_path=report_json_path,
      report_md_path=report_md_path,
      screenshots_dir=screenshots_dir,
      started_at=started_at,
  )
