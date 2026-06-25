"""Protection-system annunciator: state banner, alarm/trip tiles, scram and
reset indicators."""

from __future__ import annotations

from PySide6.QtCore import Qt
from PySide6.QtWidgets import QGridLayout, QGroupBox, QLabel, QVBoxLayout, QWidget

from rbmk_dash.core import bindings
from rbmk_dash.core.session import Session
from rbmk_dash.ui.theme import repolish

_STATE_PROPS = {
    bindings.RPS_NORMAL: ("normal", "RPS: NORMAL"),
    bindings.RPS_ALARM: ("alarm", "RPS: ALARM"),
    bindings.RPS_TRIPPED: ("tripped", "RPS: TRIPPED — SCRAM"),
    bindings.RPS_SAFE_SHUTDOWN: ("shutdown", "RPS: SAFE SHUTDOWN"),
}


class AnnunciatorPanel(QWidget):
    """Qualitative mirror of the protection system's outputs."""

    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        outer = QVBoxLayout(self)
        outer.setContentsMargins(0, 0, 0, 0)

        box = QGroupBox("Protection system")
        layout = QVBoxLayout(box)

        self._banner = QLabel("RPS: --")
        self._banner.setProperty("role", "stateBanner")
        self._banner.setProperty("state", "normal")
        self._banner.setAlignment(Qt.AlignmentFlag.AlignCenter)
        layout.addWidget(self._banner)

        grid = QGridLayout()
        grid.setSpacing(4)
        self._tiles: dict[int, QLabel] = {}
        for index, (bit, name) in enumerate(sorted(bindings.COND_NAMES.items())):
            tile = QLabel(name)
            tile.setProperty("role", "annTile")
            tile.setProperty("level", "off")
            tile.setAlignment(Qt.AlignmentFlag.AlignCenter)
            grid.addWidget(tile, index // 2, index % 2)
            self._tiles[bit] = tile
        layout.addLayout(grid)

        self._scram = QLabel("scram command: idle")
        self._scram.setProperty("role", "readoutValue")
        self._scram.setAlignment(Qt.AlignmentFlag.AlignCenter)
        layout.addWidget(self._scram)

        self._reset_note = QLabel("")
        self._reset_note.setAlignment(Qt.AlignmentFlag.AlignCenter)
        layout.addWidget(self._reset_note)

        outer.addWidget(box)

    def refresh(self, session: Session) -> None:
        obs = session.observation

        state_key, banner_text = _STATE_PROPS.get(
            int(obs.rps_state), ("tripped", "RPS: UNKNOWN")
        )
        if self._banner.property("state") != state_key or self._banner.text() != banner_text:
            self._banner.setText(banner_text)
            self._banner.setProperty("state", state_key)
            repolish(self._banner)

        for bit, tile in self._tiles.items():
            if obs.rps_trip_latched & bit:
                level = "trip"
            elif obs.rps_alarms & bit:
                level = "alarm"
            else:
                level = "off"
            if tile.property("level") != level:
                tile.setProperty("level", level)
                repolish(tile)

        if obs.rps_scram_command:
            az_pos = obs.rod_position[bindings.BANK_EMERGENCY]
            self._scram.setText(f"SCRAM ACTIVE — AZ bank at {az_pos * 100.0:5.1f} %")
        else:
            self._scram.setText("scram command: idle")

        self._reset_note.setText(
            "reset denied (permissives not met)" if obs.rps_reset_denied else ""
        )
