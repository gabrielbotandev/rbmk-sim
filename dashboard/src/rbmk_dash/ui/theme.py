"""Engineering dashboard theme: restrained dark palette, semantic status
colors only (green/amber/red reserved for actual plant state), monospaced
numerics. No decorative effects."""

from __future__ import annotations

from PySide6.QtGui import QColor, QPalette
from PySide6.QtWidgets import QApplication

COLORS = {
    "bg": "#14171c",
    "surface": "#1b2026",
    "surface_alt": "#20262e",
    "border": "#2c343f",
    "text": "#c9d2dc",
    "text_dim": "#8a96a3",
    "accent": "#5d8fc9",
    "ok": "#3f9e58",
    "warn": "#d9a23c",
    "danger": "#cf4a3f",
    "info": "#5b9bd5",
}

MONO_FONT = "DejaVu Sans Mono, Consolas, monospace"

PLOT_SERIES = {
    "power": "#e8c252",
    "rho_total": "#e8e8e8",
    "rho_rods": "#5d8fc9",
    "rho_void": "#cf6a3f",
    "rho_doppler": "#3f9e58",
    "rho_xenon": "#9d6fd0",
    "xenon": "#9d6fd0",
    "iodine": "#5b9bd5",
    "void": "#cf6a3f",
    "flow": "#5b9bd5",
    "fuel": "#cf4a3f",
    "coolant": "#5b9bd5",
    "rods": ("#5d8fc9", "#7aa7d8", "#3f9e58", "#cf4a3f"),
}


def _stylesheet() -> str:
    c = COLORS
    return f"""
    QWidget {{
        background-color: {c["bg"]};
        color: {c["text"]};
        font-size: 12px;
    }}
    QGroupBox {{
        background-color: {c["surface"]};
        border: 1px solid {c["border"]};
        border-radius: 3px;
        margin-top: 14px;
        padding: 6px 6px 6px 6px;
    }}
    QGroupBox::title {{
        subcontrol-origin: margin;
        left: 8px;
        padding: 0 4px;
        color: {c["text_dim"]};
        font-weight: bold;
        text-transform: uppercase;
    }}
    QPushButton {{
        background-color: {c["surface_alt"]};
        border: 1px solid {c["border"]};
        border-radius: 3px;
        padding: 5px 10px;
    }}
    QPushButton:hover {{ border-color: {c["accent"]}; }}
    QPushButton:pressed {{ background-color: {c["bg"]}; }}
    QPushButton:checked {{
        background-color: {c["accent"]};
        color: {c["bg"]};
        font-weight: bold;
    }}
    QPushButton#azButton {{
        background-color: #7c2a22;
        border: 2px solid {c["danger"]};
        color: #f2d9d6;
        font-weight: bold;
        font-size: 14px;
        padding: 10px;
    }}
    QPushButton#azButton:hover {{ background-color: {c["danger"]}; color: white; }}
    QDoubleSpinBox, QSpinBox, QComboBox {{
        background-color: {c["surface_alt"]};
        border: 1px solid {c["border"]};
        border-radius: 3px;
        padding: 2px 4px;
    }}
    QCheckBox::indicator {{
        width: 13px; height: 13px;
        border: 1px solid {c["border"]};
        background: {c["surface_alt"]};
    }}
    QCheckBox::indicator:checked {{ background: {c["accent"]}; }}
    QProgressBar {{
        background-color: {c["surface_alt"]};
        border: 1px solid {c["border"]};
        border-radius: 2px;
        text-align: center;
        color: {c["text"]};
        font-family: {MONO_FONT};
        font-size: 10px;
    }}
    QProgressBar::chunk {{ background-color: {c["accent"]}; }}
    QTabWidget::pane {{ border: 1px solid {c["border"]}; }}
    QTabBar::tab {{
        background: {c["surface"]};
        border: 1px solid {c["border"]};
        padding: 5px 14px;
    }}
    QTabBar::tab:selected {{
        background: {c["surface_alt"]};
        border-bottom: 2px solid {c["accent"]};
    }}
    QStatusBar {{
        background: {c["surface"]};
        color: {c["text_dim"]};
        font-family: {MONO_FONT};
    }}
    QLabel[role="readoutValue"] {{
        font-family: {MONO_FONT};
        font-size: 13px;
        color: {c["text"]};
        background-color: {c["surface_alt"]};
        border: 1px solid {c["border"]};
        border-radius: 2px;
        padding: 2px 6px;
    }}
    QLabel[role="readoutBig"] {{
        font-family: {MONO_FONT};
        font-size: 20px;
        font-weight: bold;
        color: #e8c252;
        background-color: {c["surface_alt"]};
        border: 1px solid {c["border"]};
        border-radius: 2px;
        padding: 4px 8px;
    }}
    QLabel[role="annTile"] {{
        font-family: {MONO_FONT};
        font-size: 11px;
        font-weight: bold;
        border: 1px solid {c["border"]};
        border-radius: 2px;
        padding: 8px 4px;
        background-color: {c["surface_alt"]};
        color: {c["text_dim"]};
    }}
    QLabel[role="annTile"][level="alarm"] {{
        background-color: #4d3a14;
        border-color: {c["warn"]};
        color: {c["warn"]};
    }}
    QLabel[role="annTile"][level="trip"] {{
        background-color: #4d1a14;
        border-color: {c["danger"]};
        color: #f2b8b2;
    }}
    QLabel[role="stateBanner"] {{
        font-family: {MONO_FONT};
        font-size: 15px;
        font-weight: bold;
        border-radius: 3px;
        padding: 8px;
    }}
    QLabel[role="stateBanner"][state="normal"] {{ background: #1d3a25; color: #8fd6a2; }}
    QLabel[role="stateBanner"][state="alarm"] {{ background: #4d3a14; color: #ecc77e; }}
    QLabel[role="stateBanner"][state="tripped"] {{ background: #4d1a14; color: #f2b8b2; }}
    QLabel[role="stateBanner"][state="shutdown"] {{ background: #16303f; color: #9cc8e8; }}
    """


def apply_theme(app: QApplication) -> None:
    app.setStyle("Fusion")
    palette = QPalette()
    palette.setColor(QPalette.ColorRole.Window, QColor(COLORS["bg"]))
    palette.setColor(QPalette.ColorRole.Base, QColor(COLORS["surface"]))
    palette.setColor(QPalette.ColorRole.AlternateBase, QColor(COLORS["surface_alt"]))
    palette.setColor(QPalette.ColorRole.Text, QColor(COLORS["text"]))
    palette.setColor(QPalette.ColorRole.WindowText, QColor(COLORS["text"]))
    palette.setColor(QPalette.ColorRole.Button, QColor(COLORS["surface_alt"]))
    palette.setColor(QPalette.ColorRole.ButtonText, QColor(COLORS["text"]))
    palette.setColor(QPalette.ColorRole.Highlight, QColor(COLORS["accent"]))
    palette.setColor(QPalette.ColorRole.HighlightedText, QColor(COLORS["bg"]))
    app.setPalette(palette)
    app.setStyleSheet(_stylesheet())


def configure_pyqtgraph() -> None:
    import pyqtgraph as pg

    pg.setConfigOptions(
        background=COLORS["bg"],
        foreground=COLORS["text_dim"],
        antialias=True,
    )


def repolish(widget) -> None:
    """Re-applies the stylesheet after a dynamic property change."""
    style = widget.style()
    style.unpolish(widget)
    style.polish(widget)
