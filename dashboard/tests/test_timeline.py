"""Chernobyl timeline scenario: schema, full headless replay, consistency."""

from __future__ import annotations

import numpy as np
import pytest

from rbmk_dash.core import bindings
from rbmk_dash.core.recorder import verify_run
from rbmk_dash.core.scenario import load_scenario, scenarios_dir
from rbmk_dash.core.timeline import TimelineRun


@pytest.fixture(scope="module")
def scenario(lib):
    return load_scenario(scenarios_dir() / "chernobyl_timeline.json")


@pytest.fixture(scope="module")
def finished_run(scenario):
    """One full timeline execution shared by the assertion tests."""
    run = TimelineRun(scenario)
    run.run_to_completion()
    yield run
    run.close()


class TestScenarioFile:
    def test_metadata(self, scenario) -> None:
        assert scenario.name == "chernobyl-1986-timeline"
        assert scenario.disclaimer
        assert scenario.sources
        assert scenario.config.rod_design == bindings.ROD_DESIGN_1986

    def test_events_sorted_and_annotated(self, scenario) -> None:
        offsets = [e.offset_s for e in scenario.events]
        assert offsets == sorted(offsets)
        assert len(scenario.events) >= 12
        assert any(e.historical for e in scenario.events)
        assert all(e.narrative for e in scenario.events)
        # The flagship event must exist.
        assert any(e.event_id == "e14-az5" for e in scenario.events)

    def test_rejects_unknown_command(self, tmp_path, scenario) -> None:
        import json

        raw = json.loads((scenarios_dir() / "chernobyl_timeline.json").read_text())
        raw["events"][0]["actions"] = [{"command": "blow_up", "args": []}]
        bad = tmp_path / "bad.json"
        bad.write_text(json.dumps(raw))
        with pytest.raises(ValueError, match="unknown command"):
            load_scenario(bad)


class TestTimelineReplay:
    def _series(self, run: TimelineRun, name: str) -> np.ndarray:
        recorder = run.session.recorder
        if name in recorder._scalars:
            return np.asarray(recorder._scalars[name])
        return np.asarray(recorder._ints[name])

    def _event_time(self, run: TimelineRun, event_id: str) -> float:
        event = next(e for e in run.scenario.events if e.event_id == event_id)
        return event.offset_s

    def test_all_events_applied_in_order(self, finished_run) -> None:
        applied_ids = [a.event.event_id for a in finished_run.applied]
        scenario_ids = [e.event_id for e in finished_run.scenario.events]
        assert applied_ids == scenario_ids
        kinds = {e.kind for e in finished_run.session.recorder.events}
        assert kinds == {"historical", "model"}

    def test_power_profile_follows_the_story(self, finished_run) -> None:
        t = self._series(finished_run, "time_s")
        p = self._series(finished_run, "power_frac")

        def power_at(time_s: float) -> float:
            return float(p[np.searchsorted(t, time_s)])

        # Held near 50% through the long daytime hold.
        for probe in (20000.0, 46000.0, 70000.0):
            assert 0.45 < power_at(probe) < 0.55

        # Near the planned test level before the slump.
        assert 0.17 < power_at(self._event_time(finished_run, "e06-test-level")) < 0.27
        # Slumped to ~1% before recovery begins.
        assert power_at(self._event_time(finished_run, "e08-recovery")) < 0.03
        # Recovered to ~6% before the pumps event.
        assert 0.04 < power_at(self._event_time(finished_run, "e09-pumps")) < 0.09

    def test_xenon_poisoning_evolves(self, finished_run) -> None:
        t = self._series(finished_run, "time_s")
        xe = self._series(finished_run, "xenon_rel")
        # Transient peak during the half-power hold exceeds both endpoints.
        hold = (t > 3600.0) & (t < 60000.0)
        assert xe[hold].max() > 1.10
        # Still poisoned at the ORM event.
        orm_idx = np.searchsorted(t, self._event_time(finished_run, "e10-orm"))
        assert xe[orm_idx] > 1.02

    def test_az5_produces_positive_excursion_then_shutdown(self, finished_run) -> None:
        t = self._series(finished_run, "time_s")
        p = self._series(finished_run, "power_frac")
        az5_t = self._event_time(finished_run, "e14-az5")

        before = (t > az5_t - 4.0) & (t <= az5_t)
        after = (t > az5_t) & (t <= az5_t + 15.0)
        baseline = p[before].max()
        peak = p[after].max()
        assert 0.06 < baseline < 0.16          # climbing from the test point
        assert peak > 1.3 * baseline           # the positive scram excursion
        assert p[-1] < 0.005                   # ... and the absorbers win

        rps = self._series(finished_run, "rps_trip_latched").astype(int)
        assert rps[-1] & bindings.COND_MANUAL_AZ5
        scram = self._series(finished_run, "scram_latched").astype(int)
        assert scram[-1] == 1

    def test_validity_remains_inside_envelope(self, finished_run) -> None:
        validity = self._series(finished_run, "validity_exceeded").astype(int)
        assert validity[-1] == 0  # the toy run stays inside its own envelope


class TestTimelineConsistency:
    def test_saved_timeline_log_verifies_bit_exact(self, scenario, tmp_path) -> None:
        run = TimelineRun(scenario)
        run.run_to_completion()
        path = run.session.save(tmp_path / "timeline.h5")
        run.close()

        report = verify_run(path)
        assert report
        assert max(report.values()) == 0.0
