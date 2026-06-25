"""Operator controls: simulation clock, rod banks, plant systems, protection."""

from __future__ import annotations

from PySide6.QtCore import Signal
from PySide6.QtWidgets import (
    QCheckBox,
    QComboBox,
    QDoubleSpinBox,
    QGridLayout,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QProgressBar,
    QPushButton,
    QVBoxLayout,
    QWidget,
)

from rbmk_dash.core import bindings
from rbmk_dash.core.session import Session

SPEED_STEPS: tuple[tuple[str, float], ...] = (
    ("0.5×", 0.5),
    ("1×", 1.0),
    ("2×", 2.0),
    ("5×", 5.0),
    ("10×", 10.0),
    ("60×", 60.0),
)


class ControlsPanel(QWidget):
    """Left-hand operator column. Forwards every action through the session's
    command log so runs stay replayable."""

    run_toggled = Signal(bool)
    speed_changed = Signal(float)
    step_once = Signal()
    save_requested = Signal()

    def __init__(self, session: Session, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self._session = session

        outer = QVBoxLayout(self)
        outer.setContentsMargins(4, 4, 4, 4)
        outer.setSpacing(6)
        outer.addWidget(self._build_clock_group())
        outer.addWidget(self._build_rods_group())
        outer.addWidget(self._build_plant_group())
        outer.addWidget(self._build_protection_group())
        outer.addWidget(self._build_session_group())
        outer.addStretch(1)
        self.setFixedWidth(300)

    # ------------------------------------------------------------------ build
    def _build_clock_group(self) -> QGroupBox:
        box = QGroupBox("Simulation")
        layout = QGridLayout(box)

        self._run_button = QPushButton("Run")
        self._run_button.setCheckable(True)
        self._run_button.toggled.connect(self._on_run_toggled)
        layout.addWidget(self._run_button, 0, 0)

        self._speed = QComboBox()
        for label, _ in SPEED_STEPS:
            self._speed.addItem(label)
        self._speed.setCurrentIndex(1)  # 1x
        self._speed.currentIndexChanged.connect(self._on_speed_changed)
        layout.addWidget(self._speed, 0, 1)

        step_button = QPushButton("Step 1 s")
        step_button.clicked.connect(self.step_once.emit)
        layout.addWidget(step_button, 1, 0, 1, 2)
        return box

    def _build_rods_group(self) -> QGroupBox:
        box = QGroupBox("Control rod banks")
        layout = QGridLayout(box)
        layout.setVerticalSpacing(4)

        self._rod_bars: list[QProgressBar] = []
        self._rod_targets: list[QDoubleSpinBox] = []
        for bank in range(bindings.NUM_BANKS):
            label = QLabel(bindings.BANK_NAMES[bank])
            bar = QProgressBar()
            bar.setRange(0, 1000)
            bar.setFormat("%p% in")
            bar.setFixedHeight(14)

            spin = QDoubleSpinBox()
            spin.setRange(0.0, 1.0)
            spin.setSingleStep(0.05)
            spin.setDecimals(2)
            spin.setToolTip("Demanded insertion fraction")
            spin.valueChanged.connect(
                lambda value, b=bank: self._session.apply("set_rod_target", b, float(value))
            )

            row = bank * 2
            layout.addWidget(label, row, 0, 1, 2)
            layout.addWidget(bar, row + 1, 0)
            layout.addWidget(spin, row + 1, 1)
            self._rod_bars.append(bar)
            self._rod_targets.append(spin)
        return box

    def _build_plant_group(self) -> QGroupBox:
        box = QGroupBox("Plant systems")
        layout = QGridLayout(box)

        layout.addWidget(QLabel("Pump flow [×nom]"), 0, 0)
        self._flow = QDoubleSpinBox()
        self._flow.setRange(0.05, 1.5)
        self._flow.setSingleStep(0.05)
        self._flow.setValue(1.0)
        self._flow.setDecimals(2)
        self._flow.valueChanged.connect(
            lambda value: self._session.apply("set_pump_flow", float(value))
        )
        layout.addWidget(self._flow, 0, 1)

        layout.addWidget(QLabel("Power setpoint [×nom]"), 1, 0)
        self._setpoint = QDoubleSpinBox()
        self._setpoint.setRange(0.0, 1.2)
        self._setpoint.setSingleStep(0.05)
        self._setpoint.setValue(1.0)
        self._setpoint.setDecimals(2)
        self._setpoint.valueChanged.connect(
            lambda value: self._session.apply("set_power_setpoint", float(value))
        )
        layout.addWidget(self._setpoint, 1, 1)

        self._ar_enabled = QCheckBox("Automatic regulator (AR)")
        self._ar_enabled.setChecked(True)
        self._ar_enabled.toggled.connect(
            lambda checked: self._session.apply("set_ar_enabled", bool(checked))
        )
        layout.addWidget(self._ar_enabled, 2, 0, 1, 2)
        return box

    def _build_protection_group(self) -> QGroupBox:
        box = QGroupBox("Protection")
        layout = QVBoxLayout(box)

        self._az5 = QPushButton("AZ-5\nEMERGENCY SHUTDOWN")
        self._az5.setObjectName("azButton")
        self._az5.setMinimumHeight(56)
        self._az5.clicked.connect(lambda: self._session.apply("press_az5"))
        layout.addWidget(self._az5)

        reset = QPushButton("Request RPS reset")
        reset.clicked.connect(lambda: self._session.apply("request_reset"))
        layout.addWidget(reset)

        det_row = QHBoxLayout()
        det_row.addWidget(QLabel("Detectors valid:"))
        self._det_checks: list[QCheckBox] = []
        for k in range(bindings.NUM_DETECTORS):
            check = QCheckBox(f"D{k + 1}")
            check.setChecked(True)
            check.toggled.connect(self._on_detector_toggled)
            det_row.addWidget(check)
            self._det_checks.append(check)
        layout.addLayout(det_row)
        return box

    def _build_session_group(self) -> QGroupBox:
        box = QGroupBox("Session log")
        layout = QVBoxLayout(box)

        self._samples_label = QLabel("samples: 0")
        layout.addWidget(self._samples_label)

        save = QPushButton("Save run log (HDF5)…")
        save.clicked.connect(self.save_requested.emit)
        layout.addWidget(save)
        return box

    # ------------------------------------------------------------------ slots
    def _on_run_toggled(self, checked: bool) -> None:
        self._run_button.setText("Pause" if checked else "Run")
        self.run_toggled.emit(checked)

    def _on_speed_changed(self, index: int) -> None:
        self.speed_changed.emit(SPEED_STEPS[index][1])

    def _on_detector_toggled(self) -> None:
        mask = 0
        for k, check in enumerate(self._det_checks):
            if check.isChecked():
                mask |= 1 << k
        self._session.apply("set_detectors_valid", mask)

    # ------------------------------------------------------------------ state
    @property
    def speed(self) -> float:
        return SPEED_STEPS[self._speed.currentIndex()][1]

    def set_session(self, session: Session) -> None:
        self._session = session

    def refresh(self, session: Session) -> None:
        obs = session.observation
        for bank, bar in enumerate(self._rod_bars):
            bar.setValue(int(obs.rod_position[bank] * 1000))
        recorder = getattr(session, "recorder", None)
        if recorder is not None:
            self._samples_label.setText(f"samples: {recorder.samples} (stride 1)")
        # Inputs are demand widgets: never overwritten by telemetry, except the
        # AR checkbox which the protection system can effectively disable.
        ar_on = bool(obs.ar_enabled)
        if self._ar_enabled.isChecked() != ar_on:
            with_blocked = self._ar_enabled.blockSignals(True)
            self._ar_enabled.setChecked(ar_on)
            self._ar_enabled.blockSignals(with_blocked)
