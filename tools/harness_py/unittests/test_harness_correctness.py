from __future__ import annotations

from pathlib import Path
from typing import Final
import unittest
import json
from tempfile import TemporaryDirectory
from qaharnessruntime import (
    QARunContext,
    QAStepContext,
    REPORT_STEP_SCHEMA,
    ScreenshotsContext,
    build_step_outputs,
)
from qaplangrammar import QAPlanGrammar
from qaplanparser import parse_scenario, parse_suite
from ataglanceharness import handle_validate_plan
from qaplanresolver import AllForOneStep, LocationStep, MemberDiscard, load_and_validate_plan
from qaresultinspector import write_summary_report

QA_ROOT: Final = Path(__file__).resolve().parents[3] / "qa"
PLANS_ROOT: Final = QA_ROOT / "plans"
FIXTURES_ROOT: Final = Path(__file__).resolve().parent / "fixtures"


class HarnessCorrectnessTests(unittest.TestCase):

  def test_parser_returns_parsed_scenario_without_expanding(self) -> None:
    screenshots, emulators, displays, steps, _ = parse_scenario(PLANS_ROOT / "canary.scenario")
    self.assertEqual(screenshots, "yes")
    self.assertEqual(emulators, ["emery"])
    self.assertEqual(displays, ["white"])
    self.assertEqual(len(steps), 2)
    self.assertNotIn("emulator", steps[0].fields)

  def test_canary_is_a_real_integrated_step(self) -> None:
    screenshots, emulators, displays, steps, _ = parse_scenario(PLANS_ROOT / "canary.scenario")
    self.assertEqual(screenshots, "yes")
    self.assertEqual(emulators, ["emery"])
    self.assertEqual(displays, ["white"])
    self.assertEqual([step.capability for step in steps], ["all", "all"])

    plan = load_and_validate_plan("canary", PLANS_ROOT)
    self.assertEqual(plan.step_count, 2)
    step = next(iter(plan.steps.values()))
    self.assertIsInstance(step, AllForOneStep)
    self.assertTrue(step.capture_screenshots)
    self.assertEqual(plan.expected_screenshots, 2)

  def test_parser_returns_parsed_suite_with_resolved_scenarios(self) -> None:
    archived_plans = PLANS_ROOT / "archive"
    discovered = {}
    repeated, parsed = parse_suite(
        archived_plans / "qa-emery.suite",
        discovered,
    )
    self.assertEqual(
        [member_name for _, member_name in parsed],
        ["emery-weather", "emery-battery", "emery-health", "emery-location"],
    )
    self.assertEqual(
        list(discovered),
        [
            ("scenario", "emery-weather"),
            ("scenario", "emery-battery"),
            ("scenario", "emery-health"),
            ("scenario", "emery-location"),
        ],
    )
    self.assertEqual(repeated, [])

  def test_parser_returns_direct_suite_members_in_order(self) -> None:
    _discarded, members = parse_suite(FIXTURES_ROOT / "scenarios" / "run-them-all.suite", )
    self.assertEqual(
        members,
        [
            ("scenario", "weather"),
            ("scenario", "battery"),
            ("scenario", "health"),
            ("matrix", "slice3-matrix"),
        ],
    )

  def test_named_scenario_loads_and_expands(self) -> None:
    plan = load_and_validate_plan("visual-refresh", PLANS_ROOT)
    self.assertEqual(plan.name, "visual-refresh")
    self.assertEqual(len(plan.steps), 12)
    step = next(iter(plan.steps.values()))
    self.assertTrue(step.capture_screenshots)
    self.assertEqual(step.expected_screenshots, 1)
    self.assertEqual(step.captured_screenshots, 0)
    self.assertEqual(plan.expected_screenshots, 12)
    self.assertEqual(plan.captured_screenshots, 0)

  def test_location_scenario_loads_and_expands(self) -> None:
    location_plan = PLANS_ROOT / "archive" / "emery-location.scenario"
    plan = load_and_validate_plan(str(location_plan))
    self.assertEqual(len(plan.steps), 6)
    locations: list[str] = []
    for step in plan.steps.values():
      self.assertTrue(isinstance(step, LocationStep))
      self.assertTrue(isinstance(step.location, str))
      locations.append(step.location)
    self.assertEqual(
        locations, [
            "LÀs-_ÿegasÿ",
            "LÀs-_ÿegasÿÅÉÑØ",
            "Oakland",
            "San-Francisco",
            "Cabo San Lucas",
            "Las_Vegas",
        ]
    )
    self.assertEqual(plan.expected_screenshots, 6)

  def test_direct_scenario_path_loads(self) -> None:
    path = PLANS_ROOT / "qa-smoke.matrix"
    plan = load_and_validate_plan(str(path))
    self.assertEqual(plan.name, "qa-smoke")
    self.assertEqual(plan.path, path)

  def test_matrix_loads_preamble_and_step_files(self) -> None:
    path = FIXTURES_ROOT / "scenarios" / "slice3-matrix.matrix"
    plan = load_and_validate_plan(str(path))
    self.assertEqual(plan.name, "slice3-matrix")
    self.assertEqual(len(plan.execution_configs), 6)
    self.assertEqual(len(plan.steps), 18)
    self.assertEqual(plan.expected_screenshots, 18)

  def test_pre_release_matrix_covers_every_supported_tuple(self) -> None:
    plan = load_and_validate_plan("pre-release-gate", PLANS_ROOT)

    self.assertEqual(len(plan.execution_configs), 20)
    self.assertEqual(len(plan.steps), 258)
    self.assertEqual(plan.expected_screenshots, 258)

  def test_suite_aggregates_matrix_members(self) -> None:
    _discarded, members = parse_suite(FIXTURES_ROOT / "scenarios" / "run-them-all.suite", )
    self.assertIn(("matrix", "slice3-matrix"), members)

  def test_nested_suite_members_are_discovered_once_in_order(self) -> None:
    plan = load_and_validate_plan(
        str(FIXTURES_ROOT / "scenarios" / "duplicate-nested-members.suite")
    )
    self.assertEqual(
        plan.execution_configs,
        [("emery", "white"), ("chalk", "black")],
    )
    self.assertEqual(len(plan.steps), 2)
    self.assertEqual(
        [step.capability for step in plan.steps.values()],
        ["weather", "battery"],
    )

  def test_missing_suite_member_is_discarded_without_resolution(self) -> None:
    plan = load_and_validate_plan(str(FIXTURES_ROOT / "scenarios" / "suite-missing-member.suite"))
    self.assertEqual(plan.step_count, 1)
    self.assertEqual(len(plan.discarded), 1)
    self.assertEqual(
        plan.discarded[0],
        MemberDiscard(
            kind="scenario",
            name="missing-scenario",
            reason="Unknown included plan 'missing-scenario'",
        ),
    )

  def test_commit_smoke_loads_and_expands_across_visual_matrix(self) -> None:
    plan = load_and_validate_plan("qa-smoke", PLANS_ROOT)
    self.assertEqual(plan.name, "qa-smoke")
    self.assertEqual(len(plan.execution_configs), 6)
    self.assertEqual(len(plan.steps), len(plan.execution_configs))
    self.assertTrue(all(isinstance(step, AllForOneStep) for step in plan.steps.values()))
    self.assertTrue(all(step.capture_screenshots for step in plan.steps.values()))
    self.assertEqual(plan.expected_screenshots, len(plan.execution_configs))
    self.assertEqual(plan.captured_screenshots, 0)
    _, first_step = next(iter(plan.steps.items()))
    first_step.captured_screenshots = 1

  def test_report_step_projection_matches_report_schema(self) -> None:
    plan = load_and_validate_plan("canary", PLANS_ROOT)
    step = next(iter(plan.steps.values()))
    rows = build_step_outputs(
        plan,
        [{
            "step_result_id": step.step_id,
            "status": "passed",
            "screenshot_paths": [],
        }],
    )
    row = rows[0]
    self.assertEqual(set(row.step_args), set(REPORT_STEP_SCHEMA[step.capability]))
    self.assertNotIn("emulator", row.step_args)
    self.assertNotIn("capture_screenshots", row.step_args)
    self.assertNotIn("expected_screenshots", row.step_args)
    self.assertNotIn("captured_screenshots", row.step_args)
    self.assertNotIn("_step_id", row.step_args)

  def test_pre_push_gate_contains_visual_matrix(self) -> None:
    plan = load_and_validate_plan("pre-push-gate", PLANS_ROOT)
    self.assertEqual(plan.name, "pre-push-gate")
    self.assertEqual(len(plan.execution_configs), 6)
    self.assertEqual(plan.step_count, len(plan.execution_configs) * 2)
    self.assertTrue(all(isinstance(step, AllForOneStep) for step in plan.steps.values()))
    self.assertEqual(plan.expected_screenshots, plan.step_count)
    self.assertEqual(plan.captured_screenshots, 0)

  def test_direct_suite_path_loads(self) -> None:
    path = PLANS_ROOT / "pre-push-gate.suite"
    plan = load_and_validate_plan(str(path))
    self.assertEqual(plan.name, "pre-push-gate")
    self.assertEqual(plan.path, path)

  def test_unknown_step_field_is_rejected(self) -> None:
    path = FIXTURES_ROOT / "invalid-scenarios" / "unknown-option.scenario"
    with self.assertRaisesRegex(ValueError, "Plan has no valid STEP directives"):
      handle_validate_plan(str(path))

  def test_suite_include_cycle_is_ignored(self) -> None:
    path = FIXTURES_ROOT / "invalid-scenarios" / "include-cycle-a.suite"
    plan = load_and_validate_plan(str(path), path.parent)
    self.assertEqual(plan.step_count, 1)
    self.assertEqual(plan.execution_configs, [("emery", "white")])
    self.assertEqual(
        [item.name for item in plan.discarded if isinstance(item, MemberDiscard)],
        ["cycle-base", "include-cycle-a"],
    )

  def test_unsupported_emulator_is_rejected(self) -> None:
    path = FIXTURES_ROOT / "invalid-scenarios" / "unsupported-emulator.scenario"
    with self.assertRaisesRegex(ValueError, "Unsupported emulator"):
      handle_validate_plan(str(path))

  def test_unsupported_display_mode_is_rejected(self) -> None:
    path = FIXTURES_ROOT / "invalid-scenarios" / "unsupported-display.scenario"
    with self.assertRaisesRegex(ValueError, "No supported emulator/display configurations remain"):
      handle_validate_plan(str(path))

  def test_color_display_mode_is_rejected_on_monochrome_emulator(self) -> None:
    path = FIXTURES_ROOT / "invalid-scenarios" / "color-display-on-monochrome.scenario"
    with self.assertRaisesRegex(ValueError, "No supported emulator/display configurations remain"):
      handle_validate_plan(str(path))

  def test_validate_handler_returns_process_status(self) -> None:
    self.assertEqual(handle_validate_plan(str(PLANS_ROOT / "qa-smoke")), 0)

  def test_capability_arguments_are_validated_by_key(self) -> None:
    self.assertTrue(
        QAPlanGrammar.does_step_have_needed_attributes("battery", {
            "charging": "0",
            "level": "50"
        })
    )
    self.assertFalse(QAPlanGrammar.does_step_have_needed_attributes("battery", {"level": "50"}))
    with self.assertRaisesRegex(ValueError, "Unsupported capability"):
      QAPlanGrammar.does_step_have_needed_attributes("unknown", {"charging": "0", "level": "50"})

  def test_report_step_accepts_capability_arguments_in_any_order(self) -> None:
    step = QAStepContext.from_dict(
        {
            "step_id": "battery_emery_white_50_0",
            "capability": "battery",
            "status": "passed",
            "emulator": "emery",
            "step_args": {
                "charging": 0,
                "display": "white",
                "level": 50
            },
            "screenshot_ctx": {
                "expected": 0,
                "captured": 0,
                "paths": []
            },
        }
    )
    self.assertEqual(step.capability, "battery")
    self.assertEqual(step.screenshot_ctx.captured, 0)

  def test_summary_report_matches_report_fixture(self) -> None:
    report_root = FIXTURES_ROOT / "reports" / "canary"
    expected_summary = (report_root / "summary.md").read_text(encoding="utf-8")

    with TemporaryDirectory() as temporary_directory:
      generated_summary = Path(temporary_directory) / "GeneratedReport.md"
      write_summary_report(report_root, generated_summary)

      self.assertEqual(generated_summary.read_text(encoding="utf-8"), expected_summary)

  def test_report_rejects_screenshot_path_count_mismatch(self) -> None:
    with self.assertRaisesRegex(ValueError, "Captured screenshots mismatch"):
      ScreenshotsContext.from_dict(
          {
              "expected": 1,
              "captured": 0,
              "paths": ["screenshots/step.png"],
          }
      )

  def test_report_rejects_capture_count_greater_than_expected(self) -> None:
    with self.assertRaisesRegex(ValueError, "Captured screenshots mismatch"):
      ScreenshotsContext.from_dict(
          {
              "expected": 1,
              "captured": 2,
              "paths": ["one.png", "two.png"],
          }
      )

  def test_run_identity_matches_timestamp_and_folder(self) -> None:
    run = QARunContext.from_dict(
        {
            "plan":
            "canary",
            "started_at":
            "20260727T154241",
            "output_folder":
            "/tmp/20260727T154241-pid1234",
            "run_id":
            "20260727T154241-pid1234",
            "status":
            "passed",
            "resolved": {
                "expected_screenshots": 0,
                "captured_screenshots": 0,
            },
            "run_outputs": {
                "root": "/tmp/20260727T154241-pid1234"
            },
            "step_count":
            1,
            "step_outputs": [
                {
                    "step_id": "battery_emery_white_50_0",
                    "capability": "battery",
                    "status": "passed",
                    "emulator": "emery",
                    "step_args": {
                        "charging": 0,
                        "display": "white",
                        "level": 50
                    },
                    "screenshot_ctx": {
                        "expected": 0,
                        "captured": 0,
                        "paths": []
                    },
                }
            ],
        }
    )
    self.assertEqual(run.run_id, "20260727T154241-pid1234")

  def test_run_identity_rejects_run_id_mismatch(self) -> None:
    report = {
        "plan":
        "canary",
        "started_at":
        "20260727T154242",
        "output_folder":
        "/tmp/20260727T154241-pid1234",
        "run_id":
        "20260727T154242-pid1234",
        "status":
        "passed",
        "resolved": {
            "expected_screenshots": 0,
            "captured_screenshots": 0,
        },
        "run_outputs": {
            "root": "/tmp/20260727T154241-pid1234"
        },
        "step_count":
        1,
        "step_outputs": [
            {
                "step_id": "battery_emery_white_50_0",
                "capability": "battery",
                "status": "passed",
                "emulator": "emery",
                "step_args": {
                    "charging": 0,
                    "display": "white",
                    "level": 50
                },
                "screenshot_ctx": {
                    "expected": 0,
                    "captured": 0,
                    "paths": []
                },
            }
        ],
    }
    with self.assertRaisesRegex(ValueError, "Run-id and output_folder"):
      QARunContext.from_dict(report)

  def test_report_rejects_missing_required_resolved_context(self) -> None:
    path = FIXTURES_ROOT / "invalid-reports" / "missing-resolved.json"
    with self.assertRaisesRegex(ValueError, "resolved"):
      QARunContext.from_dict(json.loads(path.read_text(encoding="utf-8")))

  def test_report_rejects_invalid_screenshot_count(self) -> None:
    path = FIXTURES_ROOT / "invalid-reports" / "invalid-screenshot-count.json"
    with self.assertRaisesRegex(ValueError, "Captured screenshots mismatch"):
      QARunContext.from_dict(json.loads(path.read_text(encoding="utf-8")))

  def test_report_rejects_unknown_capability(self) -> None:
    path = FIXTURES_ROOT / "invalid-reports" / "unknown-capability.json"
    with self.assertRaisesRegex(ValueError, "Invalid capability"):
      QARunContext.from_dict(json.loads(path.read_text(encoding="utf-8")))


if __name__ == "__main__":
  unittest.main()
