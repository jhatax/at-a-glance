from __future__ import annotations

import sys
import unittest
from pathlib import Path
from runner import validate_plan # noqa: E402

QA_ROOT = Path(__file__).resolve().parents[2] / "qa"
if QA_ROOT not in sys.path:
  sys.path.insert(0, str(QA_ROOT))


class HarnessCorrectnessTests(unittest.TestCase):

  def test_named_scenario_loads_and_expands(self) -> None:
    plan = validate_plan("canary")
    self.assertEqual(plan.name, "canary")
    self.assertEqual(len(plan.steps), 1)

  def test_direct_scenario_path_loads(self) -> None:
    path = QA_ROOT / "plans" / "canary.scenario"
    plan = validate_plan(str(path))
    self.assertEqual(plan.name, "canary")
    self.assertEqual(plan.path, path)

  def test_named_suite_loads_and_expands(self) -> None:
    plan = validate_plan("dev-smoke")
    self.assertEqual(plan.name, "dev-smoke")
    self.assertEqual(len(plan.steps), 3)
    self.assertEqual(
        [step.capability for step in plan.steps.values()],
        ["weather", "battery", "health"],
    )

  def test_direct_suite_path_loads(self) -> None:
    path = QA_ROOT / "plans" / "dev-smoke.suite"
    plan = validate_plan(str(path))
    self.assertEqual(plan.name, "dev-smoke")
    self.assertEqual(plan.path, path)

  def test_unknown_step_field_is_rejected(self) -> None:
    path = QA_ROOT / "fixtures" / "invalid-scenarios" / "unknown-option.scenario"
    with self.assertRaisesRegex(ValueError, "Unsupported STEP weather field"):
      validate_plan(str(path))

  def test_suite_include_cycle_is_rejected(self) -> None:
    path = QA_ROOT / "fixtures" / "invalid-scenarios" / "include-cycle-a.suite"
    with self.assertRaisesRegex(ValueError, "INCLUDE cycle"):
      validate_plan(str(path))

  def test_unsupported_emulator_is_rejected(self) -> None:
    path = QA_ROOT / "fixtures" / "invalid-scenarios" / "unsupported-emulator.scenario"
    with self.assertRaisesRegex(ValueError, "Unsupported emulator"):
      validate_plan(str(path))

  def test_unsupported_display_mode_is_rejected(self) -> None:
    path = QA_ROOT / "fixtures" / "invalid-scenarios" / "unsupported-display.scenario"
    with self.assertRaisesRegex(ValueError, "Unsupported display-mode"):
      validate_plan(str(path))

  def test_color_display_mode_is_rejected_on_monochrome_emulator(self) -> None:
    path = QA_ROOT / "fixtures" / "invalid-scenarios" / "color-display-on-monochrome.scenario"
    with self.assertRaisesRegex(ValueError, "unsupported on emulator 'flint'"):
      validate_plan(str(path))


if __name__ == "__main__":
  unittest.main()
