from __future__ import annotations

import sys
import time
import os
import shutil
import socket
import tempfile
from contextlib import contextmanager
from pathlib import Path
from collections.abc import Generator
from typing import Callable, Final, Iterable
import threading

APP_MESSAGE_TIMEOUT_SECONDS: Final[float] = 2.0
PEBBLE_SETTLE_DELAY: Final[float] = 1.5
APP_READY_DELAY_SECONDS: Final[int] = 3


def _load_pebble_tool() -> Path:
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
  from pebble_tool.sdk import sdk_manager, sdk_version

  active_sdk = sdk_version()
  if not active_sdk:
    raise ImportError("Pebble Tool has no active SDK")
  sdk_core = Path(sdk_manager.path_for_sdk(active_sdk))
  sdk_root = sdk_core.parent
  qemu_bin = sdk_root / "toolchain" / "bin" / "qemu-pebble"
  if not qemu_bin.is_file():
    raise ImportError(f"Active SDK has no QEMU binary: {qemu_bin}")
  os.environ.setdefault("PEBBLE_QEMU_PATH", str(qemu_bin))
  compiler_path = sdk_root / "toolchain" / "arm-none-eabi" / "bin" / "arm-none-eabi-gcc"
  if not compiler_path.is_file():
    raise ImportError(f"Active SDK has no Pebble compiler: {compiler_path}")
  return compiler_path


try:
  _PEBBLE_COMPILER_PATH = _load_pebble_tool()
  from libpebble2.communication import PebbleConnection # noqa: E402
  from libpebble2.communication.transports.qemu.protocol import QemuBattery, QemuBluetoothConnection # noqa: E402
  from libpebble2.services.appmessage import AppMessageService, CString, Int32 # noqa: E402
  from pebble_tool.commands.emucontrol import send_data_to_qemu # noqa: E402
  from pebble_tool.commands.base import PebbleTransportEmulator # noqa: E402
  from pebble_tool.commands.install import ToolAppInstaller # noqa: E402
  from pebble_tool.commands.sdk.project.build import BuildCommand # noqa: E402
  from compilerdbgenerator import generate_compile_database # noqa: E402
  from pebble_tool.sdk.emulator import ManagedEmulatorTransport # noqa: E402
  from pebble_tool.sdk.project import PebbleProject # noqa: E402
except ImportError as exc:
  raise ImportError("libpebble2 or its Pebble Tool environment is unavailable") from exc


class _HarnessPebbleConnection(PebbleConnection):
  """Treat a relay closure as normal when a test changes emulator connectivity."""

  def run_sync(self) -> None:
    try:
      super().run_sync()
    except Exception as exc:
      if self.transport.connected and not isinstance(exc, OSError):
        raise


