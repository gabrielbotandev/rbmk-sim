"""Design-evolution comparison: the flagship qualitative regression."""

from __future__ import annotations

import numpy as np
import pytest

from rbmk_dash.core import bindings
from rbmk_dash.core.comparison import run_both_designs, run_scram_comparison, worth_curves


@pytest.fixture(scope="module")
def traces(lib):
    return run_both_designs(initial_insertion=0.05)


class TestScramComparison:
    def test_1986_design_shows_positive_excursion(self, traces) -> None:
        t86, tmod = traces
        assert t86.peak_power > 1.02          # visible bump above steady power
        assert t86.rho_rods_rise > 0.0        # the displacer lobe added reactivity
        assert t86.peak_rho_rods > tmod.peak_rho_rods

    def test_modified_design_shuts_down_monotonically(self, traces) -> None:
        _, tmod = traces
        assert tmod.peak_power <= 1.002       # no bump beyond numerical noise
        assert tmod.rho_rods_rise <= 1e-12    # rods never added reactivity

    def test_both_designs_reach_shutdown(self, traces) -> None:
        t86, tmod = traces
        assert t86.final_power < 0.1
        assert tmod.final_power < 0.1

    def test_difference_vanishes_at_deep_insertion(self, lib) -> None:
        # With rods starting deep, the displacer lobe is already behind them:
        # both designs then shut down without any positive rod reactivity.
        t86 = run_scram_comparison(bindings.ROD_DESIGN_1986, initial_insertion=0.5)
        tmod = run_scram_comparison(bindings.ROD_DESIGN_MODIFIED, initial_insertion=0.5)
        assert t86.peak_power <= 1.002
        assert tmod.peak_power <= 1.002

    def test_comparison_is_deterministic(self, lib) -> None:
        a = run_scram_comparison(bindings.ROD_DESIGN_1986, initial_insertion=0.05)
        b = run_scram_comparison(bindings.ROD_DESIGN_1986, initial_insertion=0.05)
        assert np.array_equal(a.power_frac, b.power_frac)
        assert np.array_equal(a.rho_rods, b.rho_rods)


class TestWorthCurves:
    def test_curve_shapes(self) -> None:
        curves = worth_curves()
        w86, wmod = curves["w1986"], curves["wmod"]
        assert w86[0] == 0.0 and wmod[0] == 0.0
        assert w86.max() > 0.0                # the positive lobe exists
        assert wmod.max() <= 0.0              # monotonic design never positive
        assert w86[-1] == pytest.approx(wmod[-1])  # same fully-inserted worth
        assert np.all(np.diff(wmod) <= 1e-12)      # monotonically decreasing
