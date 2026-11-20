"""
PES 2021 Ultimate Studio - Standalone Windows Desktop App (EXE).
Launches a native Microsoft Edge WebView2 window backed by our pure Win32Memory
and x64 HookManager engine. Restores all game hooks cleanly on exit.
"""

import atexit
import os
import sys
from pathlib import Path

# Ensure app/ is on sys.path in dev mode
if not getattr(sys, "frozen", False):
    sys.path.insert(0, str(Path(__file__).resolve().parent))

import webview
from engine.pes_engine import PESEngine
from api import StudioAPI


def get_ui_path() -> str:
    if getattr(sys, "frozen", False) and hasattr(sys, "_MEIPASS"):
        base = Path(sys._MEIPASS) / "ui"
    else:
        base = Path(__file__).resolve().parent / "ui"
    return str(base / "index.html")


def main() -> None:
    engine = PESEngine()
    api = StudioAPI(engine)

    # Guarantee all x64 hooks are restored cleanly on exit
    atexit.register(engine.shutdown)

    window = webview.create_window(
        title="PES 2021 Ultimate Studio v12.0",
        url=get_ui_path(),
        js_api=api,
        width=1180,
        height=760,
        min_size=(960, 620),
        background_color="#0b0f17",
        text_select=False,
    )

    def on_closed() -> None:
        engine.shutdown()

    window.events.closed += on_closed
    webview.start(gui="edgechromium", debug=False)


if __name__ == "__main__":
    main()
