# Settled QA Harness Decisions

This file records settled QA harness decisions; do not reopen them without new contrary evidence.

## Parser and plan grammar

- Unknown or non-grammar lines are intentionally ignored by the parser.
- Any line containing `#` is invalid and is ignored by the parser.
- The grammar vocabulary is explicit; unsupported directives are ignored rather than interpreted.
- Matrix `STEPS` entries must use the accepted `.steps` suffix; invalid entries are skipped.
- Matrix parser `split()` failures include the source path and line number.
- Parser owns grammar and block boundaries; resolver owns concrete step construction and filtering.
- Suite files contain only `MEMBERS`; other blocks and directives in a suite are ignored.
- Block parsers consume one shared normalized `(line_number, line)` iterator and return it at their stopping point.

## Resolution and identity

- `SUPPORT_MATRIX` is the support oracle for emulator, display, and capability compatibility.
- Duplicate suite members are recorded as `MemberDiscard` and are not traversed again.
- Suite expansion discovers members into an insertion-ordered identity dictionary before resolving them.
- Duplicate step identities are skipped and recorded as discards; duplicate identities are not executed unnecessarily.
- Grammar-originated results use the symmetric `AcceptedItem` and `DiscardedItem` categories.
- Step IDs are guaranteed unique; execution result IDs derive from step IDs and require no redundant runtime uniqueness validation.
- Dynamic capability construction is accepted because step creation validates all required fields.
- `asdict()` serialization with explicit field filtering is accepted over separate DTO serialization.

## Execution and Pebble integration

- The connection lifecycle is owned by `_connection()` and all connection call sites use its context manager.
- Pebble transport exception suppression is accepted because this harness validates watchface behavior, not Pebble transport reliability.
- Fixed synthetic waits are accepted because Pebble provides no completion notification for these operations.
- Screenshot capture is best effort and follows the official Pebble Tool behavior.
- WebSocket close is the authoritative connection shutdown available from the installed libpebble2 API.
- Pebble Tool environment setup may mutate `sys.path`, `PYTHONPATH`, and `PEBBLE_QEMU_PATH` as required by the toolchain.
- Pebble connection capability and regression testing belongs to the Pebble Dev team, not this watchface harness.

## Reports and operator artifacts

- `report.json` is the canonical durable report; Markdown is optional derived output and can be regenerated.
- A valid resolved run contains at least one executable step; renderers may rely on that invariant.
- Markdown output does not require full escaping for this local operator-support harness.
- Step IDs may be used as local screenshot filenames; additional sanitization is not required.
- A step may legitimately contain one or multiple screenshot paths.
- The only run identity invariant is `run_id == output_folder.name`.
- `commands.log` is the authoritative diagnostic record for step failures; console output is secondary.

## CLI and implementation shape

- Verbose builds write complete build output and the Python traceback to `build.log`.
- Build failures show only extracted compiler diagnostics in the console and QA log; the Python traceback remains in `build.log`.
- Non-verbose builds do not create `build.log`; build failures use the harness failure path without exposing a Python traceback.
- `compilerdbgenerator.py` is an internal library module; it has no CLI `main()` entrypoint.
- Compiler logs stream directly into `iter_compile_commands()` rather than being materialized as a list.
- Dependency-free inspection imports must not require Pebble Tool initialization.
- Small readability cleanups are accepted when they remove redundant syntax or dead scaffolding without changing behavior.
