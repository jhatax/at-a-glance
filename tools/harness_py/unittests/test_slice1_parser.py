from __future__ import annotations

from pathlib import Path
import unittest

from qaplanparser import parse_scenario


FIXTURE = (
    Path(__file__).resolve().parent
    / "fixtures"
    / "scenarios"
    / "slice1-preamble-displays.scenario"
)


class Slice1ParserTests(unittest.TestCase):

  def test_supported_emulator_set_is_explicit(self) -> None:
    from qaplangrammar import QAPlanGrammar

    self.assertEqual(
        QAPlanGrammar.SUPPORTED_EMULATORS,
        {"aplite", "basalt", "chalk", "diorite", "emery", "flint", "gabbro"},
    )

  def test_displays_are_parsed_from_preamble_and_step_display_is_not_allowed(self) -> None:
    screenshots, emulators, displays, steps, discarded = parse_scenario(FIXTURE)

    self.assertEqual(screenshots, "no")
    self.assertEqual(emulators, ["emery", "flint"])
    self.assertEqual(displays, ["white", "celeste"])
    self.assertEqual(
        [step.capability for step in steps],
        ["weather", "battery", "battery", "weather"],
    )
    self.assertEqual(steps[0].fields, {"temp": "265", "code": "65", "is_day": "1"})
    self.assertEqual(len(discarded), 4)
    self.assertTrue(any("Unknown display 'unknown'" in item.reason for item in discarded))
    self.assertTrue(any("Duplicate display 'white'" in item.reason for item in discarded))
    self.assertTrue(any("missing field 'is_day'" in item.reason for item in discarded))
    self.assertTrue(any("Unsupported STEP weather field 'display'" in item.reason
                        for item in discarded))


if __name__ == "__main__":
  unittest.main()
