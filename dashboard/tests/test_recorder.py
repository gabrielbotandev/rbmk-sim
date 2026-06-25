"""HDF5 run-log round-trip, deterministic replay, and CLI behavior."""

from __future__ import annotations

import h5py
import numpy as np
import pytest

from rbmk_dash.cli.logtool import main as logtool_main
from rbmk_dash.core import bindings
from rbmk_dash.core.recorder import (
    RecordingSession,
    load_run,
    replay_run,
    verify_run,
)


@pytest.fixture
def recorded_run(lib, tmp_path):
    """A short but eventful recorded run saved to disk."""
    session = RecordingSession(scenario="pytest-run", sample_stride=2)
    session.advance(100)
    session.apply("set_rod_target", bindings.BANK_MANUAL_A, 0.2)
    session.advance(155)  # lands on an odd step on purpose
    session.apply("set_pump_flow", 0.75)
    session.advance(245)
    session.apply("press_az5")
    session.advance(300)
    session.recorder.add_event(0, "phase", "start")
    session.recorder.add_event(500, "phase", "scrammed")
    path = session.save(tmp_path / "run.h5")
    yield path, session
    session.close()


class TestRoundTrip:
    def test_meta_and_config_round_trip(self, recorded_run) -> None:
        path, session = recorded_run
        data = load_run(path)
        assert data.meta["scenario"] == "pytest-run"
        assert data.meta["model_version"] == bindings.model_version()
        assert int(data.meta["abi_version"]) == bindings.ABI_VERSION
        assert data.total_steps == session.step_count
        assert data.sample_stride == 2
        assert data.config.dt_s == pytest.approx(session.sim.config.dt_s)
        assert "disclaimer" in data.meta

    def test_commands_and_events_round_trip(self, recorded_run) -> None:
        path, session = recorded_run
        data = load_run(path)
        assert data.commands == session.commands
        assert [(e.step, e.kind, e.text) for e in data.events] == [
            (0, "phase", "start"),
            (500, "phase", "scrammed"),
        ]

    def test_timeseries_shapes_and_values(self, recorded_run) -> None:
        path, _session = recorded_run
        data = load_run(path)
        t = data.timeseries["time_s"]
        # stride 2 over 800 steps -> 401 samples including t = 0.
        assert t.shape == (401,)
        assert np.all(np.diff(t) > 0)
        assert data.timeseries["rod_position"].shape == (401, bindings.NUM_BANKS)
        assert data.timeseries["channel_void"].shape == (401, 12)
        # The AZ-5 trip must be visible in the recorded protection state.
        assert data.timeseries["rps_state"].max() >= bindings.RPS_TRIPPED

    def test_replay_is_bit_exact(self, recorded_run) -> None:
        path, _ = recorded_run
        report = verify_run(path)
        assert report, "empty verification report"
        assert max(report.values()) == 0.0

    def test_tampered_log_fails_verification(self, recorded_run, tmp_path) -> None:
        path, _ = recorded_run
        tampered = tmp_path / "tampered.h5"
        tampered.write_bytes(path.read_bytes())
        with h5py.File(tampered, "r+") as f:
            values = f["timeseries/power_frac"][()]
            values[len(values) // 2] *= 1.001
            f["timeseries/power_frac"][...] = values
        report = verify_run(tampered)
        assert report["power_frac"] > 0.0

    def test_replay_reconstructs_final_state(self, recorded_run) -> None:
        path, session = recorded_run
        clone = replay_run(load_run(path))
        assert bytes(memoryview(clone.observation)) == bytes(memoryview(session.observation))
        clone.close()


class TestLogtoolCli:
    def test_info_runs(self, recorded_run, capsys) -> None:
        path, _ = recorded_run
        assert logtool_main(["info", str(path)]) == 0
        out = capsys.readouterr().out
        assert "pytest-run" in out
        assert "commands        : 3" in out

    def test_export_csv(self, recorded_run, tmp_path) -> None:
        path, _ = recorded_run
        out_csv = tmp_path / "out.csv"
        code = logtool_main(
            ["export", str(path), "--out", str(out_csv), "--fields", "time_s", "power_mw"]
        )
        assert code == 0
        lines = out_csv.read_text().strip().splitlines()
        assert lines[0] == "time_s,power_mw"
        assert len(lines) == 402  # header + 401 samples

    def test_export_rejects_unknown_field(self, recorded_run, tmp_path) -> None:
        path, _ = recorded_run
        code = logtool_main(
            ["export", str(path), "--out", str(tmp_path / "x.csv"), "--fields", "nope"]
        )
        assert code == 2

    def test_verify_cli_passes(self, recorded_run, capsys) -> None:
        path, _ = recorded_run
        assert logtool_main(["verify", str(path)]) == 0
        assert "PASS" in capsys.readouterr().out
