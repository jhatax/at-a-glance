#!/usr/bin/env python3
from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path

LIST_RE = re.compile(r"^\s*(?:[-*+] |\d+[.)] )")
FENCE_RE = re.compile(r"^\s*(```|~~~)")


def tracked_markdown() -> list[Path]:
  result = subprocess.run(
      ["git", "ls-files", "--cached", "--others", "--exclude-standard", "--", "*.md"],
      check=True,
      capture_output=True,
      text=True,
  )
  return [Path(line) for line in result.stdout.splitlines() if line]


def is_structural(line: str) -> bool:
  stripped = line.strip()
  return (
      not stripped or stripped.startswith(("#", ">", "|", "```", "~~~", "<!--"))
      or LIST_RE.match(line) is not None or line.startswith(("    ", "\t"))
  )


def reflow(text: str) -> str:
  lines = text.splitlines()
  output: list[str] = []
  paragraph: list[str] = []
  in_fence = False
  last_was_list = False

  def flush() -> None:
    if paragraph:
      output.append(" ".join(part.strip() for part in paragraph))
      paragraph.clear()

  for line in lines:
    if FENCE_RE.match(line):
      flush()
      output.append(line.rstrip())
      in_fence = not in_fence
    elif in_fence:
      output.append(line.rstrip())
    elif not line.strip():
      flush()
      output.append("")
      last_was_list = False
    elif LIST_RE.match(line):
      flush()
      output.append(line.rstrip())
      last_was_list = True
    elif is_structural(line):
      flush()
      output.append(line.rstrip())
      last_was_list = False
    elif last_was_list:
      output[-1] += " " + line.strip()
    else:
      paragraph.append(line)
  flush()
  return "\n".join(output) + ("\n" if text.endswith(("\n", "\r")) else "")


def check_text(path: Path, text: str) -> list[str]:
  lines = text.splitlines()
  errors: list[str] = []
  in_fence = False
  previous_plain = False
  previous_list = False
  for number, line in enumerate(lines, 1):
    if FENCE_RE.match(line):
      in_fence = not in_fence
      previous_plain = False
      previous_list = False
      continue
    if in_fence:
      previous_plain = False
      previous_list = False
      continue
    if LIST_RE.match(line):
      previous_plain = False
      previous_list = True
      continue
    if is_structural(line):
      previous_plain = False
      previous_list = False
      continue
    if line.rstrip() != line:
      errors.append(f"{path}:{number}: trailing whitespace")
    if previous_plain or previous_list:
      errors.append(f"{path}:{number}: artificial prose line break")
    previous_plain = True
    previous_list = False
  return errors


def main() -> int:
  parser = argparse.ArgumentParser()
  parser.add_argument("--check", action="store_true", help="fail when prose needs reflow")
  args = parser.parse_args()
  failures: list[str] = []
  for path in tracked_markdown():
    original = path.read_text()
    if args.check:
      failures.extend(check_text(path, original))
    else:
      rewritten = reflow(original)
      if rewritten != original:
        path.write_text(rewritten)
        print(path)
  if failures:
    print("\n".join(failures), file=sys.stderr)
    return 1
  return 0


if __name__ == "__main__":
  raise SystemExit(main())
