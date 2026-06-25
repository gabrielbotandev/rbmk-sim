"""Scenario files: versioned JSON describing an initial configuration plus a
list of timed, annotated events.

Every event separates the HISTORICAL narrative (what is documented to have
happened, at INSAG-7 summary level) from the MODEL actions (what the toy
simulator is scripted to do). The UI keeps that distinction visible at all
times; conflating the two would defeat the educational purpose.
"""

from __future__ import annotations

import json
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

from rbmk_dash.core import bindings
from rbmk_dash.core.bindings import Config
from rbmk_dash.core.session import COMMANDS

SCHEMA_VERSION = 1

_ROD_DESIGNS = {
    "1986": bindings.ROD_DESIGN_1986,
    "modified": bindings.ROD_DESIGN_MODIFIED,
}


@dataclass(frozen=True)
class ScenarioAction:
    """One scripted model command."""

    command: str
    args: tuple[Any, ...] = ()


@dataclass(frozen=True)
class ScenarioEvent:
    """A timed point in the scenario with narrative and optional actions."""

    event_id: str
    offset_s: float
    label: str
    narrative: str
    historical: bool
    model_note: str = ""
    actions: tuple[ScenarioAction, ...] = ()


@dataclass
class Scenario:
    name: str
    title: str
    description: str
    sources: list[str]
    disclaimer: str
    config: Config
    events: list[ScenarioEvent] = field(default_factory=list)

    @property
    def duration_s(self) -> float:
        return self.events[-1].offset_s if self.events else 0.0


def scenarios_dir() -> Path:
    return bindings.repo_root() / "scenarios"


def _parse_config(raw: dict[str, Any]) -> Config:
    cfg = bindings.default_config()
    if "rod_design" in raw:
        design = str(raw["rod_design"])
        if design not in _ROD_DESIGNS:
            raise ValueError(f"unknown rod_design: {design!r}")
        cfg.rod_design = _ROD_DESIGNS[design]
    for key in (
        "num_channels",
        "dt_s",
        "initial_power_frac",
        "initial_manual_rod_insertion",
        "start_at_xenon_equilibrium",
        "ar_enabled",
        "detector_noise",
        "noise_seed",
    ):
        if key in raw:
            setattr(cfg, key, raw[key])
    return cfg


def _parse_event(raw: dict[str, Any], index: int) -> ScenarioEvent:
    for required in ("id", "offset_s", "label", "narrative"):
        if required not in raw:
            raise ValueError(f"event #{index}: missing required key {required!r}")
    actions = []
    for action_raw in raw.get("actions", []):
        command = action_raw["command"]
        if command not in COMMANDS:
            raise ValueError(f"event {raw['id']!r}: unknown command {command!r}")
        actions.append(ScenarioAction(command=command, args=tuple(action_raw.get("args", []))))
    return ScenarioEvent(
        event_id=str(raw["id"]),
        offset_s=float(raw["offset_s"]),
        label=str(raw["label"]),
        narrative=str(raw["narrative"]),
        historical=bool(raw.get("historical", False)),
        model_note=str(raw.get("model_note", "")),
        actions=tuple(actions),
    )


def load_scenario(path: str | Path) -> Scenario:
    raw = json.loads(Path(path).read_text(encoding="utf-8"))
    if int(raw.get("schema_version", -1)) != SCHEMA_VERSION:
        raise ValueError(f"unsupported scenario schema: {raw.get('schema_version')!r}")

    events = [_parse_event(e, i) for i, e in enumerate(raw.get("events", []))]
    offsets = [e.offset_s for e in events]
    if offsets != sorted(offsets):
        raise ValueError("scenario events must be sorted by offset_s")

    return Scenario(
        name=str(raw["name"]),
        title=str(raw["title"]),
        description=str(raw.get("description", "")),
        sources=[str(s) for s in raw.get("sources", [])],
        disclaimer=str(raw.get("disclaimer", "")),
        config=_parse_config(raw.get("config", {})),
        events=events,
    )
