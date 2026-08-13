from __future__ import annotations

from pathlib import Path
import unittest

from qaplanparser import parse_matrix_files, parse_steps


FIXTURES = Path(__file__).resolve().parent / "fixtures" / "scenarios"


class Slice3ParserTests(unittest.TestCase):

  def test_steps_parser_scans_to_execute_and_stops_at_end(self) -> None:
    steps, discarded = parse_steps(FIXTURES / "slice3-steps.steps")

    self.assertEqual([step.capability for step in steps], ["weather", "battery"])
    self.assertEqual(discarded, [])

  def test_steps_parser_ignores_lines_containing_hash(self) -> None:
    steps, discarded = parse_steps(FIXTURES / "hash-lines.steps")

    self.assertEqual([step.fields["location"] for step in steps], ["Room"])
    self.assertEqual(discarded, [])

  def test_matrix_parser_reads_step_files(self) -> None:
    screenshots, emulators, displays, step_files, discarded = parse_matrix_files(
        FIXTURES / "slice3-matrix.matrix"
    )

    self.assertEqual(screenshots, "yes")
    self.assertEqual(emulators, ["emery", "chalk"])
    self.assertEqual(displays, ["white", "black", "celeste"])
    self.assertEqual(step_files, ["slice3-steps.steps", "another-steps.steps"])
    self.assertEqual(discarded, [])

  def test_matrix_parser_ignores_step_files_without_steps_suffix(self) -> None:
    _screenshots, _emulators, _displays, step_files, _discarded = parse_matrix_files(
        FIXTURES / "matrix-invalid-steps-entry.matrix"
    )

    self.assertEqual(step_files, ["slice3-steps.steps"])

  def test_matrix_parser_ignores_lines_outside_its_blocks(self) -> None:
    screenshots, emulators, displays, step_files, discarded = parse_matrix_files(
        FIXTURES / "matrix-ignored-lines.matrix"
    )

    self.assertEqual((screenshots, emulators, displays), ("no", ["emery"], ["white"]))
    self.assertEqual(step_files, ["slice3-steps.steps"])
    self.assertEqual(discarded, [])

  def test_matrix_requires_steps_block(self) -> None:
    with self.assertRaisesRegex(ValueError, "Matrix is missing STEPS"):
      parse_matrix_files(
          FIXTURES.parent / "invalid-scenarios" / "matrix-missing-steps.matrix"
      )


if __name__ == "__main__":
  unittest.main()
