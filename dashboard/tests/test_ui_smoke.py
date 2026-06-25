"""Offscreen smoke tests: the dashboard builds, refreshes, and reacts."""

from __future__ import annotations

import pytest

pytest.importorskip("PySide6")


@pytest.fixture
def window(qtbot, lib):
    from rbmk_dash.ui.main_window import MainWindow

    win = MainWindow()
    qtbot.addWidget(win)
    return win


class TestMainWindow:
    def test_builds_with_live_session(self, window) -> None:
        assert "RBMK-SIM" in window.windowTitle()
        assert window.tabs.count() >= 1
        assert window.session.observation.power_frac == pytest.approx(1.0, abs=1e-9)

    def test_advance_updates_status_and_history(self, window) -> None:
        window.advance_steps(100)
        assert window.session.step_count == 100
        assert len(window.session.history.time_s) >= 2

    def test_az5_button_path_trips_protection(self, window, qtbot) -> None:
        from rbmk_dash.core import bindings

        window.session.apply("press_az5")
        window.advance_steps(2)
        obs = window.session.observation
        assert obs.rps_state == bindings.RPS_TRIPPED
        banner = window.annunciator._banner.text()
        assert "TRIPPED" in banner

    def test_speed_accumulator_advances_fractionally(self, window) -> None:
        window._on_speed_changed(0.5)
        before = window.session.step_count
        for _ in range(4):
            window._on_tick()
        # 0.5x speed, 50 ms tick, dt 50 ms -> 0.5 steps per tick -> 2 steps in 4.
        assert window.session.step_count - before == 2
