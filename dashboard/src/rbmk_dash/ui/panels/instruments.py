"""Numeric instrumentation readouts (control-room style)."""

from __future__ import annotations

from PySide6.QtCore import Qt
from PySide6.QtWidgets import QGridLayout, QGroupBox, QLabel, QVBoxLayout, QWidget

from rbmk_dash.core import bindings
from rbmk_dash.core.session import Session
from rbmk_dash.ui.panels.plots import BETA_TOTAL


def _fmt_period(period_s: float) -> str:
    if abs(period_s) >= 9000.0:
        return "stable"
    return f"{period_s:+.1f} s"


class InstrumentsPanel(QWidget):
    """Grid of labelled readouts fed from the latest observation."""

    _ROWS: tuple[tuple[str, str], ...] = (
        ("power_pct", "Power [% nom]"),
        ("period", "Period"),
        ("rho_total", "Reactivity [β]"),
        ("orm", "Rod equivalent in"),
        ("xenon", "Xenon [rel eq.]"),
        ("void", "Avg void"),
        ("flow", "Flow [% nom]"),
        ("t_fuel", "Fuel T [°C]"),
        ("t_cool", "Coolant T [°C]"),
        ("det", "Detectors [%]"),
        ("time", "Sim time"),
    )

    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        outer = QVBoxLayout(self)
        outer.setContentsMargins(0, 0, 0, 0)

        box = QGroupBox("Instrumentation")
        grid = QGridLayout(box)
        grid.setVerticalSpacing(4)

        self._power_big = QLabel("---- MW")
        self._power_big.setProperty("role", "readoutBig")
        self._power_big.setAlignment(Qt.AlignmentFlag.AlignRight | Qt.AlignmentFlag.AlignVCenter)
        grid.addWidget(QLabel("Thermal power"), 0, 0)
        grid.addWidget(self._power_big, 0, 1)

        self._values: dict[str, QLabel] = {}
        for row, (key, label) in enumerate(self._ROWS, start=1):
            name = QLabel(label)
            value = QLabel("--")
            value.setProperty("role", "readoutValue")
            value.setAlignment(
                Qt.AlignmentFlag.AlignRight | Qt.AlignmentFlag.AlignVCenter
            )
            grid.addWidget(name, row, 0)
            grid.addWidget(value, row, 1)
            self._values[key] = value

        outer.addWidget(box)

    def refresh(self, session: Session) -> None:
        obs = session.observation
        v = self._values

        self._power_big.setText(f"{obs.power_mw:8.1f} MW")
        v["power_pct"].setText(f"{obs.power_frac * 100.0:7.2f} %")
        v["period"].setText(_fmt_period(obs.period_s))
        v["rho_total"].setText(f"{obs.rho_total / BETA_TOTAL:+7.3f} β")
        v["orm"].setText(f"{obs.inserted_rod_equivalent:5.2f} banks")
        v["xenon"].setText(f"{obs.xenon_rel:6.3f}")
        v["void"].setText(f"{obs.avg_void_frac * 100.0:5.1f} %")
        v["flow"].setText(f"{obs.flow_frac * 100.0:5.1f} %")
        v["t_fuel"].setText(f"{obs.avg_fuel_temp_c:7.1f}")
        v["t_cool"].setText(f"{obs.coolant_temp_c:7.1f}")
        dets = " ".join(
            f"{obs.detector_power_frac[k] * 100.0:.0f}" for k in range(bindings.NUM_DETECTORS)
        )
        v["det"].setText(dets)
        minutes, seconds = divmod(obs.time_s, 60.0)
        hours, minutes = divmod(int(minutes), 60)
        v["time"].setText(f"{hours:02d}:{minutes:02d}:{seconds:04.1f}")