class PebbleAdapter:

  def __init__(self, log: Callable[[str], None]) -> None:
    self._log = log

  def _log_failure(self, operation: str, exc: Exception) -> None:
    self._log(f"{operation} status=failed error={exc}")

  def _log_success(self, operation: str) -> None:
    self._log(f"{operation} status=success")

  @contextmanager
  def _connection(self, emulator: str) -> Generator[PebbleConnection]:
    connection: PebbleConnection | None = None
    try:
      connection = _HarnessPebbleConnection(ManagedEmulatorTransport(emulator))
      connection.connect()
      connection.run_async()
      PebbleTransportEmulator.post_connect(connection)
      yield connection
    finally:
      if connection is not None:
        self._close_connection(connection)

  def _close_connection(self, connection: PebbleConnection) -> None:
    try:
      connection.transport.ws.close()
    except Exception:
      pass

  def send_app_message(self, emulator: str, values: dict[int, int | str]) -> None:
    with self._connection(emulator) as connection:
      service = None
      ack_handle = None
      nack_handle = None
      try:
        time.sleep(PEBBLE_SETTLE_DELAY)
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
        if service is not None:
          if ack_handle is not None:
            service.unregister_handler(ack_handle)
          if nack_handle is not None:
            service.unregister_handler(nack_handle)
          service.shutdown()
        time.sleep(PEBBLE_SETTLE_DELAY)
    self._log_success(f"AppMessage {emulator}: {values}")

  def install(self, emulator: str, pbw_path: Path) -> None:
    with self._connection(emulator) as connection:
      installer = ToolAppInstaller(connection, str(pbw_path), quiet=True)
      try:
        installer.install()
        time.sleep(APP_READY_DELAY_SECONDS)
      except Exception as exc:
        self._log_failure(f"Install {emulator}: {pbw_path}", exc)
        raise
    self._log_success(f"Install {emulator}: {pbw_path}")

  def install_emulators(self, emulators: Iterable[str], pbw_path: Path) -> None:
    for emulator in emulators:
      self.install(emulator, pbw_path)

  def build(self, verbose: bool = False, output_path: Path | None = None) -> None:
    command = BuildCommand()
    build_log_path = output_path or Path("build.log")
    if verbose:
      build_log_path.parent.mkdir(parents=True, exist_ok=True)
    output = build_log_path.open("w", encoding="utf-8") if verbose else None
    saved_stdout = os.dup(1) if output else None
    saved_stderr = os.dup(2) if output else None
    build_exception: Exception | None = None
    try:
      if output:
        if verbose:
          self._log("Will attempt to compile database for clangd integration if build succeeds...")
        sys.stdout.flush()
        sys.stderr.flush()
        os.dup2(output.fileno(), 1)
        os.dup2(output.fileno(), 2)
      from argparse import Namespace

      command(Namespace(sdk=None, v=1 if verbose else 0, args=[], debug=False))
    except Exception as exc:
      build_exception = exc
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
    if build_exception is not None:
      build_reason = (
          build_log_path.read_text(encoding="utf-8")
          if verbose and build_log_path.is_file() else str(build_exception)
      ).strip()
      operation = f"Build {'verbose ' if verbose else ''}".strip()
      self._log(f"{operation} status=failed error={build_reason}")
      if verbose:
        sys.stderr.write(f"Build failed: {build_reason}\n")
        sys.stderr.flush()
      raise build_exception
    if verbose:
      try:
        compile_database_path = build_log_path.parent / "compile_commands.json"
        generate_compile_database(
            log_path=build_log_path,
            output_path=compile_database_path,
            platform="emery",
            compiler_path=_PEBBLE_COMPILER_PATH,
        )
        self._log(f"Compile database generated: {compile_database_path}")
      except Exception as exc:
        self._log(f"No compile database generated: {exc}")
    self._log_success(f"Build {'verbose ' if verbose else ''}".strip())

  def set_battery(self, emulator: str, percent: int, charging: int) -> None:
    with self._connection(emulator) as connection:
      time.sleep(PEBBLE_SETTLE_DELAY)
      try:
        send_data_to_qemu(
            connection.transport,
            QemuBattery(percent=percent, charging=charging),
        )
      except Exception as exc:
        self._log_failure(f"Battery {emulator}: {percent}% charging={charging}", exc)
        raise
      finally:
        time.sleep(PEBBLE_SETTLE_DELAY)
    self._log_success(f"Battery {emulator}: {percent}% charging={charging}")

  def set_bluetooth(self, emulator: str, connected: int) -> None:
    with self._connection(emulator) as connection:
      try:
        time.sleep(PEBBLE_SETTLE_DELAY)
        send_data_to_qemu(
            connection.transport,
            QemuBluetoothConnection(connected=True if connected == 1 else False),
        )
      except Exception as exc:
        self._log_failure(f"Bluetooth {emulator}: connected={connected}", exc)
        raise
      finally:
        time.sleep(PEBBLE_SETTLE_DELAY)
    self._log_success(f"Bluetooth {emulator}: connected={connected}")

  def screenshot(self, emulator: str, output_path: Path) -> None:
    try:
      if not output_path.parent.exists():
        output_path.parent.mkdir(parents=True, exist_ok=True)

      from PIL import Image

      transport = ManagedEmulatorTransport(emulator)
      monitor_port = getattr(transport, "qemu_monitor_port", None)
      if not monitor_port:
        raise RuntimeError(f"QEMU monitor port unavailable for emulator '{emulator}'")

      with tempfile.TemporaryDirectory(prefix=f"qa-screenshot-{emulator}-") as temp_dir:
        ppm_path = Path(temp_dir) / "screen.ppm"
        with socket.create_connection(("127.0.0.1", int(monitor_port)), timeout=1.5) as monitor:
          monitor.settimeout(1.5)
          try:
            monitor.recv(4096)
          except socket.timeout:
            pass
          monitor.sendall(f"screendump {ppm_path}\n".encode("utf-8"))
          try:
            monitor.recv(4096)
          except socket.timeout:
            pass

        deadline = time.time() + 0.80
        while time.time() < deadline and not ppm_path.exists():
          time.sleep(0.1)
        if not ppm_path.exists() or ppm_path.stat().st_size == 0:
          raise TimeoutError(f"Timed out waiting for emulator screendump: {ppm_path}")
        with Image.open(ppm_path) as image:
          image.convert("RGB").save(output_path)
    except Exception as exc:
      self._log_failure(f"Screenshot {emulator}:\n{output_path}", exc)
      raise
    self._log_success(f"Screenshot {emulator}:\n{output_path}")

  def close(self) -> None:
    return
