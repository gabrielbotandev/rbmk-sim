"""Deterministic HDF5 run logs: recording, loading, replay, verification.

A run is fully described by (config, ordered command log, total steps); the
recorded timeseries is a stride-sampled trace of the observations. Replaying
the config and commands on the same binary must reproduce the trace exactly
(bit-for-bit); ``verify_run`` enforces that property.

File schema (format_version 1):
    /meta            attrs: format_version, model_version, abi_version,
                     dash_version, scenario, created_utc, dt_s, sample_stride,
                     total_steps
    /config          attrs: every rbmk_config field
    /timeseries/*    1-D float64/uint datasets, one per signal, equal length
    /commands/*      step (uint64), name (str), args_json (str)
    /events/*        step (uint64), kind (str), text (str) - timeline runs
"""

from __future__ import annotations

import json
from dataclasses import dataclass, field
from datetime import UTC, datetime
from pathlib import Path
from typing import Any

import h5py
import numpy as np

from rbmk_dash import __version__ as DASH_VERSION
from rbmk_dash.core import bindings
from rbmk_dash.core.bindings import Config, Observation
from rbmk_dash.core.session import HISTORY_FIELDS, CommandRecord, Session

FORMAT_VERSION = 1

#: Scalar signals recorded per sample (subset of the observation).
SCALAR_SIGNALS: tuple[str, ...] = ("time_s", *HISTORY_FIELDS)

#: Integer signals recorded per sample.
INT_SIGNALS: tuple[str, ...] = (
    "step_count",
    "scram_latched",
    "rps_state",
    "rps_scram_command",
    "rps_alarms",
    "rps_trip_active",
    "rps_trip_latched",
    "validity_exceeded",
)


@dataclass
class EventRecord:
    """Annotated moment (used by scenario/timeline runs)."""

    step: int
    kind: str
    text: str


@dataclass
class RunData:
    """In-memory image of a run log."""

    meta: dict[str, Any]
    config: Config
    commands: list[CommandRecord]
    events: list[EventRecord]
    timeseries: dict[str, np.ndarray]

    @property
    def total_steps(self) -> int:
        return int(self.meta["total_steps"])

    @property
    def sample_stride(self) -> int:
        return int(self.meta["sample_stride"])


@dataclass
class RunRecorder:
    """Accumulates stride-sampled observations and writes the HDF5 log."""

    scenario: str = "interactive"
    sample_stride: int = 1
    _scalars: dict[str, list[float]] = field(default_factory=dict)
    _ints: dict[str, list[int]] = field(default_factory=dict)
    _rods: list[list[float]] = field(default_factory=list)
    _detectors: list[list[float]] = field(default_factory=list)
    _channel_void: list[list[float]] = field(default_factory=list)
    _channel_power: list[list[float]] = field(default_factory=list)
    events: list[EventRecord] = field(default_factory=list)
    _last_recorded_step: int = -1

    def __post_init__(self) -> None:
        self.sample_stride = max(1, int(self.sample_stride))
        self._scalars = {name: [] for name in SCALAR_SIGNALS}
        self._ints = {name: [] for name in INT_SIGNALS}

    # ------------------------------------------------------------------ sampling
    @property
    def samples(self) -> int:
        return len(self._scalars["time_s"])

    def record(self, obs: Observation) -> None:
        """Appends one sample (deduplicated by step index)."""
        step = int(obs.step_count)
        if step == self._last_recorded_step:
            return
        self._last_recorded_step = step
        for name in SCALAR_SIGNALS:
            self._scalars[name].append(float(getattr(obs, name)))
        for name in INT_SIGNALS:
            self._ints[name].append(int(getattr(obs, name)))
        self._rods.append([obs.rod_position[b] for b in range(bindings.NUM_BANKS)])
        self._detectors.append(
            [obs.detector_power_frac[k] for k in range(bindings.NUM_DETECTORS)]
        )
        nch = int(obs.num_channels)
        self._channel_void.append([obs.channel_void[i] for i in range(nch)])
        self._channel_power.append([obs.channel_power[i] for i in range(nch)])

    def add_event(self, step: int, kind: str, text: str) -> None:
        self.events.append(EventRecord(step=step, kind=kind, text=text))

    # ------------------------------------------------------------------ writing
    def save(self, path: str | Path, session: Session) -> Path:
        """Writes the complete run log next to the session that produced it."""
        path = Path(path)
        path.parent.mkdir(parents=True, exist_ok=True)
        obs = session.observation
        cfg = session.sim.config
        str_dtype = h5py.string_dtype(encoding="utf-8")

        with h5py.File(path, "w") as f:
            meta = f.create_group("meta")
            meta.attrs["format_version"] = FORMAT_VERSION
            meta.attrs["model_version"] = bindings.model_version()
            meta.attrs["abi_version"] = bindings.ABI_VERSION
            meta.attrs["dash_version"] = DASH_VERSION
            meta.attrs["scenario"] = self.scenario
            meta.attrs["created_utc"] = datetime.now(UTC).isoformat()
            meta.attrs["dt_s"] = float(cfg.dt_s)
            meta.attrs["sample_stride"] = self.sample_stride
            meta.attrs["total_steps"] = int(obs.step_count)
            meta.attrs["disclaimer"] = (
                "Educational toy model output. Not suitable for any operational, "
                "predictive, or safety-related use."
            )

            cfg_group = f.create_group("config")
            for name, _ in Config._fields_:
                cfg_group.attrs[name] = getattr(cfg, name)

            ts = f.create_group("timeseries")
            for name, values in self._scalars.items():
                ts.create_dataset(name, data=np.asarray(values, dtype=np.float64))
            for name, ivalues in self._ints.items():
                ts.create_dataset(name, data=np.asarray(ivalues, dtype=np.uint64))
            ts.create_dataset("rod_position", data=np.asarray(self._rods, dtype=np.float64))
            ts.create_dataset(
                "detector_power_frac", data=np.asarray(self._detectors, dtype=np.float64)
            )
            ts.create_dataset(
                "channel_void", data=np.asarray(self._channel_void, dtype=np.float64)
            )
            ts.create_dataset(
                "channel_power", data=np.asarray(self._channel_power, dtype=np.float64)
            )

            cmd = f.create_group("commands")
            cmd.create_dataset(
                "step", data=np.asarray([c.step for c in session.commands], dtype=np.uint64)
            )
            cmd.create_dataset(
                "name", data=[c.name for c in session.commands], dtype=str_dtype
            )
            cmd.create_dataset(
                "args_json",
                data=[json.dumps(list(c.args)) for c in session.commands],
                dtype=str_dtype,
            )

            ev = f.create_group("events")
            ev.create_dataset(
                "step", data=np.asarray([e.step for e in self.events], dtype=np.uint64)
            )
            ev.create_dataset("kind", data=[e.kind for e in self.events], dtype=str_dtype)
            ev.create_dataset("text", data=[e.text for e in self.events], dtype=str_dtype)
        return path


