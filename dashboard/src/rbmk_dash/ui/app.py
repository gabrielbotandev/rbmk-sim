"""Application entry point."""

from __future__ import annotations

import sys


def main() -> int:
    from PySide6.QtWidgets import QApplication

    from rbmk_dash.ui.main_window import MainWindow
    from rbmk_dash.ui.theme import apply_theme, configure_pyqtgraph

    app = QApplication(sys.argv)
    app.setApplicationName("rbmk-sim dashboard")
    apply_theme(app)
    configure_pyqtgraph()

    window = MainWindow()
    window.show()
    return app.exec()


if __name__ == "__main__":
    raise SystemExit(main())
