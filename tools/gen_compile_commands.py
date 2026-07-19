#!/usr/bin/env python3
"""Generate compile_commands.json from a verbose Pebble build log.

The Pebble build emits `runner [...]` lines that contain the raw compiler
arguments passed to `arm-none-eabi-gcc`. This script extracts those command
lines, filters them to one platform, resolves the compiler to an absolute path
when possible, and writes a clangd-friendly compile database.
"""

from __future__ import annotations

import argparse
import ast
import json
import logging
import re
import shutil
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, Sequence, TypeGuard

LOGGER = logging.getLogger("gen_compile_commands")
ANSI_ESCAPE_RE = re.compile(r"\x1b\[[0-9;]*m")
SDK_ROOT_RE = re.compile(r"Found Pebble SDK for (\w+) in:\s*:\s*(.+?)\s*$")
RUNNER_TOKEN = "runner "
DEFAULT_COMPILER = "arm-none-eabi-gcc"
DEFAULT_PLATFORM = "emery"
DEFAULT_LOG_PATH = Path("build.log")
DEFAULT_OUTPUT_PATH = Path("compile_commands.json")

EXIT_SUCCESS = 0
EXIT_USAGE = 2
EXIT_INPUT_ERROR = 3
EXIT_PARSE_ERROR = 4
EXIT_OUTPUT_ERROR = 5


@dataclass(frozen=True)
class CompileCommand:
    """One compile_commands.json entry."""

    directory: str
    file: str
    arguments: list[str]

    def to_json(self) -> dict[str, object]:
        return {
            "directory": self.directory,
            "file": self.file,
            "arguments": self.arguments,
        }


def parse_args(argv: Sequence[str]) -> argparse.Namespace:
    """Parse CLI arguments."""

    parser = argparse.ArgumentParser(
        description=("Generate compile_commands.json from `pebble build -v` output.")
    )
    parser.add_argument(
        "--log-path",
        type=Path,
        default=DEFAULT_LOG_PATH,
        help="Verbose Pebble build log to parse. Default: %(default)s",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=DEFAULT_OUTPUT_PATH,
        help="Path to write compile_commands.json. Default: %(default)s",
    )
    parser.add_argument(
        "--platform",
        default=DEFAULT_PLATFORM,
        help=("Pebble platform to extract, for example `emery` or `gabbro`. Default: %(default)s"),
    )
    parser.add_argument(
        "--compiler",
        default=DEFAULT_COMPILER,
        help=(
            "Compiler executable name or absolute path. The script resolves "
            "this to an absolute path when possible. Default: %(default)s"
        ),
    )
    parser.add_argument(
        "--verbose",
        action="store_true",
        help="Enable debug logging.",
    )
    return parser.parse_args(argv)


def configure_logging(verbose: bool) -> None:
    """Configure process logging."""

    logging.basicConfig(
        level=logging.DEBUG if verbose else logging.INFO,
        format="%(levelname)s: %(message)s",
    )


def strip_ansi(text: str) -> str:
    """Remove ANSI color codes from one log line."""

    return ANSI_ESCAPE_RE.sub("", text)


def validate_inputs(log_path: Path, output_path: Path) -> None:
    """Validate file-system inputs before parsing."""

    if not log_path.exists():
        raise FileNotFoundError(f"Build log not found: {log_path}")
    if not log_path.is_file():
        raise ValueError(f"Build log is not a file: {log_path}")

    output_parent = output_path.parent
    if output_parent and not output_parent.exists():
        raise FileNotFoundError(f"Output directory does not exist: {output_parent}")
    if output_parent and not output_parent.is_dir():
        raise ValueError(f"Output parent is not a directory: {output_parent}")


def platform_define(platform: str) -> str:
    """Convert a Pebble platform name into its compiler define."""

    normalized = platform.strip().upper()
    if not normalized:
        raise ValueError("Platform must not be empty")
    return f"-DPBL_PLATFORM_{normalized}"


def find_sdk_root(log_lines: Sequence[str], platform: str) -> Path | None:
    """Find the active Pebble SDK root for one platform from the build log."""

    target = platform.strip().lower()
    for raw_line in log_lines:
        line = strip_ansi(raw_line)
        match = SDK_ROOT_RE.search(line)
        if not match:
            continue
        if match.group(1).lower() != target:
            continue

        sdk_leaf = Path(match.group(2).strip())
        try:
            return sdk_leaf.resolve().parents[2]
        except IndexError:
            LOGGER.debug("SDK path too short to derive SDK root: %s", sdk_leaf)
            return None
    return None


def resolve_compiler_path(compiler: str, sdk_root: Path | None) -> str:
    """Return an absolute compiler path when available.

    If the compiler cannot be resolved on the current machine, keep the original
    token so the generated database remains usable for review and can be fixed
    by the caller later.
    """

    compiler_path = Path(compiler)
    if compiler_path.is_absolute():
        return str(compiler_path)

    resolved = shutil.which(compiler)
    if resolved:
        return resolved

    if sdk_root is not None:
        sdk_candidate = sdk_root / "toolchain" / "arm-none-eabi" / "bin" / Path(compiler).name
        if sdk_candidate.exists():
            return str(sdk_candidate.resolve())

    LOGGER.warning(
        "Could not resolve `%s` from PATH or Pebble SDK root; keeping compiler token unchanged",
        compiler,
    )
    return compiler


def _is_str_list(value: object) -> TypeGuard[list[str]]:
    return isinstance(value, list) and all(isinstance(item, str) for item in value)


