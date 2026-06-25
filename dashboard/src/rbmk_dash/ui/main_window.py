"""Main window: operator controls + tabbed views + status bar.

The window owns the simulation clock: a fixed-period QTimer advances the
session by a whole number of kernel steps per tick (fractional remainders are
accumulated so any speed multiplier stays exact on average).
"""

from __future__ import annotations

from datetime import datetime

from PySide6.QtCore import QTimer
from PySide6.QtWidgets import (
    QFileDialog,
    QHBoxLayout,
    QLabel,
    QMainWindow,
    QSplitter,
    QTabWidget,
    QVBoxLayout,
    QWidget,
)

from rbmk_dash import __version__
from rbmk_dash.core import bindings
from rbmk_dash.core.recorder import RecordingSession
from rbmk_dash.core.session import Session
from rbmk_dash.ui.panels.annunciator import AnnunciatorPanel
from rbmk_dash.ui.panels.comparison import ComparisonPanel
from rbmk_dash.ui.panels.controls import ControlsPanel
from rbmk_dash.ui.panels.instruments import InstrumentsPanel
from rbmk_dash.ui.panels.plots import PlotsPanel
from rbmk_dash.ui.panels.timeline import TimelinePanel

TICK_MS = 50


class MainWindow(QMainWindow):
    def __init__(self, session: Session | None = None) -> None:
        super().__init__()
        self.session = session if session is not None else RecordingSession()
        self._speed = 1.0
        self._step_accumulator = 0.0

        self.setWindowTitle("RBMK-SIM — educational reactor simulator (toy model)")
        self.resize(1480, 920)

        # --- panels -----------------------------------------------------
        self.controls = ControlsPanel(self.session)
        self.plots = PlotsPanel()
        self.instruments = InstrumentsPanel()
        self.annunciator = AnnunciatorPanel()

        right_column = QWidget()
        right_layout = QVBoxLayout(right_column)
        right_layout.setContentsMargins(0, 0, 0, 0)
        right_layout.addWidget(self.annunciator)
        right_layout.addWidget(self.instruments)
        right_layout.addStretch(1)
        right_column.setFixedWidth(310)

        operate_tab = QWidget()
        operate_layout = QHBoxLayout(operate_tab)
        operate_layout.setContentsMargins(4, 4, 4, 4)
        operate_layout.addWidget(self.plots, 1)
        operate_layout.addWidget(right_column)

        self.tabs = QTabWidget()
        self.tabs.addTab(operate_tab, "Operate")

        self.timeline = TimelinePanel()
        self.tabs.addTab(self.timeline, "Timeline replay")

        self.comparison = ComparisonPanel()
        self.tabs.addTab(self.comparison, "Design comparison")

        splitter = QSplitter()
        splitter.addWidget(self.controls)
        splitter.addWidget(self.tabs)
        splitter.setStretchFactor(1, 1)
        self.setCentralWidget(splitter)

        # --- status bar ---------------------------------------------------
        self._status_model = QLabel(
            f"model {bindings.model_version()} | ABI {bindings.ABI_VERSION} "
            f"| dash {__version__}"
        )
        self._status_clock = QLabel("t = 0.0 s")
        self._status_validity = QLabel("")
        self.statusBar().addWidget(self._status_model)
        self.statusBar().addPermanentWidget(self._status_validity)
        self.statusBar().addPermanentWidget(self._status_clock)

        # --- clock ----------------------------------------------------------
        self.timer = QTimer(self)
        self.timer.setInterval(TICK_MS)
        self.timer.timeout.connect(self._on_tick)

        self.controls.run_toggled.connect(self._on_run_toggled)
        self.controls.speed_changed.connect(self._on_speed_changed)
        self.controls.step_once.connect(self._on_step_once)
        self.controls.save_requested.connect(self._on_save_requested)

        self.refresh_all()

    # ------------------------------------------------------------------ clock
    def _on_run_toggled(self, running: bool) -> None:
        if running:
            self.timer.start()
        else:
            self.timer.stop()

    def _on_speed_changed(self, speed: float) -> None:
        self._speed = speed

    def _on_step_once(self) -> None:
        self.advance_steps(max(1, round(1.0 / self.session.dt_s)))  # one sim-second

    def _on_save_requested(self) -> None:
        if not isinstance(self.session, RecordingSession):
            self.statusBar().showMessage("This session does not record", 4000)
            return
        default_dir = bindings.repo_root() / "runs"
        default_name = datetime.now().strftime("rbmk_%Y%m%d_%H%M%S.h5")
        path, _ = QFileDialog.getSaveFileName(
            self,
            "Save run log",
            str(default_dir / default_name),
            "HDF5 run logs (*.h5)",
        )
        if path:
            saved = self.session.save(path)
            self.statusBar().showMessage(f"Run log saved: {saved}", 6000)

    def _on_tick(self) -> None:
        tick_s = TICK_MS / 1000.0
        self._step_accumulator += self._speed * tick_s / self.session.dt_s
        steps = int(self._step_accumulator)
        if steps > 0:
            self._step_accumulator -= steps
            self.advance_steps(steps)

    # ------------------------------------------------------------------ logic
    def advance_steps(self, steps: int) -> None:
        self.session.advance(steps)
        self.refresh_all()

    def refresh_all(self) -> None:
        self.plots.refresh(self.session)
        self.instruments.refresh(self.session)
        self.annunciator.refresh(self.session)
        self.controls.refresh(self.session)

        obs = self.session.observation
        self._status_clock.setText(
            f"t = {obs.time_s:9.1f} s | step {int(obs.step_count)}"
        )
        self._status_validity.setText(
            "MODEL VALIDITY EXCEEDED (qualitative only) | " if obs.validity_exceeded else ""
        )
