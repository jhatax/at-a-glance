from __future__ import annotations

from pathlib import Path
from typing import Final
import unittest
import json
from qaharnessruntime import QARunContext, QAStepContext, ScreenshotsContext
from qaplangrammar import ParsedScenario, ParsedSuite, QAPlanGrammar
from qaplanparser import parse_scenario, parse_suite
from ataglanceharness import handle_validate_plan
from qaplanresolver import load_and_validate_plan

QA_ROOT: Final = Path(__file__).resolve().parent
PLANS_ROOT: Final = QA_ROOT / "plans"


class HarnessCorrectnessTests(unittest.TestCase):

  def test_parser_returns_parsed_scenario_without_expanding(self) -> None:
    parsed = parse_scenario(PLANS_ROOT / "canary.scenario")
    self.assertIsInstance(parsed, ParsedScenario)
    self.assertEqual(parsed.name, "canary")
    self.assertEqual(parsed.screenshots_policy, "no")
    self.assertEqual(parsed.emulators, ["emery"])
    self.assertEqual(len(parsed.steps), 1)
    self.assertNotIn("emulator", parsed.steps[0].fields)

  def test_parser_returns_parsed_suite_with_resolved_scenarios(self) -> None:
    parsed = parse_suite(PLANS_ROOT / "dev-smoke.suite")
    self.assertIsInstance(parsed, ParsedSuite)
    self.assertEqual(parsed.name, "dev-smoke")
    self.assertEqual(
        [scenario.name for scenario in parsed.members], [
            "emery-weather",
            "emery-battery",
            "emery-health",
        ]
    )

  def test_named_scenario_loads_and_expands(self) -> None:
    plan = load_and_validate_plan("canary", PLANS_ROOT)
    self.assertEqual(plan.name, "canary")
    self.assertEqual(len(plan.steps), 1)
    _, step = next(iter(plan.steps.items()))
    self.assertFalse(step.capture_screenshots)
    self.assertEqual(step.expected_screenshots, 0)
    self.assertEqual(step.captured_screenshots, 0)
    self.assertEqual(plan.expected_screenshots, 0)
    self.assertEqual(plan.captured_screenshots, 0)

  def test_location_scenario_loads_and_expands(self) -> None:
    location_plan = (
        Path(__file__).resolve().parents[3] / "qa" / "plans" / "emery-location.scenario"
    )
    plan = load_and_validate_plan(str(location_plan))
    self.assertEqual(len(plan.steps), 4)
    self.assertEqual([step.capability for step in plan.steps.values()], ["location"] * 4)
    self.assertEqual(
        [step.location for step in plan.steps.values()],
        ["Oakland", "San-Francisco", "Cabo San Lucas", "Las_Vegas"]
    )
    self.assertEqual(plan.expected_screenshots, 4)

  def test_direct_scenario_path_loads(self) -> None:
    path = PLANS_ROOT / "canary.scenario"
    plan = load_and_validate_plan(str(path))
    self.assertEqual(plan.name, "canary")
    self.assertEqual(plan.path, path)

  def test_named_suite_loads_and_expands(self) -> None:
    plan = load_and_validate_plan("dev-smoke", PLANS_ROOT)
    self.assertEqual(plan.name, "dev-smoke")
    self.assertEqual(len(plan.steps), 8)
    self.assertEqual(
        [step.capability for step in plan.steps.values()],
        ["weather", "battery", "battery", "battery", "battery", "battery", "battery", "health"],
    )
    self.assertTrue(all(step.capture_screenshots for step in plan.steps.values()))
    self.assertEqual(plan.expected_screenshots, 8)
    self.assertEqual(plan.captured_screenshots, 0)
    _, first_step = next(iter(plan.steps.items()))
    first_step.captured_screenshots = 1

  def test_direct_suite_path_loads(self) -> None:
    path = PLANS_ROOT / "dev-smoke.suite"
    plan = load_and_validate_plan(str(path))
    self.assertEqual(plan.name, "dev-smoke")
    self.assertEqual(plan.path, path)

  def test_unknown_step_field_is_rejected(self) -> None:
    path = QA_ROOT / "fixtures" / "invalid-scenarios" / "unknown-option.scenario"
    with self.assertRaisesRegex(ValueError, "Unsupported STEP weather field"):
      handle_validate_plan(str(path))

  def test_suite_include_cycle_is_rejected(self) -> None:
    path = QA_ROOT / "fixtures" / "invalid-scenarios" / "include-cycle-a.suite"
    with self.assertRaisesRegex(ValueError, "INCLUDE cycle"):
      load_and_validate_plan(str(path), path.parent)

  def test_unsupported_emulator_is_rejected(self) -> None:
    path = QA_ROOT / "fixtures" / "invalid-scenarios" / "unsupported-emulator.scenario"
    with self.assertRaisesRegex(ValueError, "Unsupported emulator"):
      handle_validate_plan(str(path))

  def test_unsupported_display_mode_is_rejected(self) -> None:
    path = QA_ROOT / "fixtures" / "invalid-scenarios" / "unsupported-display.scenario"
    with self.assertRaisesRegex(ValueError, "Unsupported display-mode"):
      handle_validate_plan(str(path))

  def test_color_display_mode_is_rejected_on_monochrome_emulator(self) -> None:
    path = QA_ROOT / "fixtures" / "invalid-scenarios" / "color-display-on-monochrome.scenario"
    with self.assertRaisesRegex(ValueError, "unsupported on emulator 'flint'"):
      handle_validate_plan(str(path))

  def test_validate_handler_returns_process_status(self) -> None:
    self.assertEqual(handle_validate_plan(str(PLANS_ROOT / "canary")), 0)

  def test_capability_arguments_are_validated_by_key(self) -> None:
    self.assertTrue(
        QAPlanGrammar.does_step_have_needed_attributes(
            "battery", {
                "charging": "0",
                "level": "50",
                "display": "white"
            }
        )
    )
    self.assertFalse(
        QAPlanGrammar.does_step_have_needed_attributes("battery", {
            "charging": "0",
            "level": "50"
        })
    )
    with self.assertRaisesRegex(ValueError, "Unsupported capability"):
      QAPlanGrammar.does_step_have_needed_attributes(
          "unknown", {
              "charging": "0",
              "level": "50",
              "display": "white"
          }
      )

  def test_report_step_accepts_capability_arguments_in_any_order(self) -> None:
    step = QAStepContext.from_dict(
        {
            "step_id": "battery_emery_50_0",
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
            "_run_id":
            "20260727T154241-pid1234",
            "status":
            "passed",
            "resolved": {
                "expected_screenshots": 0,
                "captured_screenshots": 0,
                "emulators": ["emery"],
            },
            "run_outputs": {
                "root": "/tmp/20260727T154241-pid1234"
            },
            "step_count":
            1,
            "step_outputs": [
                {
                    "step_id": "battery_emery_50_0",
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

  def test_run_identity_rejects_timestamp_mismatch(self) -> None:
    report = {
        "plan":
        "canary",
        "started_at":
        "20260727T154242",
        "output_folder":
        "/tmp/20260727T154241-pid1234",
        "_run_id":
        "20260727T154241-pid1234",
        "status":
        "passed",
        "resolved": {
            "expected_screenshots": 0,
            "captured_screenshots": 0,
            "emulators": ["emery"],
        },
        "run_outputs": {
            "root": "/tmp/20260727T154241-pid1234"
        },
        "step_count":
        1,
        "step_outputs": [
            {
                "step_id": "battery_emery_50_0",
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
    with self.assertRaisesRegex(ValueError, "folder and run_id mismatch"):
      QARunContext.from_dict(report)

  def test_report_rejects_missing_required_resolved_context(self) -> None:
    path = QA_ROOT / "fixtures" / "invalid-reports" / "missing-resolved.json"
    with self.assertRaisesRegex(ValueError, "resolved"):
      QARunContext.from_dict(json.loads(path.read_text(encoding="utf-8")))

  def test_report_rejects_invalid_screenshot_count(self) -> None:
    path = QA_ROOT / "fixtures" / "invalid-reports" / "invalid-screenshot-count.json"
    with self.assertRaisesRegex(ValueError, "Captured screenshots mismatch"):
      QARunContext.from_dict(json.loads(path.read_text(encoding="utf-8")))

  def test_report_rejects_unknown_capability(self) -> None:
    path = QA_ROOT / "fixtures" / "invalid-reports" / "unknown-capability.json"
    with self.assertRaisesRegex(ValueError, "Invalid capability"):
      QARunContext.from_dict(json.loads(path.read_text(encoding="utf-8")))


if __name__ == "__main__":
  unittest.main()
