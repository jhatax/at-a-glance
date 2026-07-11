from __future__ import annotations

from dataclasses import dataclass
import os
from pathlib import Path


def _env_bool(name: str) -> bool:
    return os.environ.get(name, "false") == "true"


def _env_list(name: str) -> list[str]:
    value = os.environ.get(name, "")
    if not value:
        return []
    return [item for item in value.split(",") if item]


@dataclass(frozen=True)
class CliState:
    build: bool
    clean: bool
    install: bool
    wipe: bool
    nuke: bool
    run_qa: bool
    phone_install: bool
    phone_ip: str
    scenario_name: str
    emulators: list[str]
    script_dir: Path
    qa_dir: Path
    compile_db_path: Path


def read_cli_state_from_env() -> CliState:
    script_dir = Path(os.environ["SCRIPT_DIR"])
    qa_dir = Path(os.environ["QA_DIR"])
    compile_db_path = Path(os.environ["COMPILE_DB_PATH"])

    return CliState(
        build=_env_bool("BUILD"),
        clean=_env_bool("CLEAN"),
        install=_env_bool("INSTALL"),
        wipe=_env_bool("WIPE"),
        nuke=_env_bool("NUKE"),
        run_qa=_env_bool("RUN_QA"),
        phone_install=_env_bool("PHONE_INSTALL"),
        phone_ip=os.environ.get("PHONE_IP", ""),
        scenario_name=os.environ.get("SCENARIO_NAME", ""),
        emulators=_env_list("EMULATORS_CSV"),
        script_dir=script_dir,
        qa_dir=qa_dir,
        compile_db_path=compile_db_path,
    )
