from __future__ import annotations

from dataclasses import dataclass
from datetime import datetime, UTC
import json
import os
from pathlib import Path
from typing import Any

from .state import CliState
from .scenarios import ScenarioDefinition, ScenarioStep, expected_step_screenshot_count


def timestamp_utc() -> str:
    return datetime.now(UTC).strftime("%Y-%m-%dT%H:%M:%SZ")


def generate_run_id() -> str:
    return f"{datetime.now(UTC).strftime('%Y%m%dT%H%M%SZ')}-pid{os.getpid()}"


@dataclass(frozen=True)
class BootstrapResult:
    run_id: str
    artifact_root: Path
    commands_log_path: Path
    run_json_path: Path
    report_json_path: Path
    report_md_path: Path
    logs_dir: Path
    screenshots_dir: Path
    screenshot_index_path: Path
    build_log_path: Path
    scenario_steps_json_path: Path
    effective_scenario: str
    capture_screenshots: bool
    scenario_path: str
    run_qa: bool
    install: bool
    phone_install: bool
    phone_ip: str
    emulators_csv: str


def _write_json(path: Path, payload: dict[str, Any]) -> None:
    path.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")


def _list_from_csv(value: str) -> list[str]:
    if not value:
        return []
    return [item for item in value.split(",") if item]


def _scenario_emulators(scenario: ScenarioDefinition) -> list[str]:
    emulators: list[str] = []
    for step in scenario.steps:
        emulator = step.fields.get("emulator", "")
        if emulator and emulator not in emulators:
            emulators.append(emulator)
    return emulators


def bootstrap(cli_state: CliState, scenario: ScenarioDefinition | None) -> BootstrapResult:
    run_id = generate_run_id()
    artifact_root = cli_state.qa_dir / "qa-runs" / run_id
    logs_dir = artifact_root / "logs"
    screenshots_dir = artifact_root / "screenshots"
    commands_log_path = artifact_root / "commands.log"
    run_json_path = artifact_root / "run.json"
    report_json_path = artifact_root / "report.json"
    report_md_path = artifact_root / "report.md"
    screenshot_index_path = screenshots_dir / "index.tsv"
    build_log_path = logs_dir / "build.log"
    scenario_steps_json_path = artifact_root / "scenario_steps.json"

    logs_dir.mkdir(parents=True, exist_ok=True)
    screenshots_dir.mkdir(parents=True, exist_ok=True)
    commands_log_path.write_text("", encoding="utf-8")
    screenshot_index_path.write_text(
        "scenario\tcapability\tartifact_identity\temulator\tstate_tags\trelative_path\n",
        encoding="utf-8",
    )

    if scenario is None:
        effective_scenario = "adhoc"
        capture_screenshots = False
        scenario_path = ""
        resolved_run_qa = cli_state.run_qa
        resolved_install = cli_state.install
        resolved_phone_install = cli_state.phone_install
        resolved_phone_ip = cli_state.phone_ip
        resolved_emulators = cli_state.emulators
        scenario_steps: list[dict[str, str]] = []
    else:
        effective_scenario = scenario.name
        capture_screenshots = scenario.screenshots_policy == "yes"
        scenario_path = str(scenario.path)
        resolved_run_qa = True
        resolved_install = True
        resolved_phone_install = False
        resolved_phone_ip = ""
        resolved_emulators = _scenario_emulators(scenario)
        scenario_steps = [step.as_dict() for step in scenario.steps]

    _write_json(scenario_steps_json_path, {"steps": scenario_steps})

    run_payload = {
        "run_id": run_id,
        "status": "running",
        "started_at": timestamp_utc(),
        "ended_at": "",
        "scenario": effective_scenario,
        "entrypoint": str(cli_state.script_dir / "aag-build-qa.sh"),
        "cwd": str(cli_state.script_dir),
        "artifacts_root": str(artifact_root),
        "requested": {
            "build": cli_state.build,
            "clean": cli_state.clean,
            "install": cli_state.install,
            "wipe": cli_state.wipe,
            "nuke": cli_state.nuke,
            "run_qa": cli_state.run_qa,
            "phone_install": cli_state.phone_install,
            "phone_ip": cli_state.phone_ip,
            "scenario": cli_state.scenario_name,
            "scenario_path": scenario_path,
            "emulators": cli_state.emulators,
            "tests": cli_state.tests,
        },
        "resolved": {
            "run_qa": resolved_run_qa,
            "install": resolved_install,
            "phone_install": resolved_phone_install,
            "phone_ip": resolved_phone_ip,
            "scenario": effective_scenario,
            "capture_screenshots": capture_screenshots,
            "emulators": resolved_emulators,
            "tests": resolved_tests,
            "scenario_steps_json": str(scenario_steps_json_path),
        },
    }
    _write_json(run_json_path, run_payload)

    return BootstrapResult(
        run_id=run_id,
        artifact_root=artifact_root,
        commands_log_path=commands_log_path,
        run_json_path=run_json_path,
        report_json_path=report_json_path,
        report_md_path=report_md_path,
        logs_dir=logs_dir,
        screenshots_dir=screenshots_dir,
        screenshot_index_path=screenshot_index_path,
        build_log_path=build_log_path,
        scenario_steps_json_path=scenario_steps_json_path,
        effective_scenario=effective_scenario,
        capture_screenshots=capture_screenshots,
        scenario_path=scenario_path,
        run_qa=resolved_run_qa,
        install=resolved_install,
        phone_install=resolved_phone_install,
        phone_ip=resolved_phone_ip,
        emulators_csv=",".join(resolved_emulators),
    )


