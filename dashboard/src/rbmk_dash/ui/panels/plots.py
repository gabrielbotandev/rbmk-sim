"""Live trend plots (pyqtgraph): power, reactivity breakdown, poisons, rods,
void/flow, and temperatures."""

from __future__ import annotations

import pyqtgraph as pg
from PySide6.QtWidgets import QCheckBox, QHBoxLayout, QLabel, QVBoxLayout, QWidget

from rbmk_dash.core import bindings
from rbmk_dash.core.session import Session
from rbmk_dash.ui.theme import PLOT_SERIES

#: Display-only conversion of absolute reactivity to beta units ("$").
#: Mirrors the kernel's textbook six-group beta total; never used by physics.
BETA_TOTAL = 0.006502


class PlotsPanel(QWidget):
    """3x2 grid of synced live plots."""

    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)

        header = QHBoxLayout()
        header.addWidget(QLabel("Trends"))
        header.addStretch(1)
        self._log_power = QCheckBox("log power axis")
        self._log_power.toggled.connect(self._on_log_toggled)
        header.addWidget(self._log_power)
        layout.addLayout(header)

        self._graphics = pg.GraphicsLayoutWidget()
        layout.addWidget(self._graphics, 1)

        # --- power -------------------------------------------------------
        self._power_plot = self._graphics.addPlot(row=0, col=0, title="Thermal power [MW]")
        self._power_curve = self._power_plot.plot(pen=pg.mkPen(PLOT_SERIES["power"], width=2))

        # --- reactivity ----------------------------------------------------
        rho = self._graphics.addPlot(row=0, col=1, title="Reactivity [β]")
        rho.addLegend(offset=(4, 4), labelTextSize="8pt")
        self._rho_curves = {
            name: rho.plot(
                pen=pg.mkPen(PLOT_SERIES[key], width=2 if key == "rho_total" else 1),
                name=label,
            )
            for name, key, label in (
                ("rho_total", "rho_total", "total"),
                ("rho_rods", "rho_rods", "rods"),
                ("rho_void", "rho_void", "void"),
                ("rho_doppler", "rho_doppler", "fuel T"),
                ("rho_xenon", "rho_xenon", "xenon"),
            )
        }
        rho.addLine(y=0.0, pen=pg.mkPen("#555555", style=pg.QtCore.Qt.PenStyle.DashLine))
        rho.addLine(y=1.0, pen=pg.mkPen("#7c2a22", style=pg.QtCore.Qt.PenStyle.DashLine))
        self._rho_plot = rho

        # --- poisons -------------------------------------------------------
        xe = self._graphics.addPlot(row=1, col=0, title="Poisons [relative to nominal eq.]")
        xe.addLegend(offset=(4, 4), labelTextSize="8pt")
        self._xenon_curve = xe.plot(pen=pg.mkPen(PLOT_SERIES["xenon"], width=2), name="xenon-135")
        self._iodine_curve = xe.plot(
            pen=pg.mkPen(PLOT_SERIES["iodine"], width=1), name="iodine-135"
        )

        # --- rods ----------------------------------------------------------
        rods = self._graphics.addPlot(row=1, col=1, title="Rod insertion [0=out, 1=in]")
        rods.addLegend(offset=(4, 4), labelTextSize="8pt")
        rods.setYRange(0.0, 1.0)
        rod_colors = PLOT_SERIES["rods"]
        self._rod_curves = [
            rods.plot(
                pen=pg.mkPen(rod_colors[b], width=1),
                name=bindings.BANK_NAMES[b],
            )
            for b in range(bindings.NUM_BANKS)
        ]

        # --- void / flow -----------------------------------------------------
        vf = self._graphics.addPlot(row=2, col=0, title="Core void / coolant flow [-]")
        vf.addLegend(offset=(4, 4), labelTextSize="8pt")
        self._void_curve = vf.plot(pen=pg.mkPen(PLOT_SERIES["void"], width=2), name="avg void")
        self._flow_curve = vf.plot(pen=pg.mkPen(PLOT_SERIES["flow"], width=1), name="flow")

        # --- temperatures ----------------------------------------------------
        temps = self._graphics.addPlot(row=2, col=1, title="Temperatures [°C]")
        temps.addLegend(offset=(4, 4), labelTextSize="8pt")
        self._fuel_curve = temps.plot(pen=pg.mkPen(PLOT_SERIES["fuel"], width=2), name="avg fuel")
        self._coolant_curve = temps.plot(
            pen=pg.mkPen(PLOT_SERIES["coolant"], width=1), name="coolant"
        )

        for plot in (self._power_plot, rho, xe, rods, vf, temps):
            plot.showGrid(x=True, y=True, alpha=0.15)
            plot.setLabel("bottom", "time [s]")

    def _on_log_toggled(self, checked: bool) -> None:
        self._power_plot.setLogMode(False, checked)

    def refresh(self, session: Session) -> None:
        h = session.history
        t = h.times()
        if t.size == 0:
            return

        self._power_curve.setData(t, h.series("power_mw"))
        for name, curve in self._rho_curves.items():
            curve.setData(t, h.series(name) / BETA_TOTAL)
        self._xenon_curve.setData(t, h.series("xenon_rel"))
        self._iodine_curve.setData(t, h.series("iodine"))
        for b, curve in enumerate(self._rod_curves):
            curve.setData(t, h.rod_series(b))
        self._void_curve.setData(t, h.series("avg_void_frac"))
        self._flow_curve.setData(t, h.series("flow_frac"))
        self._fuel_curve.setData(t, h.series("avg_fuel_temp_c"))
        self._coolant_curve.setData(t, h.series("coolant_temp_c"))
