from __future__ import annotations

import sys
import time
import os
import shutil
from pathlib import Path
from typing import Callable, Final
import threading
from dataclasses import dataclass

APP_MESSAGE_TIMEOUT_SECONDS: Final[float] = 2.0
PEBBLE_SETTLE_DELAY_SECONDS: Final[int] = 2
APP_READY_DELAY_SECONDS: Final[int] = 4


@dataclass(frozen=True)
class PebbleToolPaths:
  sdk_root: Path
  compiler_path: Path


def _load_pebble_tool() -> PebbleToolPaths:
  """Make the installed Pebble tool libraries available to the harness."""
  pebble = shutil.which("pebble")
  if pebble is None:
    raise ImportError("Pebble Tool is not installed")
  pebble_path = Path(pebble)
  interpreter = pebble_path.read_text(encoding="utf-8").splitlines()[0].removeprefix("#!").strip()
  interpreter_path = shutil.which(interpreter.split()[0]) or interpreter.split()[0]
  site_packages = next(
      (
          path
          for path in (Path(interpreter_path).parent.parent / "lib").glob("python*/site-packages")
          if path.is_dir()
      ),
      None,
  )
  if site_packages is None:
    raise ImportError("Pebble Tool site-packages directory is not available")
  if str(site_packages) not in sys.path:
    sys.path.insert(0, str(site_packages))
  os.environ["PYTHONPATH"] = os.pathsep.join(
      filter(None, [str(site_packages), os.environ.get("PYTHONPATH", "")])
  )
  from pebble_tool.sdk import sdk_version
  from pebble_tool.util import get_persist_dir

  active_sdk = sdk_version()
  if not active_sdk:
    raise ImportError("Pebble Tool has no active SDK")
  sdk_root = Path(get_persist_dir()) / "SDKs" / active_sdk
  qemu = sdk_root / "toolchain" / "bin" / "qemu-pebble"
  if not qemu.is_file():
    raise ImportError(f"Active Pebble SDK has no QEMU binary: {qemu}")
  os.environ.setdefault("PEBBLE_QEMU_PATH", str(qemu))
  os.environ["PATH"] = os.pathsep.join([str(qemu.parent), os.environ.get("PATH", "")])
  compiler_path = sdk_root / "toolchain" / "arm-none-eabi" / "bin" / "arm-none-eabi-gcc"
  if not compiler_path.is_file():
    raise ImportError(f"Active Pebble SDK has no compiler: {compiler_path}")
  return PebbleToolPaths(sdk_root=sdk_root, compiler_path=compiler_path)


try:
  PEBBLE_TOOL_PATHS = _load_pebble_tool()
  from libpebble2.communication import PebbleConnection # noqa: E402
  from libpebble2.communication.transports.qemu.protocol import QemuBattery # noqa: E402
  from libpebble2.services.install import AppInstaller # noqa: E402
  from libpebble2.services.appmessage import AppMessageService, CString, Int32 # noqa: E402
  from libpebble2.services.screenshot import Screenshot # noqa: E402
  from pebble_tool.commands.emucontrol import send_data_to_qemu # noqa: E402
  from pebble_tool.commands.sdk.project import SDKProjectCommand # noqa: E402
  from pebble_tool.sdk.emulator import ManagedEmulatorTransport # noqa: E402
  from pebble_tool.sdk.project import PebbleProject # noqa: E402
  from compilerdbgenerator import generate_compile_database # noqa: E402
except ImportError as exc:
  raise ImportError("libpebble2 or its Pebble Tool environment is unavailable") from exc