def bootstrap_records(result: BootstrapResult) -> list[tuple[str, str]]:
    exports = {
        "QA_RUN_ID": result.run_id,
        "QA_EFFECTIVE_SCENARIO": result.effective_scenario,
        "QA_CAPTURE_SCREENSHOTS": "true" if result.capture_screenshots else "false",
        "QA_ARTIFACTS_ROOT": str(result.artifact_root),
        "QA_COMMANDS_LOG_PATH": str(result.commands_log_path),
        "QA_RUN_JSON_PATH": str(result.run_json_path),
        "QA_REPORT_JSON_PATH": str(result.report_json_path),
        "QA_REPORT_MD_PATH": str(result.report_md_path),
        "QA_LOGS_DIR": str(result.logs_dir),
        "QA_SCREENSHOTS_DIR": str(result.screenshots_dir),
        "QA_SCREENSHOT_INDEX_PATH": str(result.screenshot_index_path),
        "QA_SCENARIO_STEPS_JSON_PATH": str(result.scenario_steps_json_path),
        "BUILD_LOG_PATH": str(result.build_log_path),
        "RUN_QA": "true" if result.run_qa else "false",
        "INSTALL": "true" if result.install else "false",
        "PHONE_INSTALL": "true" if result.phone_install else "false",
        "PHONE_IP": result.phone_ip,
        "EMULATORS_CSV": result.emulators_csv,
    }
    return list(exports.items())