# ---------------------------------------------------------------------- loading
def load_run(path: str | Path) -> RunData:
    with h5py.File(Path(path), "r") as f:
        meta = dict(f["meta"].attrs)
        if int(meta.get("format_version", -1)) != FORMAT_VERSION:
            raise ValueError(f"unsupported log format: {meta.get('format_version')!r}")

        config = Config()
        for name, _ in Config._fields_:
            setattr(config, name, f["config"].attrs[name])

        commands = [
            CommandRecord(step=int(s), name=str(n), args=tuple(json.loads(str(a))))
            for s, n, a in zip(
                f["commands/step"][()],
                f["commands/name"].asstr()[()],
                f["commands/args_json"].asstr()[()],
                strict=True,
            )
        ]
        events = [
            EventRecord(step=int(s), kind=str(k), text=str(t))
            for s, k, t in zip(
                f["events/step"][()],
                f["events/kind"].asstr()[()],
                f["events/text"].asstr()[()],
                strict=True,
            )
        ]
        timeseries = {name: f["timeseries"][name][()] for name in f["timeseries"]}
    return RunData(
        meta=meta, config=config, commands=commands, events=events, timeseries=timeseries
    )


# ---------------------------------------------------------------------- replay
class RecordingSession(Session):
    """A session that records every stride-th step into a RunRecorder."""

    def __init__(
        self,
        config: Config | None = None,
        scenario: str = "interactive",
        sample_stride: int = 1,
        history_maxlen: int = 36000,
    ) -> None:
        self.recorder = RunRecorder(scenario=scenario, sample_stride=sample_stride)
        self._last_history_step = 0
        super().__init__(config, history_maxlen=history_maxlen)
        self.recorder.record(self.observation)  # t = 0 sample

    def advance(self, n_steps: int) -> Observation:
        stride = self.recorder.sample_stride
        remaining = int(n_steps)
        while remaining > 0:
            current = self.step_count
            to_boundary = stride - (current % stride)
            chunk = min(remaining, to_boundary)
            self.sim.step(chunk)
            obs = self.sim.observe(self._obs)
            if int(obs.step_count) % stride == 0:
                self.recorder.record(obs)
                # Mirror every recorded sample into the plot history so the
                # UI sees full-resolution trends even across fast-forwards.
                if int(obs.step_count) != self._last_history_step:
                    self.history.append(obs)
                    self._last_history_step = int(obs.step_count)
            remaining -= chunk
        obs = self.sim.observe(self._obs)
        if int(obs.step_count) != self._last_history_step:
            self.history.append(obs)
            self._last_history_step = int(obs.step_count)
        return obs

    def save(self, path: str | Path) -> Path:
        self.sample()
        return self.recorder.save(path, self)


def replay_run(data: RunData) -> RecordingSession:
    """Re-executes a run from its config and command log, sampling at the
    recorded stride so traces are directly comparable."""
    config = Config()
    for name, _ in Config._fields_:
        setattr(config, name, getattr(data.config, name))
    session = RecordingSession(
        config,
        scenario=str(data.meta.get("scenario", "replay")),
        sample_stride=data.sample_stride,
    )
    for record in sorted(data.commands, key=lambda r: r.step):
        if record.step > session.step_count:
            session.advance(record.step - session.step_count)
        session.apply(record.name, *record.args)
    if data.total_steps > session.step_count:
        session.advance(data.total_steps - session.step_count)
    return session


def verify_run(path: str | Path) -> dict[str, float]:
    """Replays a log and returns the max abs deviation per signal.

    Deterministic replay on the same binary must give exactly 0.0 everywhere;
    any nonzero entry means the log and the model disagree.
    """
    data = load_run(path)
    session = replay_run(data)
    rec = session.recorder

    report: dict[str, float] = {}
    for name in SCALAR_SIGNALS:
        recorded = data.timeseries[name]
        replayed = np.asarray(rec._scalars[name], dtype=np.float64)
        if recorded.shape != replayed.shape:
            report[name] = float("inf")
        else:
            report[name] = float(np.max(np.abs(recorded - replayed))) if recorded.size else 0.0
    for name in INT_SIGNALS:
        recorded = data.timeseries[name].astype(np.int64)
        replayed = np.asarray(rec._ints[name], dtype=np.int64)
        if recorded.shape != replayed.shape:
            report[name] = float("inf")
        else:
            report[name] = float(np.max(np.abs(recorded - replayed))) if recorded.size else 0.0
    session.close()
    return report
