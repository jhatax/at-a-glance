from __future__ import annotations

import errno
import os
import re
import shutil
import signal
import socket
import sys
import tempfile
import threading
import time
import traceback
from collections.abc import Callable, Generator, Iterable
from contextlib import contextmanager
from pathlib import Path
from typing import Final

APP_MESSAGE_TIMEOUT_SECONDS: Final[int] = 2
PEBBLE_SETTLE_DELAY: Final[int] = 1
PEBBLE_SCREENSHOT_DELAY: Final[int] = 1
APP_READY_DELAY_SECONDS: Final[int] = 3
_EMULATOR_REGISTRY_LOCK = threading.RLock()
PEBBLE_BLUETOOTH_DISCONNECT_TIMEOUT: Final[int] = 29


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
  from pebble_tool.sdk import sdk_manager

  sdk_core = sdk_manager.current_path
  if sdk_core is None:
    raise ImportError("Pebble Tool has no active SDK")
  sdk_root = Path(sdk_core).parent
  qemu_bin = sdk_root / "toolchain" / "bin" / "qemu-pebble"
  if not qemu_bin.is_file():
    raise ImportError(f"Active SDK has no QEMU binary: {qemu_bin}")
  os.environ.setdefault("PEBBLE_QEMU_PATH", str(qemu_bin))
  compiler_path = sdk_root / "toolchain" / "arm-none-eabi" / "bin" / "arm-none-eabi-gcc"
  if not compiler_path.is_file():
    raise ImportError(f"Active SDK has no Pebble compiler: {compiler_path}")
  return compiler_path


try:
  _load_pebble_tool()
  from compilerdbgenerator import generate_compile_database
  from libpebble2.communication import PebbleConnection
  from libpebble2.communication.transports.qemu.protocol import (
      QemuBattery,
      QemuBluetoothConnection,
  )
  from libpebble2.services.appmessage import AppMessageService, CString, Int32
  from pebble_tool.commands.base import PebbleTransportEmulator
  from pebble_tool.commands.emucontrol import send_data_to_qemu
  from pebble_tool.commands.install import ToolAppInstaller
  from pebble_tool.commands.sdk.project import SDKProjectCommand
  from pebble_tool.sdk import sdk_manager
  from pebble_tool.sdk.emulator import (
      ManagedEmulatorTransport,
      get_emulator_info,
      update_emulator_info,
  )
  from pebble_tool.sdk.project import PebbleProject
except ImportError as exc:
  raise ImportError("libpebble2 or its Pebble Tool environment is unavailable") from exc


