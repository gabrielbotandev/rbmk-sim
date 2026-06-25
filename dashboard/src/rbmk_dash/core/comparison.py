"""Design-evolution comparison: the same scram transient under the 1986-style
rod design (graphite displacer "tip") versus the modified monotonic design.

Both runs are deterministic and identically configured except for the rod
design flag, so every difference in the traces is attributable to the worth
curve alone. Qualitative and educational only.
"""

from __future__ import annotations

from dataclasses import dataclass

import numpy as np

from rbmk_dash.core import bindings
from rbmk_dash.core.bindings import Config
from rbmk_dash.core.recorder import RecordingSession

#: Scenario phases (seconds) for the canonical comparison transient.
SETTLE_S = 10.0
SCRAM_AT_S = 10.0
FOLLOW_S = 40.0


def comparison_config(rod_design: int, initial_insertion: float = 0.05) -> Config:
    """The canonical 'primed core' configuration: rods nearly withdrawn."""
    cfg = bindings.default_config()
    cfg.rod_design = rod_design
    cfg.initial_manual_rod_insertion = initial_insertion
    cfg.ar_enabled = 0  # open loop: nothing fights the transient
    return cfg


@dataclass
class ComparisonTrace:
    """Recorded scram transient for one rod design."""

    design: int
    time_s: np.ndarray
    power_frac: np.ndarray
    rho_rods: np.ndarray
    rho_total: np.ndarray
    rod_position: np.ndarray  # manual bank A insertion
    scram_at_s: float

    @property
    def design_name(self) -> str:
        return "1986 (graphite displacer)" if self.design == bindings.ROD_DESIGN_1986 else (
            "modified (monotonic)"
        )

    @property
    def peak_power(self) -> float:
        after = self.time_s >= self.scram_at_s
        return float(self.power_frac[after].max())

    @property
    def peak_rho_rods(self) -> float:
        """Largest rod-bank reactivity reached during the scram (the lobe)."""
        after = self.time_s >= self.scram_at_s
        return float(self.rho_rods[after].max())

    @property
    def rho_rods_rise(self) -> float:
        """How much the rod reactivity ROSE after the scram command.

        Positive only when the displacer lobe momentarily adds reactivity;
        a monotonic design can never make this positive.
        """
        before = self.rho_rods[self.time_s < self.scram_at_s]
        reference = float(before[-1]) if before.size else float(self.rho_rods[0])
        return self.peak_rho_rods - reference

    @property
    def final_power(self) -> float:
        return float(self.power_frac[-1])


def run_scram_comparison(design: int, initial_insertion: float = 0.05) -> ComparisonTrace:
    """Runs the canonical transient for one design and returns its trace."""
    session = RecordingSession(
        comparison_config(design, initial_insertion),
        scenario=f"scram-comparison-{design}",
        sample_stride=1,
    )
    dt = session.dt_s
    session.advance(round(SETTLE_S / dt))
    session.apply("press_az5")
    session.advance(round(FOLLOW_S / dt))

    rec = session.recorder
    trace = ComparisonTrace(
        design=design,
        time_s=np.asarray(rec._scalars["time_s"]),
        power_frac=np.asarray(rec._scalars["power_frac"]),
        rho_rods=np.asarray(rec._scalars["rho_rods"]),
        rho_total=np.asarray(rec._scalars["rho_total"]),
        rod_position=np.asarray(rec._rods)[:, bindings.BANK_MANUAL_A],
        scram_at_s=SCRAM_AT_S,
    )
    session.close()
    return trace


def run_both_designs(initial_insertion: float = 0.05) -> tuple[ComparisonTrace, ComparisonTrace]:
    """Returns (1986 trace, modified trace) for identical conditions."""
    return (
        run_scram_comparison(bindings.ROD_DESIGN_1986, initial_insertion),
        run_scram_comparison(bindings.ROD_DESIGN_MODIFIED, initial_insertion),
    )


def worth_curves(samples: int = 201) -> dict[str, np.ndarray]:
    """Static bank worth curves for both designs (display only).

    Reconstructs the kernel's published curve shapes for one manual bank:
    smoothstep absorber plus, for the 1986 design, the displacer lobe.
    Values mirror kernel/include/rbmk/kernel/constants.hpp.
    """
    bank_worth = -0.020
    tip_worth = +0.0035
    x = np.linspace(0.0, 1.0, samples)
    absorber = x * x * (3.0 - 2.0 * x)
    displacer = (27.0 / 4.0) * x * (1.0 - x) ** 2
    return {
        "x": x,
        "w1986": bank_worth * absorber + tip_worth * displacer,
        "wmod": bank_worth * absorber,
    }