class PebbleAdapter:

  def __init__(self, log: Callable[[str], None]) -> None:
    self._log = log
    self._connections: dict[str, PebbleConnection] = {}

  def _log_failure(self, operation: str, exc: Exception) -> None:
    self._log(f"{operation} status=failed error={exc}")

  def _log_success(self, operation: str) -> None:
    self._log(f"{operation} status=success")

  def _connection(self, emulator: str) -> PebbleConnection:
    if emulator not in self._connections:
      connection = PebbleConnection(ManagedEmulatorTransport(emulator))
      connection.connect()
      connection.run_async()
      self._connections[emulator] = connection
    return self._connections[emulator]

  def send_app_message(self, emulator: str, values: dict[int, int | str]) -> None:
    connection = self._connection(emulator)
    service = AppMessageService(connection)
    completed = threading.Event()
    outcome: dict[str, tuple[object, ...]] = {}
    transaction_id = -1

    def handle_result(result: str, *args: object) -> None:
      if args and args[0] == transaction_id:
        outcome[result] = args
        completed.set()

    ack_handle = service.register_handler("ack", lambda *args: handle_result("ack", *args))
    nack_handle = service.register_handler("nack", lambda *args: handle_result("nack", *args))
    try:
      transaction_id = service.send_message(
          PebbleProject().uuid,
          {
              key: CString(value) if isinstance(value, str) else Int32(value)
              for key, value in values.items()
          },
      )
      if not completed.wait(APP_MESSAGE_TIMEOUT_SECONDS):
        raise TimeoutError(
            f"AppMessage timed out waiting for emulator '{emulator}' acknowledgement"
        )
      if "nack" in outcome:
        nack_args = outcome["nack"]
        raise RuntimeError(f"AppMessage rejected by emulator '{emulator}': {nack_args}")
    except Exception as exc:
      self._log_failure(f"AppMessage {emulator}: {values}", exc)
      raise
    finally:
      service.unregister_handler(ack_handle)
      service.unregister_handler(nack_handle)
      service.shutdown()
      time.sleep(PEBBLE_SETTLE_DELAY_SECONDS)
    self._log_success(f"AppMessage {emulator}: {values}")

  def install(self, emulator: str, pbw_path: Path) -> None:
    connection = self._connection(emulator)
    installer = AppInstaller(connection, str(pbw_path))
    try:
      installer.install()
      time.sleep(APP_READY_DELAY_SECONDS)
    except Exception as exc:
      self._log_failure(f"Install {emulator}: {pbw_path}", exc)
      raise
    self._log_success(f"Install {emulator}: {pbw_path}")

  def install_emulators(self, emulators: list[str], pbw_path: Path) -> None:
    for emulator in emulators:
      self.install(emulator, pbw_path)

  def build(self, verbose: bool = False, output_path: Path | None = None) -> None:
    command = SDKProjectCommand()
    command.sdk = None
    command._verbosity = 1 if verbose else 0
    build_error = True
    build_log_path = output_path or Path("build.log")
    if verbose:
      build_log_path.parent.mkdir(parents=True, exist_ok=True)
    output = build_log_path.open("w", encoding="utf-8") if verbose else None
    saved_stdout = os.dup(1) if output else None
    saved_stderr = os.dup(2) if output else None
    try:
      if output:
        if verbose:
          self._log("Will attempt to compile database for clangd integration if build succeeds...")
        sys.stdout.flush()
        sys.stderr.flush()
        os.dup2(output.fileno(), 1)
        os.dup2(output.fileno(), 2)
      command._waf("configure")
      command._waf("build")
      build_error = False
      compile_database_path = build_log_path.parent / "compile_commands.json"
      compile_database_status = None
      if verbose:
        compile_database_status = generate_compile_database(
            log_path=build_log_path,
            output_path=compile_database_path,
            platform="emery",
            compiler_path=PEBBLE_TOOL_PATHS.compiler_path,
        )
    except Exception as exc:
      self._log_failure(f"Build {'verbose ' if verbose else ''}".strip(), exc)
      raise
    finally:
      if output:
        sys.stdout.flush()
        sys.stderr.flush()
        if saved_stdout:
          os.dup2(saved_stdout, 1)
          os.close(saved_stdout)
        if saved_stderr:
          os.dup2(saved_stderr, 2)
          os.close(saved_stderr)
      if output:
        output.close()
    if verbose:
      if compile_database_status == 0:
        self._log(f"Compile database generated: {compile_database_path}")
      else:
        self._log(f"No compile database generated: {compile_database_path}")
    if build_error:
      if verbose:
        sys.stderr.write(f"Build failed. Contents of '{build_log_path}':")
        sys.stderr.write("-" * 40)
        sys.stderr.write(build_log_path.read_text(encoding="utf-8"))
        sys.stderr.write("-" * 40)
        sys.stderr.flush()
    else:
      self._log_success(f"Build {'verbose ' if verbose else ''}".strip())

  def set_battery(self, emulator: str, percent: int, charging: int) -> None:
    connection = self._connection(emulator)
    try:
      send_data_to_qemu(
          connection.transport,
          QemuBattery(percent=percent, charging=charging),
      )
    except Exception as exc:
      self._log_failure(f"Battery {emulator}: {percent}% charging={charging}", exc)
      raise
    finally:
      time.sleep(PEBBLE_SETTLE_DELAY_SECONDS)
    self._log_success(f"Battery {emulator}: {percent}% charging={int(charging)}")

  def screenshot(self, emulator: str, output_path: Path) -> None:
    try:
      if not output_path.parent.exists():
        output_path.parent.mkdir(parents=True, exist_ok=True)

      image = Screenshot(self._connection(emulator)).grab_image()
      from PIL import Image

      height = len(image)
      width = len(image[0]) // 3
      pixels = b"".join(bytes(row) for row in image)
      Image.frombytes("RGB", (width, height), pixels).save(output_path)
    except Exception as exc:
      self._log_failure(f"Screenshot {emulator}:\n{output_path}", exc)
      raise
    self._log_success(f"Screenshot {emulator}:\n{output_path}")

  def close(self) -> None:
    for connection in self._connections.values():
      connection.transport.ws.close()
    self._connections.clear()
