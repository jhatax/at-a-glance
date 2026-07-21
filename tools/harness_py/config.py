from pathlib import Path
import sys
from typing import Final

HARNESS_ROOT: Final = Path(__file__).resolve().parent
REPO_ROOT: Final = HARNESS_ROOT.parents[1]
QA_ROOT: Final = REPO_ROOT / "qa"
PLANS_ROOT: Final = QA_ROOT / "plans"

for path in (HARNESS_ROOT, REPO_ROOT, QA_ROOT):
  if path not in sys.path:
    sys.path.insert(0, str(path))
