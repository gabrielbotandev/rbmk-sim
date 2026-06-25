"""Design-evolution comparison tab: side-by-side scram transients for the
1986-style and modified rod designs, plus the static worth curves and a
plain-language explanation."""

from __future__ import annotations

import pyqtgraph as pg
from PySide6.QtWidgets import (
    QDoubleSpinBox,
    QHBoxLayout,
    QLabel,
    QPushButton,
    QTextBrowser,
    QVBoxLayout,
    QWidget,
)

from rbmk_dash.core.comparison import ComparisonTrace, run_both_designs, worth_curves
from rbmk_dash.ui.panels.plots import BETA_TOTAL
from rbmk_dash.ui.theme import COLORS

_COLOR_1986 = "#cf6a3f"
_COLOR_MOD = "#3f9e58"

_EXPLANATION = f"""
<h3>Why the same button gives two different shutdowns</h3>
<p>Both runs start from an identical, deliberately fragile state: manual banks almost
fully withdrawn, regulator off, then AZ-5 at t = 10&nbsp;s. The <b>only</b> difference
is the shape of the rod worth curve.</p>
<p><span style='color:{_COLOR_1986};font-weight:bold'>1986-style rod</span> — a graphite
displacer hangs below the absorber. Inserting from the fully withdrawn position first
pushes the displacer through the lower core, <i>adding</i> reactivity (the positive lobe
of the worth curve) before the absorber arrives. From a nearly withdrawn, void-prone
state the scram therefore begins with a power <i>rise</i> — the "positive scram" effect
INSAG-7 identifies at Chernobyl.</p>
<p><span style='color:{_COLOR_MOD};font-weight:bold'>Modified rod</span> — the
post-accident changes (re-dimensioned displacers/absorbers, higher minimum insertion,
more rods, faster scram in the real fixes) make the worth curve monotonically negative
in this model: every centimetre of travel removes reactivity, so power falls from the
first instant.</p>
<p style='color:{COLORS["text_dim"]}'>Toy model, qualitative only: curve shapes and
magnitudes are illustrative, not engineering data. The initial insertion slider changes
how "primed" the core is — at deeper initial insertions the 1986 lobe has already been
passed and the two designs behave almost identically.</p>
"""


class ComparisonPanel(QWidget):
    """Runs both designs through the canonical scram and plots them together."""

    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        outer = QVBoxLayout(self)
        outer.setContentsMargins(4, 4, 4, 4)

        controls = QHBoxLayout()
        self._run = QPushButton("Run comparison scram")
        self._run.clicked.connect(self.run_comparison)
        controls.addWidget(self._run)
        controls.addWidget(QLabel("Initial manual insertion:"))
        self._insertion = QDoubleSpinBox()
        self._insertion.setRange(0.0, 0.6)
        self._insertion.setSingleStep(0.05)
        self._insertion.setValue(0.05)
        self._insertion.setDecimals(2)
        controls.addWidget(self._insertion)
        controls.addStretch(1)
        self._summary = QLabel("not yet run")
        controls.addWidget(self._summary)
        outer.addLayout(controls)

        body = QHBoxLayout()

        self._graphics = pg.GraphicsLayoutWidget()
        power = self._graphics.addPlot(row=0, col=0, title="Power after AZ-5 [× nominal]")
        power.addLegend(offset=(4, 4), labelTextSize="8pt")
        self._power_1986 = power.plot(pen=pg.mkPen(_COLOR_1986, width=2), name="1986 design")
        self._power_mod = power.plot(pen=pg.mkPen(_COLOR_MOD, width=2), name="modified design")
        self._power_plot = power

        rho = self._graphics.addPlot(row=1, col=0, title="Rod-bank reactivity [β]")
        rho.addLegend(offset=(4, 4), labelTextSize="8pt")
        self._rho_1986 = rho.plot(pen=pg.mkPen(_COLOR_1986, width=2), name="1986 design")
        self._rho_mod = rho.plot(pen=pg.mkPen(_COLOR_MOD, width=2), name="modified design")
        rho.addLine(y=0.0, pen=pg.mkPen("#555555", style=pg.QtCore.Qt.PenStyle.DashLine))

        worth = self._graphics.addPlot(row=2, col=0, title="Static bank worth curve [β]")
        worth.addLegend(offset=(4, 4), labelTextSize="8pt")
        worth.setLabel("bottom", "insertion fraction")
        self._worth_1986 = worth.plot(pen=pg.mkPen(_COLOR_1986, width=2), name="1986 design")
        self._worth_mod = worth.plot(pen=pg.mkPen(_COLOR_MOD, width=2), name="modified design")
        worth.addLine(y=0.0, pen=pg.mkPen("#555555", style=pg.QtCore.Qt.PenStyle.DashLine))

        for plot in (power, rho):
            plot.showGrid(x=True, y=True, alpha=0.15)
            plot.setLabel("bottom", "time [s]")
        worth.showGrid(x=True, y=True, alpha=0.15)
        body.addWidget(self._graphics, 1)

        explanation = QTextBrowser()
        explanation.setHtml(_EXPLANATION)
        explanation.setFixedWidth(380)
        body.addWidget(explanation)

        outer.addLayout(body, 1)
        self._plot_static_curves()

    def _plot_static_curves(self) -> None:
        curves = worth_curves()
        self._worth_1986.setData(curves["x"], curves["w1986"] / BETA_TOTAL)
        self._worth_mod.setData(curves["x"], curves["wmod"] / BETA_TOTAL)

    def run_comparison(self) -> None:
        insertion = float(self._insertion.value())
        trace_1986, trace_mod = run_both_designs(insertion)
        self._show(trace_1986, trace_mod)

    def _show(self, t86: ComparisonTrace, tmod: ComparisonTrace) -> None:
        self._power_1986.setData(t86.time_s, t86.power_frac)
        self._power_mod.setData(tmod.time_s, tmod.power_frac)
        self._rho_1986.setData(t86.time_s, t86.rho_rods / BETA_TOTAL)
        self._rho_mod.setData(tmod.time_s, tmod.rho_rods / BETA_TOTAL)
        self._summary.setText(
            f"peak power — 1986: {t86.peak_power * 100.0:.1f}% | "
            f"modified: {tmod.peak_power * 100.0:.1f}%"
        )
