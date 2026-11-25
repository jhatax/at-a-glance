#!/usr/bin/env python3
"""List a suite's Clar cases, or validate one exact case, without compiling."""
import argparse
import importlib.util
from pathlib import Path


def main():
  parser = argparse.ArgumentParser(description=__doc__)
  parser.add_argument("source", type=Path, metavar="SUITE_SOURCE", help="suite C source file")
  parser.add_argument(
      "case", nargs="?", metavar="CASE_NAME", help="exact case name; omit to list all cases"
  )
  arguments = parser.parse_args()

  try:
    source_text = arguments.source.read_text(encoding="utf-8")

    # Clar's generator is a standalone upstream script. Import its parser unchanged.
    generator_path = Path(__file__).resolve().parent / "vendor" / "clar" / "generate.py"
    module_spec = importlib.util.spec_from_file_location("clar_generate", generator_path)
    if module_spec is None or module_spec.loader is None:
      raise ImportError(f"Cannot load Clar generator: {generator_path}")
    generator = importlib.util.module_from_spec(module_spec)
    module_spec.loader.exec_module(generator)

    # Match registration generation: filename, 'clar' application, 'test' prefix.
    suite = generator.Module(name=arguments.source.stem, app_name="clar", prefix="test")
    suite.parse(source_text)
    # Clar keeps lifecycle callbacks separate from ordinary cases.
    case_names = [callback["short_name"] for callback in suite.callbacks]
  except (ImportError, OSError, UnicodeError) as error:
    parser.exit(1, f"Cannot discover cases in {arguments.source}: {error}\n")

  if arguments.case is not None:
    if arguments.case not in case_names:
      parser.exit(1, f"No matching cases in {arguments.source}: {arguments.case}\n")
    case_names = [arguments.case]
  if not case_names:
    parser.exit(1, f"No matching cases in {arguments.source}\n")

  for case_name in case_names:
    print(f"{arguments.source.stem}::{case_name}")


if __name__ == "__main__":
  main()
