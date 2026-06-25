"""Simulation session: drives one simulator, keeps plot history, and records
every operator command with its step index so a run can be replayed exactly.

The command log is the backbone of deterministic replay: a run is fully
described by (config, ordered command log, number of steps).
"""

from __future__ import annotations

from collections import deque
from dataclasses import dataclass, field
from typing import Any

import numpy as np

from rbmk_dash.core import bindings
from rbmk_dash.core.bindings import Config, Observation, Simulator

#: Scalar observation fields kept as plot history (name -> attribute).
HISTORY_FIELDS: tuple[str, ...] = (
    "power_frac",
    "power_mw",
    "period_s",
    "rho_total",
    "rho_rods",
    "rho_void",
    "rho_doppler",
    "rho_xenon",
    "xenon_rel",
    "iodine",
    "avg_void_frac",
    "avg_fuel_temp_c",
    "coolant_temp_c",
    "flow_frac",
    "flow_command_frac",
    "inserted_rod_equivalent",
)

#: Commands accepted by Session.apply (name -> Simulator method).
COMMANDS: dict[str, str] = {
    "set_rod_target": "set_rod_target",
    "set_pump_flow": "set_pump_flow",
    "set_power_setpoint": "set_power_setpoint",
    "set_ar_enabled": "set_ar_enabled",
    "press_az5": "press_az5",
    "request_reset": "request_reset",
    "set_detectors_valid": "set_detectors_valid",
}


@dataclass(frozen=True)
class CommandRecord:
    """One operator action, pinned to the simulation step where it applied."""

    step: int
    name: str
    args: tuple[Any, ...]


@dataclass
class SessionHistory:
    """Bounded plot history (ring buffers)."""

    maxlen: int = 36000
    time_s: deque[float] = field(default_factory=deque)
    scalars: dict[str, deque[float]] = field(default_factory=dict)
    rod_position: list[deque[float]] = field(default_factory=list)
    detector: list[deque[float]] = field(default_factory=list)
    rps_state: deque[int] = field(default_factory=deque)

    def __post_init__(self) -> None:
        self.time_s = deque(maxlen=self.maxlen)
        self.scalars = {name: deque(maxlen=self.maxlen) for name in HISTORY_FIELDS}
        self.rod_position = [deque(maxlen=self.maxlen) for _ in range(bindings.NUM_BANKS)]
        self.detector = [deque(maxlen=self.maxlen) for _ in range(bindings.NUM_DETECTORS)]
        self.rps_state = deque(maxlen=self.maxlen)

    def append(self, obs: Observation) -> None:
        self.time_s.append(obs.time_s)
        for name in HISTORY_FIELDS:
            self.scalars[name].append(getattr(obs, name))
        for b in range(bindings.NUM_BANKS):
            self.rod_position[b].append(obs.rod_position[b])
        for k in range(bindings.NUM_DETECTORS):
            self.detector[k].append(obs.detector_power_frac[k])
        self.rps_state.append(int(obs.rps_state))

    def clear(self) -> None:
        self.time_s.clear()
        for dq in self.scalars.values():
            dq.clear()
        for dq in self.rod_position:
            dq.clear()
        for dq in self.detector:
            dq.clear()
        self.rps_state.clear()

    def times(self) -> np.ndarray:
        return np.fromiter(self.time_s, dtype=np.float64, count=len(self.time_s))

    def series(self, name: str) -> np.ndarray:
        dq = self.scalars[name]
        return np.fromiter(dq, dtype=np.float64, count=len(dq))

    def rod_series(self, bank: int) -> np.ndarray:
        dq = self.rod_position[bank]
        return np.fromiter(dq, dtype=np.float64, count=len(dq))


class Session:
    """One live simulation run: simulator + history + command log."""

    def __init__(self, config: Config | None = None, history_maxlen: int = 36000) -> None:
        self.sim = Simulator(config)
        self.history = SessionHistory(maxlen=history_maxlen)
        self.commands: list[CommandRecord] = []
        self._obs = Observation()
        self.sample()

    # -- introspection -------------------------------------------------------
    @property
    def observation(self) -> Observation:
        """Latest sampled observation (refreshed by sample())."""
        return self._obs

    @property
    def step_count(self) -> int:
        return int(self._obs.step_count)

    @property
    def dt_s(self) -> float:
        return float(self.sim.config.dt_s)

    # -- control -------------------------------------------------------------
    def apply(self, name: str, *args: Any) -> None:
        """Applies an operator command and records it at the current step."""
        method = COMMANDS.get(name)
        if method is None:
            raise ValueError(f"unknown command: {name!r}")
        current = self.sim.observe(self._obs).step_count
        self.commands.append(CommandRecord(step=int(current), name=name, args=tuple(args)))
        getattr(self.sim, method)(*args)

    # -- time ----------------------------------------------------------------
    def advance(self, n_steps: int) -> Observation:
        """Advances n steps and samples once at the end."""
        if n_steps > 0:
            self.sim.step(n_steps)
        return self.sample()

    def sample(self) -> Observation:
        obs = self.sim.observe(self._obs)
        self.history.append(obs)
        return obs

    def close(self) -> None:
        self.sim.close()
