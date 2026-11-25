#!/usr/bin/env python3
"""Check native selection, matrices, diagnostics, and isolation in a disposable copy."""
import json
from pathlib import Path
import re
import shutil
import subprocess
import tempfile


def run(root, *arguments, succeeds=True):
  result = subprocess.run(
      ["sh", str(root / "tests/native/run.sh"), *arguments],
      cwd=root,
      text=True,
      stdout=subprocess.PIPE,
      stderr=subprocess.STDOUT,
      timeout=90,
  )
  if (result.returncode == 0) != succeeds:
    raise AssertionError(result.stdout)
  return result.stdout


def main():
  native = Path(__file__).resolve().parent
  with tempfile.TemporaryDirectory(prefix="aag native runner ") as temporary:
    root = Path(temporary)
    (root / "src").symlink_to(native.parents[1] / "src", target_is_directory=True)
    target = root / "tests/native"
    shutil.copytree(
        native,
        target,
        ignore=shutil.ignore_patterns("__pycache__", "compile_commands.json", "*.swp"),
    )
    focus = ("--platform", "flint", "--health", "no-health")

    listed = run(root, "--list")
    assert "settings::legacy_migration" in listed and "comms::open_retries" in listed
    assert "::initialize" not in listed and "::cleanup" not in listed
    assert "No matching suite" in run(root, "--suite", "missing", "--list", succeeds=False)
    assert "No matching cases" in run(
        root, "--suite", "settings", "--case", "missing", "--list", succeeds=False
    )
    assert "requires --suite" in run(root, "--case", "missing", succeeds=False)
    for option in ("--matrix", "--platform", "--health"):
      assert "Unknown" in run(root, option, "missing", succeeds=False)
      assert "requires a nonempty value" in run(root, option, succeeds=False)
    print("PASS discovery and invalid selections")

    # The default gate builds four binaries, each shared by every selected suite.
    output = run(root, "--keep-build")
    case_count = len(listed.splitlines())
    assert f"4 builds, {4 * case_count} case executions, 0 failures" in output, output
    builds = root / "build/native-tests"
    expected = {"emery-health", "chalk-health", "aplite-no-health", "flint-health"}
    assert {path.parent.name for path in builds.glob("*/native_tests")} == expected
    assert "settings::defaults: ok" in output and "requires PBL_HEALTH" in output
    assert "comms::open_retries: ok\ncomms::reinitialize_cancels_timer: ok" in output
    assert "Native C: running settings / emery / health" in output
    assert "Loaded " not in output and "Started (test status codes:" not in output
    expected_rows = []
    for platform, health in (("emery", "health"), ("chalk", "health"), ("aplite", "no-health"),
                             ("flint", "health")):
      for identifier in listed.splitlines():
        suite, case = identifier.split("::", 1)
        expected_rows.append([platform, health, suite, case])
    rows = [line.split("\t") for line in (builds / "run-list.tsv").read_text().splitlines()]
    assert rows == expected_rows, rows
    cursor = 0
    for platform, health, suite, case in rows:
      label = f"{suite.replace('_', '::')}::{case}:"
      position = output.find(label, cursor)
      assert position >= 0, (platform, health, suite, case, output[cursor:])
      cursor = position + len(label)
    entries = json.loads((target / "compile_commands.json").read_text())
    for platform in ("emery", "chalk", "aplite", "flint"):
      settings = [
          entry for entry in entries if entry["file"].endswith("suites/settings.c")
          and f'TEST_PLATFORM="{platform}"' in " ".join(entry["arguments"])
      ]
      assert len(settings) == 1, settings
      assert ("PBL_HEALTH" in " ".join(settings[0]["arguments"])) == (platform != "aplite")
    print("PASS ordered work list, four-profile default, health selection, and shared builds")

    # A source-free suite exercises discovery and the unmapped source-entry fallback.
    # A prefix collision tests exact selection.
    probe = target / "suites/new_suite.c"
    probe.write_text(
        '''#include "test_runner.h"
void test_new_suite__smoke(void) { cl_assert_equal_i(2, 1 + 1); }
void test_new_suite__smoke_extra(void) { cl_fail("selected a prefix instead of an exact case"); }
void test_new_suite__failure(void) { cl_fail("underscore replay probe"); }
'''
    )
    # Clar cannot distinguish these namespace prefixes in a shared executable.
    conflict = target / "suites/new.c"
    conflict.write_text('#include "test_runner.h"\nvoid test_new__smoke(void) { cl_assert(1); }\n')
    assert "Conflicting suite names" in run(root, "--list", succeeds=False)
    conflict.unlink()
    selected = run(root, "--suite", "new_suite", "--case", "smoke", *focus, "--keep-build")
    assert "no production source mapping for new_suite; skipping source entry" in selected
    assert "new::suite::smoke: ok" in selected and "smoke_extra" not in selected, selected
    assert "1 builds, 1 case executions, 0 failures" in selected
    assert (builds / "flint-no-health/native_tests").exists()
    assert (builds / "flint-no-health/build.log").exists()
    assert (builds /
            "flint-no-health/new_suite.smoke.log").read_text().startswith("Loaded 1 suites:")
    assert (builds / "run-list.tsv").read_text() == "flint\tno-health\tnew_suite\tsmoke\n"
    full = run(root, "--matrix", "full", "--suite", "new_suite", "--case", "smoke")
    assert "14 builds, 14 case executions, 0 failures" in full, full
    print("PASS new suite discovery, exact selection, retained builds, and full matrix")

    # Failed compilation must not fall back to the binary retained just above.
    original_probe = probe.read_text()
    probe.write_text('#include "missing_native_probe_header.h"\n' + original_probe)
    failed = run(
        root, "--suite", "new_suite", "--case", "smoke", *focus, "--keep-build", succeeds=False
    )
    assert "not run (build failed)" in failed and "new::suite::smoke: ok" not in failed
    assert not (builds / "flint-no-health/native_tests").exists()
    assert (builds / "run-list.tsv").read_text() == "flint\tno-health\tnew_suite\tsmoke\n"
    probe.write_text(original_probe)
    print("PASS failed builds retain planned cases without executing stale binaries")

    failed = run(
        root, "--suite", "new_suite", "--case", "failure", *focus, "--sanitize", succeeds=False
    )
    assert "underscore replay probe" in failed
    assert "--suite new_suite --case failure --platform flint --health no-health --sanitize" in failed
    print("PASS underscore suite names and sanitizer replay commands")
    probe.unlink()

    # Append dedicated probes; never rewrite existing test or production statements.
    comms = target / "suites/comms.c"
    original = comms.read_text()
    comms.write_text(
        '#include <stdlib.h>\n' + original + '''
static void runner_check_cleanup(void) {
  if (test_active_timers() || retry_timer) _Exit(87);
  puts("cleanup verified after failure");
}
void test_comms__runner_cleanup_probe(void) {
  cl_assert_equal_i(0, atexit(runner_check_cleanup));
  test_pebble_reset();
  test_pebble.open_result = APP_MSG_BUSY;
  initialize_inbox_outbox(&retry_timer);
  test_context("deliberate failure with a pending timer");
  cl_fail("runner cleanup probe");
}
void test_comms__runner_after_cleanup_probe(void) {
  cl_assert_equal_i(0, test_active_timers());
  cl_assert_equal_p(NULL, retry_timer);
}
'''
    )
    failed = run(root, *focus, succeeds=False)
    assert "runner cleanup probe" in failed and "deliberate failure with a pending timer" in failed
    assert "timer register: delay=2000 ms" in failed
    assert "cleanup verified after failure" in failed
    assert "runner_cleanup_probe: fail (process status 1)" in failed
    assert "comms::runner_after_cleanup_probe: ok" in failed, failed
    assert "helpers::ranges: ok" in failed and "settings::defaults: ok" in failed
    assert "--suite comms --case runner_cleanup_probe --platform flint --health no-health" in failed
    print("PASS cleanup in the failing process, fresh subsequent cases, and replay commands")

    comms.write_text(
        '#include <signal.h>\n' + original + '''
void test_comms__runner_signal_probe(void) { raise(SIGTERM); }
void test_comms__runner_assertion_probe(void) { cl_fail("failure after a crashed case"); }
void test_comms__runner_after_signal_probe(void) { cl_assert(1); }
void test_comms__runner_final_probe(void) { cl_assert(1); }
'''
    )
    failed = run(root, *focus, succeeds=False)
    assert "runner_signal_probe" in failed and "FAILED:" in failed
    assert "runner_signal_probe: fail (process status" in failed
    assert "failure after a crashed case" in failed
    assert "comms::runner_after_signal_probe: ok" in failed and "comms::runner_final_probe: ok" in failed
    assert failed.index("runner_signal_probe: fail (process status") < failed.index(
        "failure after a crashed case"
    ) < failed.index("comms::runner_after_signal_probe: ok")
    assert "helpers::ranges: ok" in failed and "settings::defaults: ok" in failed, failed
    comms.write_text(original)
    print("PASS ordered continuation after both a crashed case and an assertion failure")

    # Inspect the saved operator output for every screen size and health variant.
    run(root, "--suite", "layout", "--matrix", "full", "--bounds", "--keep-build")
    for platform, width, height in (("aplite", 144, 168), ("basalt", 144, 168), ("chalk", 180, 180),
                                    ("diorite", 144, 168), ("emery", 200, 228), ("flint", 144, 168),
                                    ("gabbro", 260, 260)):
      for variant in ("no-health", "health"):
        log = (builds / f"{platform}-{variant}/layout.screen_bounds.log").read_text()
        for orientation in ("horizontal", "vertical"):
          heading = f"Geometry: {platform} / {orientation} / screen={width}x{height};"
          group = log.split(heading, 1)[1].split(f"Geometry: {orientation},", 1)[0]
          rows = re.findall(
              rf"^BOUNDS {orientation} ([\w.]+): origin \((-?\d+), (-?\d+)\) "
              r"\| width (-?\d+) \| height (-?\d+) \| bottom-right \((-?\d+), (-?\d+)\)$",
              group,
              re.MULTILINE,
          )
          names = {row[0] for row in rows}
          expected_names = {
              "time", "date", "location", "climate.icon", "climate.text", "battery.track",
              "battery.fill", "battery.bolt", "bluetooth", "horizontal_rule"
          }
          if variant == "health":
            expected_names.update(
                {"steps.icon", "steps.text", "steps.progress", "bpm.icon", "bpm.text"}
            )
          assert names == expected_names and len(rows) == len(expected_names), (platform, group)
          origins = []
          for name, *coordinates in rows:
            x, y, item_width, item_height, right, bottom = map(int, coordinates)
            assert (right, bottom) == (x + item_width,
                                       y + item_height), (platform, name, coordinates)
            origins.append((x, y))
          assert origins == sorted(
              origins, key=lambda origin: (origin[1], origin[0])
          ), (platform, orientation, origins)
          assert group.index(f"Top-Left: (0, 0) | Top-Right: ({width}, 0)") < group.index("BOUNDS ")
          assert group.rindex("BOUNDS ") < group.index(
              f"Bottom-Left: (0, {height}) | Bottom-Right: ({width}, {height})"
          )
    print(
        "PASS saved geometry rows, Y/X ordering, endpoints, and screen corners across all profiles"
    )

    layout = target / "suites/layout.c"
    layout.write_text(
        layout.read_text() + '''
void test_layout__runner_rectangle_probe(void) {
  int violations = 0;
  for (int vertical = 0; vertical <= 1; ++vertical) {
    WatchfaceSurface surface;
    cl_assert(layout_watchface_prepare(PBL_DISPLAY_WIDTH, PBL_DISPLAY_HEIGHT, vertical, &surface));
    surface.time.frame.origin.x = -3;
    surface.location.frame.origin.x = PBL_DISPLAY_WIDTH - 1;
    surface.location.frame.size.w = 4;
    violations += check_surface(&surface);
  }
  cl_assert_equal_i(0, violations);
}
void test_layout__runner_circle_probe(void) {
  GRect frame = GRect(0, 0, 1, 1);
  int violations = check_frame("probe", &frame, CONTENT_FRAME, false);
  violations += check_frame("probe", &frame, CONTENT_FRAME, true);
  cl_assert_equal_i(0, violations);
}
'''
    )
    failed = run(
        root, "--suite", "layout", "--case", "runner_rectangle_probe", *focus, succeeds=False
    )
    for orientation in ("horizontal", "vertical"):
      for element in ("time", "location"):
        assert f"BOUNDARY VIOLATION {orientation} {element}" in failed, failed
    assert "overflow left=3" in failed and "right=3" in failed
    failed = run(
        root,
        "--suite",
        "layout",
        "--case",
        "runner_circle_probe",
        "--platform",
        "chalk",
        succeeds=False
    )
    assert "CIRCLE VIOLATION horizontal probe" in failed and "CIRCLE VIOLATION vertical probe" in failed
    print("PASS all rectangle violations collected and physical-circle violations reported")


if __name__ == "__main__":
  main()