def extract_runner_args(line: str) -> list[str] | None:
    """Extract the argv payload from one `runner [...]` log line."""

    if RUNNER_TOKEN not in line:
        return None

    payload = line[line.index(RUNNER_TOKEN) + len(RUNNER_TOKEN) :].strip()
    try:
        parsed: object = ast.literal_eval(payload)
        if not _is_str_list(parsed):
            LOGGER.debug("Skipping non-string runner payload: %r", parsed)
            return None

        return parsed

    except (SyntaxError, ValueError):
        LOGGER.debug("Skipping unparsable runner payload: %s", payload)
        return None


def compiler_matches(args: Sequence[str], compiler_name: str) -> bool:
    """Check whether argv[0] matches the expected compiler."""

    if not args:
        return False
    return Path(args[0]).name == Path(compiler_name).name


def extract_build_directory(output_arg: str, project_root: Path) -> Path | None:
    """Infer the Waf build directory from one output-object path."""

    candidate = Path(output_arg)
    parts = candidate.parts
    if "build" not in parts:
        return None

    build_index = parts.index("build")
    build_path = Path(*parts[: build_index + 1])
    if build_path.is_absolute():
        return build_path.resolve()
    return (project_root / build_path).resolve()


def normalize_compile_arguments(
    args: Sequence[str], compiler_path: str, project_root: Path
) -> tuple[list[str], Path]:
    """Remove output flags and return cleaned arguments plus build directory."""

    cleaned_args: list[str] = []
    build_dir: Path | None = None
    pending_output_value = False

    for arg in args:
        if pending_output_value:
            build_dir = extract_build_directory(arg, project_root) or build_dir
            pending_output_value = False
            continue

        if arg == "-o":
            pending_output_value = True
            continue

        if arg.startswith("-o") and len(arg) > 2:
            build_dir = extract_build_directory(arg[2:], project_root) or build_dir
            continue

        cleaned_args.append(arg)

    if pending_output_value:
        raise ValueError("Compiler command ended with bare `-o`")

    if not cleaned_args:
        raise ValueError("Compiler command became empty after normalization")

    cleaned_args[0] = compiler_path
    if build_dir is None:
        build_dir = (project_root / "build").resolve()
    return cleaned_args, build_dir


def resolve_source_file(build_dir: Path, args: Sequence[str]) -> Path:
    """Resolve the compile unit path from a normalized compiler command."""

    try:
        compile_index = args.index("-c")
    except ValueError as exc:
        raise ValueError("Compiler command missing `-c`") from exc

    if compile_index == 0:
        raise ValueError("Compiler command has no source file before `-c`")

    source_arg = Path(args[compile_index - 1])
    return (build_dir / source_arg).resolve()


def iter_compile_commands(
    log_lines: Iterable[str],
    *,
    project_root: Path,
    compiler_name: str,
    compiler_path: str,
    platform_flag: str,
) -> Iterable[CompileCommand]:
    """Yield normalized compile database entries from the build log."""

    for line_number, raw_line in enumerate(log_lines, start=1):
        args = extract_runner_args(strip_ansi(raw_line))
        if args is None:
            continue
        if not compiler_matches(args, compiler_name):
            continue
        if "-c" not in args:
            continue
        if platform_flag not in args:
            continue

        try:
            cleaned_args, build_dir = normalize_compile_arguments(args, compiler_path, project_root)
            source_file = resolve_source_file(build_dir, cleaned_args)
        except ValueError as exc:
            raise ValueError(f"Invalid compiler command at log line {line_number}: {exc}") from exc

        yield CompileCommand(
            directory=str(build_dir),
            file=str(source_file),
            arguments=list(cleaned_args),
        )


def write_compile_database(commands: Sequence[CompileCommand], output_path: Path) -> None:
    """Write the compile database to disk."""

    payload = [command.to_json() for command in commands]
    with output_path.open("w", encoding="utf-8") as handle:
        json.dump(payload, handle, indent=2)
        handle.write("\n")


def main(argv: Sequence[str] | None = None) -> int:
    """Program entrypoint."""

    args = parse_args(argv or sys.argv[1:])
    configure_logging(args.verbose)

    try:
        validate_inputs(args.log_path, args.output)
        platform_flag = platform_define(args.platform)
        project_root = args.log_path.resolve().parent
        with args.log_path.open("r", encoding="utf-8", errors="ignore") as log:
            log_lines = list(log)
        sdk_root = find_sdk_root(log_lines, args.platform)
        compiler_path = resolve_compiler_path(args.compiler, sdk_root)

        entries_by_file: dict[str, CompileCommand] = {}
        for command in iter_compile_commands(
            log_lines,
            project_root=project_root,
            compiler_name=args.compiler,
            compiler_path=compiler_path,
            platform_flag=platform_flag,
        ):
            entries_by_file[command.file] = command

        if not entries_by_file:
            LOGGER.error(
                "No compile commands found for platform `%s` in %s. "
                "This usually means the build log came from an incremental "
                "build with no compile steps. Run a clean verbose build and "
                "try again.",
                args.platform,
                args.log_path,
            )
            return EXIT_PARSE_ERROR

        sorted_entries = [entries_by_file[key] for key in sorted(entries_by_file.keys())]
        write_compile_database(sorted_entries, args.output)

    except argparse.ArgumentError as exc:
        LOGGER.error("%s", exc)
        return EXIT_USAGE
    except (FileNotFoundError, ValueError) as exc:
        LOGGER.error("%s", exc)
        return EXIT_INPUT_ERROR
    except OSError as exc:
        LOGGER.error("Failed to write compile database: %s", exc)
        return EXIT_OUTPUT_ERROR

    LOGGER.info(
        "Wrote %d compile commands for `%s` to %s",
        len(sorted_entries),
        args.platform,
        args.output,
    )
    return EXIT_SUCCESS


if __name__ == "__main__":
    raise SystemExit(main())
