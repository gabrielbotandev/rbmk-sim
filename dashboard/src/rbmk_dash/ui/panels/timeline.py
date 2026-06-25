"""Accident timeline replay tab: event list, historical/model narrative,
replay controls, and full-history trends.

The historical narrative and the model's scripted actions are rendered as
visually distinct blocks so the educational boundary stays unmistakable.
"""

from __future__ import annotations

from PySide6.QtCore import Qt
from PySide6.QtWidgets import (
    QFileDialog,
    QHBoxLayout,
    QLabel,
    QListWidget,
    QListWidgetItem,
    QPushButton,
    QSplitter,
    QTextBrowser,
    QVBoxLayout,
    QWidget,
)

from rbmk_dash.core import bindings
from rbmk_dash.core.scenario import Scenario, ScenarioEvent, load_scenario, scenarios_dir
from rbmk_dash.core.timeline import TimelineRun
from rbmk_dash.ui.panels.plots import PlotsPanel
from rbmk_dash.ui.theme import COLORS

_BADGE_HIST = (
    f"<span style='background:{COLORS['surface_alt']};color:{COLORS['warn']};"
    "font-weight:bold;padding:1px 6px;border-radius:2px'>HISTORICAL</span>"
)
_BADGE_MODEL = (
    f"<span style='background:{COLORS['surface_alt']};color:{COLORS['info']};"
    "font-weight:bold;padding:1px 6px;border-radius:2px'>MODEL</span>"
)


def _format_offset(offset_s: float) -> str:
    hours, rem = divmod(int(offset_s), 3600)
    minutes, seconds = divmod(rem, 60)
    return f"T+{hours:02d}:{minutes:02d}:{seconds:02d}"


class TimelinePanel(QWidget):
    """Replay of a scenario timeline with explicit narrative separation."""

    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.run: TimelineRun | None = None
        self.scenario: Scenario | None = None

        outer = QVBoxLayout(self)
        outer.setContentsMargins(4, 4, 4, 4)

        self._disclaimer = QLabel(
            "Educational narrative (INSAG-7 summary level) driving a toy model — "
            "not a reconstruction. Historical statements and model actions are "
            "labelled separately."
        )
        self._disclaimer.setWordWrap(True)
        self._disclaimer.setStyleSheet(
            f"border: 1px solid {COLORS['warn']}; border-radius: 3px; padding: 6px;"
            f"color: {COLORS['warn']}; background: {COLORS['surface']};"
        )
        outer.addWidget(self._disclaimer)

        controls = QHBoxLayout()
        self._restart = QPushButton("Start / restart timeline")
        self._restart.clicked.connect(self.restart)
        controls.addWidget(self._restart)
        self._next = QPushButton("Advance to next event")
        self._next.clicked.connect(self._on_next_event)
        self._next.setEnabled(False)
        controls.addWidget(self._next)
        self._plus60 = QPushButton("Run 60 s")
        self._plus60.clicked.connect(self._on_plus_60)
        self._plus60.setEnabled(False)
        controls.addWidget(self._plus60)
        self._save = QPushButton("Save timeline log…")
        self._save.clicked.connect(self._on_save)
        self._save.setEnabled(False)
        controls.addWidget(self._save)
        controls.addStretch(1)
        self._status = QLabel("no timeline loaded")
        controls.addWidget(self._status)
        outer.addLayout(controls)

        split = QSplitter(Qt.Orientation.Vertical)

        top = QSplitter(Qt.Orientation.Horizontal)
        self._events = QListWidget()
        self._events.currentRowChanged.connect(self._on_select_event)
        top.addWidget(self._events)
        self._narrative = QTextBrowser()
        self._narrative.setOpenExternalLinks(False)
        top.addWidget(self._narrative)
        top.setStretchFactor(1, 1)
        split.addWidget(top)

        self.plots = PlotsPanel()
        split.addWidget(self.plots)
        split.setStretchFactor(1, 1)
        outer.addWidget(split, 1)

    # ------------------------------------------------------------------ loading
    def restart(self) -> None:
        if self.run is not None:
            self.run.close()
            self.run = None
        if self.scenario is None:
            self.scenario = load_scenario(scenarios_dir() / "chernobyl_timeline.json")
        self.run = TimelineRun(self.scenario)
        self._populate_events()
        self._next.setEnabled(True)
        self._plus60.setEnabled(True)
        self._save.setEnabled(True)
        self.refresh()

    def _populate_events(self) -> None:
        assert self.scenario is not None
        self._events.clear()
        for event in self.scenario.events:
            tag = "H" if event.historical else "M"
            item = QListWidgetItem(f"{_format_offset(event.offset_s)}  [{tag}] {event.label}")
            item.setToolTip(event.label)
            self._events.addItem(item)

    # ------------------------------------------------------------------ actions
    def _on_next_event(self) -> None:
        if self.run is None or self.run.finished:
            return
        self.run.advance_to_next_event()
        self.refresh()

    def _on_plus_60(self) -> None:
        if self.run is None:
            return
        self.run.advance_seconds(60.0)
        self.refresh()

    def _on_save(self) -> None:
        if self.run is None:
            return
        default_dir = bindings.repo_root() / "runs"
        path, _ = QFileDialog.getSaveFileName(
            self,
            "Save timeline run log",
            str(default_dir / "chernobyl_timeline.h5"),
            "HDF5 run logs (*.h5)",
        )
        if path:
            saved = self.run.session.save(path)
            self._status.setText(f"saved {saved.name}")

    def _on_select_event(self, row: int) -> None:
        if self.scenario is None or row < 0 or row >= len(self.scenario.events):
            return
        self._show_event(self.scenario.events[row])

    # ------------------------------------------------------------------ display
    def _show_event(self, event: ScenarioEvent) -> None:
        applied = self.run is not None and any(
            a.event.event_id == event.event_id for a in self.run.applied
        )
        state = "applied to the model" if applied else "not yet reached"
        actions = (
            "<br>".join(
                f"<code>{action.command}{tuple(action.args)!r}</code>"
                for action in event.actions
            )
            or "<i>none</i>"
        )
        html = f"""
        <h3>{event.label}</h3>
        <p style='color:{COLORS["text_dim"]}'>{_format_offset(event.offset_s)}
        — {state}</p>
        <p>{_BADGE_HIST if event.historical else _BADGE_MODEL} {event.narrative}</p>
        <p>{_BADGE_MODEL} {event.model_note}</p>
        <p style='color:{COLORS["text_dim"]}'>Scripted actions:<br>{actions}</p>
        """
        self._narrative.setHtml(html)

    def refresh(self) -> None:
        if self.run is None:
            return
        for row in range(self._events.count()):
            item = self._events.item(row)
            applied = row < len(self.run.applied)
            font = item.font()
            font.setBold(applied)
            item.setFont(font)
        current_row = max(0, len(self.run.applied) - 1)
        self._events.setCurrentRow(current_row)

        self.plots.refresh(self.run.session)

        obs = self.run.session.observation
        state = bindings.RPS_STATE_NAMES.get(int(obs.rps_state), "?")
        upcoming = self.run.next_event
        nxt = f" | next: {upcoming.label}" if upcoming is not None else " | timeline complete"
        self._status.setText(
            f"t = {obs.time_s:9.0f} s | P = {obs.power_frac * 100.0:7.2f} % | RPS {state}{nxt}"
        )