class PebbleAdapter:
  _log: Callable[[str], None]

  def __init__(self, log: Callable[[str], None]) -> None:
    self._log = log

  def log_message(self, log: str) -> None:
    self._log(f"{log}")

  @contextmanager
  def create_connection(
      self,
      emulator: str,
      logs: list[str],
  ) -> Generator[PebbleEmulatorConnection]:
    connection: PebbleEmulatorConnection = None
    try:
      with _EMULATOR_REGISTRY_LOCK:
        connection = PebbleEmulatorConnection(ManagedEmulatorTransport(emulator), emulator, logs)
        connection.connect()
        connection.run_async()
        PebbleTransportEmulator.post_connect(connection)
      if not connection.connected or connection.watch_info is None:
        raise ConnectionError(f"Emulator '{emulator}' connection is not ready")
      yield connection
    finally:
      with _EMULATOR_REGISTRY_LOCK:
        if connection:
          connection.transport.ws.close()

  @classmethod
  def kill_emulator(cls, emulator: str) -> None:
    from qaharnessruntime import (
        ANSI_BOLD,
        ANSI_RESET,
        ANSI_YELLOW,
    )

    info = {}
    print(f"{ANSI_YELLOW}{ANSI_BOLD}Killing emulator: {emulator}{ANSI_RESET}")

    with _EMULATOR_REGISTRY_LOCK:
      info = get_emulator_info(emulator, sdk_manager.get_current_sdk())
      if info is None:
        return
      pids_to_wait: list[int] = []
      for key in ("qemu", "pypkjs", "websockify"):
        pid = info.get(key, {}).get("pid")
        if not pid:
          continue
        try:
          os.kill(pid, signal.SIGTERM)
          pids_to_wait.append(pid)
        except OSError as exc:
          if exc.errno != errno.ESRCH:
            raise

      deadline = time.time() + 5.0
      for pid in pids_to_wait:
        while time.time() < deadline:
          try:
            os.kill(pid, 0)
          except OSError as exc:
            if exc.errno == errno.ESRCH:
              break
          time.sleep(0.05)
        else:
          try:
            os.kill(pid, signal.SIGKILL)
          except OSError:
            pass

      update_emulator_info(emulator, info["version"], None)

  # Best effort emulator termination
  @classmethod
  def kill_emulators(cls, emulators: Iterable[str]) -> None:
    if emulators:
      try:
        for emulator in emulators:
          PebbleAdapter.kill_emulator(emulator)
      except Exception: # noqa: BLE001, S110
        pass

  def restart_emulator(self, emulator: str) -> None:
    from qaharnessconfig import REPO_ROOT
    PebbleAdapter.kill_emulator(emulator)
    self.install_emulator(emulator, REPO_ROOT / "build" / "at-a-glance.pbw")

  def install_emulator(self, emulator: str, pbw_path: Path) -> None:
    from qaharnessruntime import (
        ANSI_BOLD,
        ANSI_GREEN,
        ANSI_RESET,
    )

    print(f"{ANSI_GREEN}{ANSI_BOLD}Installing emulator: {emulator}{ANSI_RESET}")
    logs: list[str] = []
    with self.create_connection(emulator, logs) as connection:
      installer = ToolAppInstaller(connection, str(pbw_path), quiet=True)
      try:
        installer.install()
        time.sleep(APP_READY_DELAY_SECONDS)
        logs.append(f"Success: Install {emulator}: {pbw_path}")
      except Exception as exc:
        logs.append(f"Failure: Install {emulator}: {pbw_path}", exc)
        raise
      finally:
        self.log_message("\n".join(logs))

  def install_emulators(self, emulators: Iterable[str], pbw_path: Path) -> None:
    for emulator in emulators:
      self.install_emulator(emulator, pbw_path)

  @classmethod
  def build(cls, verbose: bool = False, output_path: Path | None = None) -> None:
    compiler_error = re.compile(r"^.+:\d+(?::\d+)?:\s+(?:fatal )?error:")
    command = SDKProjectCommand()
    command.sdk = None
    command._verbosity = 1 if verbose else 0
    build_log_path = output_path or Path("build.log")
    capture_output = verbose or output_path is not None
    if capture_output:
      build_log_path.parent.mkdir(parents=True, exist_ok=True)
    output = build_log_path.open("w", encoding="utf-8") if capture_output else None
    saved_stdout = os.dup(1) if output else None
    saved_stderr = os.dup(2) if output else None
    build_exception: Exception | None = None
    try:
      if verbose:
        print("Will attempt to compile database for clangd integration if build succeeds...")
      if output:
        sys.stdout.flush()
        sys.stderr.flush()
        os.dup2(output.fileno(), 1)
        os.dup2(output.fileno(), 2)
      command._waf("configure")
      command._waf("build")
    except Exception as exc: # noqa: BLE001
      build_exception = exc
    finally:
      if output:
        sys.stdout.flush()
        sys.stderr.flush()
        os.dup2(saved_stdout, 1)
        os.close(saved_stdout)
        os.dup2(saved_stderr, 2)
        os.close(saved_stderr)
      if output and build_exception is not None:
        output.write("\nPython build traceback:\n")
        output.write("".join(traceback.format_exception(build_exception)))
      if output:
        output.close()
    if build_exception is not None:
      build_reason = ""
      if capture_output:
        build_reason = "\n".join(
            line for line in build_log_path.read_text(encoding="utf-8").splitlines()
            if compiler_error.match(line)
        )
      if not build_reason:
        build_reason = "Build failed; rerun with --verbose for compiler diagnostics"
      operation = f"Build {'verbose ' if verbose else ''}".strip()
      print(f"{operation} status=failed error={build_reason}")
      raise ValueError(build_reason) from None
    if verbose:
      try:
        compile_database_path = build_log_path.parent / "compile_commands.json"
        generate_compile_database(
            log_path=build_log_path,
            output_path=compile_database_path,
            platform="emery",
        )
        print(f"Compile database generated: {compile_database_path}")
      except Exception as exc: # noqa: BLE001
        print(f"No compile database generated: {exc}")
    print(f"Success: Build {'verbose ' if verbose else ''}".strip())


