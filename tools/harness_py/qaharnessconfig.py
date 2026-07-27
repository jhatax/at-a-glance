from pathlib import Path
import sys
from types import MappingProxyType
from typing import Final

HARNESS_ROOT: Final = Path(__file__).resolve().parent
REPO_ROOT: Final = HARNESS_ROOT.parents[1]
QA_ROOT: Final = REPO_ROOT / "qa"
PLANS_ROOT: Final = QA_ROOT / "plans"
QARUNS_ROOT: Final = QA_ROOT / "qa-runs"

DISPLAY_MODE_VALUES: Final = MappingProxyType(
    {
        "white": 0,
        "black": 1,
        "celeste": 2,
        "oxford": 3,
    }
)

for path in (HARNESS_ROOT, REPO_ROOT, QA_ROOT):
  if path not in sys.path:
    sys.path.insert(0, str(path))