def _read_existing_run_payload(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def finalize_from_env() -> int:
    run_json_path = Path(os.environ["QA_RUN_JSON_PATH"])
    report_json_path = Path(os.environ["QA_REPORT_JSON_PATH"])
    report_md_path = Path(os.environ["QA_REPORT_MD_PATH"])
    screenshot_index_path = Path(os.environ["QA_SCREENSHOT_INDEX_PATH"])

    run_payload = _read_existing_run_payload(run_json_path)
    exit_status = int(os.environ.get("QA_EXIT_STATUS", "0"))
    stage_records = [line for line in os.environ.get("QA_STAGE_RESULTS_LINES", "").splitlines() if line]
    test_records = [line for line in os.environ.get("QA_TEST_RESULTS_LINES", "").splitlines() if line]
    assertion_records = [
        line for line in os.environ.get("QA_ASSERTION_RESULTS_LINES", "").splitlines() if line
    ]

    ended_at = timestamp_utc()
    stage_items = [{"name": name, "status": result} for name, result in _split_records(stage_records)]
    test_items = [{"name": name, "status": result} for name, result in _split_records(test_records)]
    assertions = [
        {"name": name, "status": result, "detail": detail}
        for name, result, detail in _split_assertion_records(assertion_records)
    ]
    scenario_name = os.environ.get("QA_EFFECTIVE_SCENARIO", run_payload.get("scenario", "adhoc"))
    artifacts = {
        "root": os.environ["QA_ARTIFACTS_ROOT"],
        "run_json": os.environ["QA_RUN_JSON_PATH"],
        "commands_log": os.environ["QA_COMMANDS_LOG_PATH"],
        "report_json": os.environ["QA_REPORT_JSON_PATH"],
        "report_md": os.environ["QA_REPORT_MD_PATH"],
        "build_log": os.environ.get("BUILD_LOG_PATH", ""),
        "compile_commands": os.environ.get("COMPILE_DB_PATH", ""),
        "screenshots_dir": os.environ["QA_SCREENSHOTS_DIR"],
        "screenshot_index": os.environ["QA_SCREENSHOT_INDEX_PATH"],
        "logs_dir": os.environ["QA_LOGS_DIR"],
    }
    step_rows = _build_step_rows(run_payload, scenario_name, artifacts["screenshot_index"], test_items)
    report_payload = _build_report_payload(
        run_payload=run_payload,
        scenario_name=scenario_name,
        ended_at=ended_at,
        exit_status=exit_status,
        stage_items=stage_items,
        step_rows=step_rows,
        assertions=assertions,
        artifacts=artifacts,
    )

    assertions.extend(_artifact_presence_assertions(report_payload["artifacts"]))
    screenshot_assertion = _required_screenshot_assertion(
        run_payload=run_payload,
        report_payload=report_payload,
        screenshot_index_path=screenshot_index_path,
    )
    if screenshot_assertion is not None:
        assertions.append(screenshot_assertion)

    status = "passed" if exit_status == 0 and _all_assertions_passed(assertions) else "failed"
    operator_actions = _operator_actions(stage_items, assertions, step_rows)
    report_payload["status"] = status
    report_payload["run"]["status"] = status
    report_payload["assertions"] = assertions
    report_payload["operator_actions"] = operator_actions
    report_payload["summary"] = _report_summary(stage_items, step_rows, assertions, report_payload["summary"]["screenshots"])
    run_payload["status"] = status
    run_payload["ended_at"] = ended_at
    _write_json(run_json_path, run_payload)
    _write_json(report_json_path, report_payload)
    report_md_path.write_text(_report_markdown(report_payload), encoding="utf-8")
    return 0 if status == "passed" else 1


def _split_records(records: list[str]) -> list[tuple[str, str]]:
    split: list[tuple[str, str]] = []
    for record in records:
        name, status = record.split("|", 1)
        split.append((name, status))
    return split


def _split_assertion_records(records: list[str]) -> list[tuple[str, str, str]]:
    split: list[tuple[str, str, str]] = []
    for record in records:
        name, status, detail = record.split("|", 2)
        split.append((name, status, detail))
    return split


def _assertion(name: str, status: str, detail: str) -> dict[str, str]:
    return {"name": name, "status": status, "detail": detail}


def _build_step_rows(
    run_payload: dict[str, Any],
    scenario_name: str,
    screenshot_index_path: str,
    test_items: list[dict[str, str]],
) -> dict[str, dict[str, Any]]:
    resolved = run_payload.get("resolved", {})
    scenario_steps_path = Path(resolved.get("scenario_steps_json", ""))
    if not scenario_steps_path.is_file():
        return {}

    steps_payload = json.loads(scenario_steps_path.read_text(encoding="utf-8"))
    screenshot_rows = _load_screenshot_rows(screenshot_index_path)
    screenshots_by_identity = {row["identity"]: row for row in screenshot_rows}
    test_status_by_identity = {item["name"]: item["status"] for item in test_items}

    step_rows: dict[str, dict[str, Any]] = {}
    for item in steps_payload.get("steps", []):
        step = ScenarioStep.from_dict(item)
        screenshot = screenshots_by_identity.get(step.artifact_identity)
        step_rows[step.artifact_identity] = {
            "artifact_identity": step.artifact_identity,
            "capability": step.capability,
            "status": test_status_by_identity.get(step.artifact_identity, "unknown"),
            "emulator": step.fields.get("emulator", ""),
            "display": step.fields.get("display", ""),
            "inputs": {key: value for key, value in step.fields.items() if key not in {"emulator", "display"}},
            "screenshot": {
                "expected": expected_step_screenshot_count(step),
                "captured": 1 if screenshot and screenshot["exists"] == "true" else 0,
                "path": screenshot["absolute_path"] if screenshot else "",
            },
            "logs": [],
        }

    return step_rows


def _build_report_payload(
    *,
    run_payload: dict[str, Any],
    scenario_name: str,
    ended_at: str,
    exit_status: int,
    stage_items: list[dict[str, str]],
    step_rows: dict[str, dict[str, Any]],
    assertions: list[dict[str, str]],
    artifacts: dict[str, str],
) -> dict[str, Any]:
    run_section = {
        "run_id": run_payload["run_id"],
        "status": "running",
        "started_at": run_payload["started_at"],
        "ended_at": ended_at,
        "scenario": scenario_name,
        "exit_status": exit_status,
    }
    summary = _report_summary(stage_items, step_rows, assertions, int(os.environ.get("QA_SCREENSHOT_COUNT", "0")))
    return {
        "run": run_section,
        "summary": summary,
        "stages": stage_items,
        "steps": step_rows,
        "assertions": assertions,
        "artifacts": artifacts,
        "operator_actions": [],
        "run_id": run_section["run_id"],
        "status": run_section["status"],
        "started_at": run_section["started_at"],
        "ended_at": run_section["ended_at"],
        "exit_status": run_section["exit_status"],
        "scenario": run_section["scenario"],
        "counts": {
            "commands": int(os.environ.get("QA_COMMAND_COUNT", "0")),
            "screenshots": int(os.environ.get("QA_SCREENSHOT_COUNT", "0")),
        },
        "requested": run_payload.get("requested", {}),
        "resolved": run_payload.get("resolved", {}),
        "tests": [],
        "manual_signoff": _manual_signoff_items(scenario_name),
        "active_stage": os.environ.get("QA_ACTIVE_STAGE", ""),
        "active_test": os.environ.get("QA_ACTIVE_TEST", ""),
    }


def _report_summary(
    stage_items: list[dict[str, str]],
    step_rows: dict[str, dict[str, Any]],
    assertions: list[dict[str, str]],
    screenshots_count: int,
) -> dict[str, int]:
    return {
        "stages": len(stage_items),
        "steps": len(step_rows),
        "assertions": len(assertions),
        "screenshots": screenshots_count,
    }


def _all_assertions_passed(assertions: list[dict[str, str]]) -> bool:
    return all(item["status"] == "passed" for item in assertions)


def _artifact_presence_assertions(artifacts: dict[str, str]) -> list[dict[str, str]]:
    checks = [
        ("artifact-root-present", artifacts["root"]),
        ("artifact-run-json-present", artifacts["run_json"]),
        ("artifact-commands-log-present", artifacts["commands_log"]),
        ("artifact-report-json-present", artifacts["report_json"]),
        ("artifact-report-md-present", artifacts["report_md"]),
        ("artifact-screenshot-index-present", artifacts["screenshot_index"]),
        ("artifact-logs-dir-present", artifacts["logs_dir"]),
        ("artifact-screenshots-dir-present", artifacts["screenshots_dir"]),
    ]

    assertions: list[dict[str, str]] = []
    for name, path_str in checks:
        path = Path(path_str)
        exists = path.exists()
        path_kind = "directory" if path.is_dir() else "file"
        detail = f"{path_kind} {'present' if exists else 'missing'} at {path}"
        assertions.append(_assertion(name, "passed" if exists else "failed", detail))
    return assertions


def _stage_passed(report_payload: dict[str, Any], stage_name: str) -> bool:
    return any(item["name"] == stage_name and item["status"] == "passed" for item in report_payload["stages"])


def _required_screenshot_assertion(
    run_payload: dict[str, Any],
    report_payload: dict[str, Any],
    screenshot_index_path: Path,
) -> dict[str, str] | None:
    resolved = run_payload.get("resolved", {})
    if not resolved.get("capture_screenshots", False):
        return None
    if not _stage_passed(report_payload, "scenario-execution"):
        return None

    scenario_steps_path = Path(resolved.get("scenario_steps_json", ""))
    steps_payload = json.loads(scenario_steps_path.read_text(encoding="utf-8"))
    steps = [ScenarioStep.from_dict(item) for item in steps_payload.get("steps", [])]
    expected_count = sum(expected_step_screenshot_count(step) for step in steps)

    lines = screenshot_index_path.read_text(encoding="utf-8").splitlines()
    index_count = max(len(lines) - 1, 0)
    captured_count = report_payload["counts"]["screenshots"]

    passed = captured_count == expected_count and index_count == expected_count
    detail = (
        f"expected {expected_count}, captured {captured_count}, indexed {index_count}"
    )
    return _assertion("required-screenshot-count", "passed" if passed else "failed", detail)


def _manual_signoff_items(scenario: str) -> list[dict[str, str]]:
    if scenario not in {"release-core", "release-full"}:
        return []

    review_scope = "core-family" if scenario == "release-core" else "full-family"
    return [
        {
            "id": "config-page-review",
            "label": "Config page reviewed manually",
            "status": "pending",
        },
        {
            "id": "hardware-install-check",
            "label": "Hardware install verified manually",
            "status": "pending",
        },
        {
            "id": "visual-review",
            "label": f"{review_scope} screenshot review completed",
            "status": "pending",
        },
    ]


def _operator_actions(
    stage_items: list[dict[str, str]],
    assertions: list[dict[str, str]],
    step_rows: dict[str, dict[str, Any]],
) -> list[dict[str, str]]:
    actions: list[dict[str, str]] = []

    for stage in stage_items:
        if stage["status"] != "passed":
            actions.append(
                {
                    "label": f"inspect failed stage {stage['name']}",
                    "detail": f"stage `{stage['name']}` recorded `{stage['status']}`",
                }
            )
            return actions

    for assertion in assertions:
        if assertion["status"] != "passed":
            actions.append(
                {
                    "label": f"inspect failed assertion {assertion['name']}",
                    "detail": assertion.get("detail", ""),
                }
            )
            return actions

    for step in step_rows.values():
        screenshot = step.get("screenshot", {})
        if screenshot.get("expected", 0) != screenshot.get("captured", 0):
            actions.append(
                {
                    "label": f"inspect missing screenshot {step['artifact_identity']}",
                    "detail": (
                        f"expected `{screenshot.get('expected', 0)}` captured `{screenshot.get('captured', 0)}`"
                    ),
                }
            )
            return actions

    return actions


def _report_markdown(report_payload: dict[str, Any]) -> str:
    run = report_payload["run"]
    stages = report_payload["stages"]
    step_rows = report_payload["steps"]
    assertions = report_payload["assertions"]
    artifacts = report_payload["artifacts"]

    lines = [
        "# Validation Run Report",
        "",
        "| Detail | Output |",
        "| :--- | :--- |",
        f"| Run ID | `{run['run_id']}` |",
        f"| Status | {run['status']} |",
        f"| Scenario | `{run['scenario']}` |",
        f"| Started | `{run['started_at']}` |",
        f"| Ended | `{run['ended_at']}` |",
        f"| Exit Status | `{run['exit_status']}` |",
        "",
        "## Summary",
        "",
        "| Count | Value |",
        "| :--- | :--- |",
        f"| Stages | `{report_payload['summary']['stages']}` |",
        f"| Steps | `{report_payload['summary']['steps']}` |",
        f"| Assertions | `{report_payload['summary']['assertions']}` |",
        f"| Screenshots | `{report_payload['summary']['screenshots']}` |",
        "",
        "## Failed Items",
    ]

    failed_stages = [stage for stage in stages if stage["status"] != "passed"]
    failed_assertions = [assertion for assertion in assertions if assertion["status"] != "passed"]
    failed_steps = [step for step in step_rows.values() if step["status"] != "passed"]

    if not failed_stages and not failed_assertions and not failed_steps:
        lines.extend(["none", ""])
    else:
        for stage in failed_stages:
            lines.append(f"- Stage `{stage['name']}` `{stage['status']}`")
        for assertion in failed_assertions:
            lines.append(f"- Assertion `{assertion['name']}` `{assertion['status']}`")
            if assertion.get("detail"):
                lines.append(f"  - {assertion['detail']}")
        for step in failed_steps:
            lines.append(f"- Step `{step['artifact_identity']}` `{step['status']}`")
        lines.append("")

    lines.extend(
        [
            "## Step Evidence",
            "",
            "| Artifact Identity | Capability | Status | Emulator | Display | Inputs | Screenshot | Logs |",
            "| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |",
        ]
    )

    for artifact_identity in sorted(step_rows):
        step = step_rows[artifact_identity]
        screenshot = step["screenshot"]
        screenshot_value = (
            f"expected `{screenshot['expected']}` captured `{screenshot['captured']}`"
            if screenshot
            else "none"
        )
        logs_value = _report_logs_cell(step.get("logs", []))
        lines.append(
            "| "
            + " | ".join(
                [
                    f"`{step['artifact_identity']}`",
                    f"`{step['capability']}`",
                    step["status"],
                    f"`{step['emulator']}`" if step["emulator"] else "none",
                    f"`{step['display']}`" if step["display"] else "none",
                    _inputs_summary(step["inputs"]),
                    screenshot_value,
                    logs_value,
                ]
            )
            + " |"
        )

    lines.extend(
        [
            "",
            "## Artifacts",
            "",
            "| Artifact | Path |",
            "| :--- | :--- |",
            f"| Commands Log | {_file_link(artifacts['commands_log'])} |",
            f"| Run JSON | {_file_link(artifacts['run_json'])} |",
            f"| Report JSON | {_file_link(artifacts['report_json'])} |",
            f"| Screenshot Index | {_file_link(artifacts['screenshot_index'])} |",
            f"| Logs | {_logs_summary(artifacts['logs_dir'])} |",
            f"| Screenshots | {_screenshots_summary(_load_screenshot_rows(artifacts['screenshot_index']))} |",
            "",
        ]
    )
    return "\n".join(lines)


def _inputs_summary(inputs: dict[str, str]) -> str:
    if not inputs:
        return "none"
    return "<br>".join(f"`{key}`=`{value}`" for key, value in inputs.items())


def _report_logs_cell(logs: list[str]) -> str:
    if not logs:
        return "none"
    return "<br>".join(_file_link(path) for path in logs)


def _assertion_summary(assertions: list[dict[str, str]]) -> str:
    if not assertions:
        return "none"
    return "<br>".join(f"`{item['status']}` `{item['name']}`" for item in assertions)


def _stage_summary(stages: list[dict[str, str]]) -> str:
    if not stages:
        return "none"
    return "<br>".join(f"`{item['status']}` `{item['name']}`" for item in stages)


def _logs_summary(logs_dir: str) -> str:
    logs_root = Path(logs_dir)
    if not logs_dir or not logs_root.exists():
        return "none"
    log_paths = sorted(Path(logs_dir).glob("*.log"))
    if not log_paths:
        return _file_link(str(logs_root), "logs/")
    links = "<br>".join(f"&nbsp;&nbsp;- {_file_link(str(path))}" for path in log_paths)
    return f"{_file_link(str(logs_root), 'logs/')}<br>{links}"


def _load_screenshot_rows(index_path: str) -> list[dict[str, str]]:
    path = Path(index_path)
    if not path.is_file():
        return []

    rows: list[dict[str, str]] = []
    for line in path.read_text(encoding="utf-8").splitlines()[1:]:
        if not line.strip():
            continue
        fields = line.split("\t")
        if len(fields) != 6:
            continue
        scenario, capability, artifact_identity, emulator, state_tags, relative_path = fields
        absolute_path = path.parent.parent / relative_path
        rows.append(
            {
                "identity": artifact_identity,
                "detail": _screenshot_label(state_tags),
                "capability": capability,
                "absolute_path": str(absolute_path),
                "relative_path": relative_path,
                "exists": "true" if absolute_path.is_file() else "false",
            }
        )
    return rows


def _screenshots_summary(rows: list[dict[str, str]]) -> str:
    if not rows:
        return "none"

    rendered: list[str] = []
    for row in rows:
        if row["exists"] != "true":
            rendered.append(
                f"missing asset: `{row['relative_path']}`<br>{row['identity']}<br>{row['detail']}"
            )
            continue
        rendered.append(
            f"<img src=\"{row['absolute_path']}\" alt=\"{row['identity']}\" width=\"200\">"
            f"<br>{row['identity']}<br>{row['detail']}<br>"
        )
    return "<br><br>".join(rendered)


def _file_link(path_str: str, label: str | None = None) -> str:
    if not path_str:
        return "none"
    path = Path(path_str)
    return f"<a href=\"{path}\">{label or path.name}</a>"


def _screenshot_label(state_tags: str) -> str:
    if "," not in state_tags:
        return state_tags
    head, tail = state_tags.split(",", 1)
    return f"{head}/{tail}"
