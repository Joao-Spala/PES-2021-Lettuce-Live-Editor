"""
Builds the standalone Windows executable:
  dist/PES 2021 Ultimate Studio.exe
Requires app/.venv with pywebview + pyinstaller installed.
"""

import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
APP_DIR = ROOT / "app"
DIST_DIR = ROOT / "dist"
BUILD_DIR = APP_DIR / "build_tmp"
VENV_PY = APP_DIR / ".venv" / "Scripts" / "python.exe"


def main() -> None:
    py = str(VENV_PY if VENV_PY.exists() else Path(sys.executable))
    DIST_DIR.mkdir(parents=True, exist_ok=True)
    BUILD_DIR.mkdir(parents=True, exist_ok=True)

    cmd = [
        py,
        "-m",
        "PyInstaller",
        "--noconfirm",
        "--clean",
        "--onefile",
        "--noconsole",
        "--name",
        "PES 2021 Ultimate Studio",
        "--distpath",
        str(DIST_DIR),
        "--workpath",
        str(BUILD_DIR),
        "--specpath",
        str(BUILD_DIR),
        "--paths",
        str(APP_DIR),
        "--add-data",
        f"{APP_DIR / 'ui'};ui",
        "--add-data",
        f"{ROOT / 'src' / 'data'};data",
        str(APP_DIR / "main.py"),
    ]
    print("Building standalone EXE...")
    res = subprocess.run(cmd, cwd=str(ROOT))
    if res.returncode != 0:
        sys.exit(res.returncode)

    exe_path = DIST_DIR / "PES 2021 Ultimate Studio.exe"
    size_mb = exe_path.stat().st_size / (1024 * 1024)
    print(f"SUCCESS: {exe_path} ({size_mb:.2f} MB)")


if __name__ == "__main__":
    main()