class PebbleEmulatorConnection(PebbleConnection):
  monitor_port: int
  transport: ManagedEmulatorTransport
  emulator: str
  logs: list[str]

  def __init__(self, transport: ManagedEmulatorTransport, emu: str, logs: list[str]) -> None:
    super().__init__(transport)
    self.transport = transport
    self.monitor_port = getattr(transport, "qemu_monitor_port", None)
    self.emulator = emu
    self.logs = logs

  def run_sync(self) -> None:
    try:
      super().run_sync()
    except Exception as exc:
      if self.transport.connected and not isinstance(exc, OSError):
        raise

  def send_app_message(self, values: dict[int, int | str]) -> None:
    service = None
    ack_handle = None
    nack_handle = None
    try:
      service = AppMessageService(super())
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
            f"AppMessage timed out waiting for emulator '{self.emulator}' acknowledgement"
        )
      if "nack" in outcome:
        nack_args = outcome["nack"]
        raise RuntimeError(f"AppMessage rejected by emulator '{self.emulator}': {nack_args}")
    except Exception as exc:
      self.logs.append(f"Failure: AppMessage {self.emulator}: {values}", exc)
      raise
    finally:
      if service is not None:
        if ack_handle is not None:
          service.unregister_handler(ack_handle)
        if nack_handle is not None:
          service.unregister_handler(nack_handle)
        service.shutdown()
      time.sleep(PEBBLE_SETTLE_DELAY)
    self.logs.append(f"Success: AppMessage {self.emulator}: {values}")

  def set_battery(self, percent: int, charging: int) -> None:
    try:
      send_data_to_qemu(
          self.transport,
          QemuBattery(percent=percent, charging=charging),
      )
    except Exception as exc:
      self.logs.append(
          f"Failure: Battery {self.emulator}: {percent}% charging={charging}, Error: {exc}"
      )
      raise
    finally:
      time.sleep(PEBBLE_SETTLE_DELAY)
    self.logs.append(f"Success: Battery {self.emulator}: {percent}% charging={charging}")

  def set_bluetooth(self, connected: int) -> bool:
    try:
      send_data_to_qemu(
          self.transport,
          QemuBluetoothConnection(connected=(connected == 1)),
      )
    except Exception as exc:
      self.logs.append(f"Failure: Bluetooth {self.emulator}: connected={connected}, Error: {exc}")
      raise
    finally:
      if not connected:
        time.sleep(PEBBLE_BLUETOOTH_DISCONNECT_TIMEOUT) # bluetooth delay is long
      else:
        time.sleep(PEBBLE_SETTLE_DELAY)

    self.logs.append(f"Success: Bluetooth {self.emulator}: connected={connected}")
    return connected == 0

  def screenshot(self, output_path: Path) -> None:
    try:
      if not output_path.parent.exists():
        output_path.parent.mkdir(parents=True, exist_ok=True)

      time.sleep(PEBBLE_SETTLE_DELAY)
      from PIL import Image
      with tempfile.TemporaryDirectory(prefix=f"qa-screenshot-{self.emulator}-") as temp_dir:
        ppm_path = Path(temp_dir) / "screen.ppm"
        with socket.create_connection(
            ("127.0.0.1", int(self.monitor_port)),
            timeout=1.5,
        ) as monitor:
          monitor.settimeout(1.5)
          try:
            monitor.recv(4096)
          except TimeoutError:
            pass
          monitor.sendall(f"screendump {ppm_path}\n".encode())
          try:
            monitor.recv(4096)
          except TimeoutError:
            pass

        deadline = time.time() + 0.80
        while time.time() < deadline and not ppm_path.exists():
          time.sleep(0.1)
        if not ppm_path.exists() or ppm_path.stat().st_size == 0:
          raise TimeoutError(f"Timed out waiting for emulator screendump: {ppm_path}")
        with Image.open(ppm_path) as image:
          image.convert("RGB").save(output_path)
    except Exception as exc:
      self.logs.append(f"Failure: Screenshot {self.emulator}:\n{output_path}, Error: {exc}")
      raise
    self.logs.append(f"Success: Screenshot {self.emulator}:\n{output_path}")
