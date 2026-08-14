from __future__ import annotations

from pathlib import Path
from contextlib import redirect_stdout
from io import StringIO
import unittest
from unittest.mock import patch

from qaplanexecutor import resolve_and_execute_plan
from qaplangrammar import ParseDiscard
from qaplanresolver import (
    MemberDiscard,
    PlanDefinition,
    ResolutionDiscard,
    load_and_validate_plan,
    print_plan,
)

FIXTURES = Path(__file__).resolve().parent / "fixtures" / "scenarios"


class Slice2ResolverTests(unittest.TestCase):

  def test_expands_steps_for_supported_emulator_display_configs(self) -> None:
    plan = load_and_validate_plan(str(FIXTURES / "slice1-preamble-displays.scenario"))

    self.assertEqual(
        plan.execution_configs,
        [("emery", "white"), ("emery", "celeste"), ("flint", "white")],
    )
    self.assertFalse(hasattr(plan, "emulators"))
    self.assertFalse(hasattr(plan, "displays"))
    self.assertEqual(
        [(step.emulator, step.display) for step in plan.steps.values()],
        [
            ("emery", "white"),
            ("emery", "celeste"),
            ("flint", "white"),
            ("emery", "white"),
            ("emery", "celeste"),
            ("flint", "white"),
        ],
    )
    self.assertEqual(len(plan.discarded), 11)

  def test_discards_unsupported_display_and_capability_combinations(self) -> None:
    plan = load_and_validate_plan(str(FIXTURES / "slice2-capability-filter.scenario"))

    self.assertEqual(
        plan.execution_configs,
        [("aplite", "white"), ("flint", "white")],
    )
    self.assertEqual(len(plan.steps), 1)
    self.assertEqual(len(plan.discarded), 3)
    self.assertTrue(
        any("unsupported on emulator 'aplite'" in item.reason for item in plan.discarded)
    )
    self.assertTrue(
        any("doesn't support capability 'health'" in item.reason for item in plan.discarded)
    )

  def test_fails_when_filtering_leaves_no_executable_steps(self) -> None:
    with self.assertRaisesRegex(ValueError, "No supported emulator/display configurations remain"):
      load_and_validate_plan(str(FIXTURES / "slice2-no-executable-steps.scenario"))

  def test_plan_output_lists_executable_steps_before_discards(self) -> None:
    output = StringIO()
    with redirect_stdout(output):
      load_and_validate_plan(str(FIXTURES / "slice2-capability-filter.scenario"))

    rendered = output.getvalue()
    self.assertLess(
        rendered.index("Resolved execution plan:"),
        rendered.index("Discarded plan items:"),
    )

  def test_discard_reporting_uses_each_type_description(self) -> None:
    plan = PlanDefinition(
        name="discarding",
        path=FIXTURES / "slice2-capability-filter.scenario",
        steps={},
        execution_configs=[],
        discarded=[
            ParseDiscard(1, "STEP weather", "invalid field"),
            ResolutionDiscard("health", ("aplite", "white"), "unsupported capability"),
            MemberDiscard("scenario", "missing", "member is unavailable"),
        ],
    )
    output = StringIO()
    with redirect_stdout(output):
      print_plan(plan)

    rendered = output.getvalue()
    self.assertLess(
        rendered.index("1. line 1: STEP weather (invalid field)"),
        rendered.index("2. Emulator 'aplite', display 'white'")
    )
    self.assertLess(
        rendered.index("2. Emulator 'aplite', display 'white'"),
        rendered.index("3. Scenario 'missing': member is unavailable")
    )

  def test_force_requires_confirmation_when_plan_has_discards(self) -> None:
    plan = PlanDefinition(
        name="discarding",
        path=FIXTURES / "slice2-capability-filter.scenario",
        steps={},
        execution_configs=[("flint", "white")],
        discarded=[ParseDiscard(1, "STEP health", "unsupported capability")],
    )
    with patch("qaplanresolver.load_and_validate_plan", return_value=plan), \
         patch("qaplanexecutor.run_plan_execution", return_value=0) as execute, \
         patch("builtins.input", return_value="yes") as operator_input:
      result = resolve_and_execute_plan("force-scenario", "discarding")

    self.assertEqual(result, 0)
    operator_input.assert_called_once_with()
    execute.assert_called_once_with(plan)

  def test_force_skips_confirmation_for_discard_free_plan(self) -> None:
    plan = PlanDefinition(
        name="clean",
        path=FIXTURES / "slice2-capability-filter.scenario",
        steps={},
        execution_configs=[("flint", "white")],
    )

    with patch("qaplanresolver.load_and_validate_plan", return_value=plan), \
         patch("qaplanexecutor.run_plan_execution", return_value=0) as execute, \
         patch("builtins.input") as operator_input:
      result = resolve_and_execute_plan("force-scenario", "clean")

    self.assertEqual(result, 0)
    operator_input.assert_not_called()
    execute.assert_called_once_with(plan)


if __name__ == "__main__":
  unittest.main()
