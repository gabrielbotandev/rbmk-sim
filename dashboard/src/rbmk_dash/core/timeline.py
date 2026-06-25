"""Timeline replay engine: drives a recording session through a scenario's
events, fast-forwarding deterministically between them.

The engine never compresses simulated time (poison dynamics need their real
hours); instead it advances the kernel as fast as the host allows and samples
at the recording stride. Events are applied at exact step boundaries, so a
timeline run is as replayable as any interactive session.
"""

from __future__ import annotations

from dataclasses import dataclass

from rbmk_dash.core.recorder import RecordingSession
from rbmk_dash.core.scenario import Scenario, ScenarioEvent

#: Default sampling stride for long timelines (steps); 20 steps = 1 s at dt 0.05.
TIMELINE_STRIDE = 20


@dataclass(frozen=True)
class AppliedEvent:
    event: ScenarioEvent
    step: int


class TimelineRun:
    """One pass through a scenario."""

    def __init__(self, scenario: Scenario, sample_stride: int = TIMELINE_STRIDE) -> None:
        self.scenario = scenario
        self.session = RecordingSession(
            scenario.config,
            scenario=scenario.name,
            sample_stride=sample_stride,
            history_maxlen=200_000,
        )
        self.applied: list[AppliedEvent] = []
        self._next_index = 0
        # Apply any events scheduled at t = 0 immediately.
        self._apply_due_events()

    # ------------------------------------------------------------------ state
    @property
    def next_event(self) -> ScenarioEvent | None:
        if self._next_index < len(self.scenario.events):
            return self.scenario.events[self._next_index]
        return None

    @property
    def current_event(self) -> ScenarioEvent | None:
        return self.applied[-1].event if self.applied else None

    @property
    def finished(self) -> bool:
        return self.next_event is None

    def event_step(self, event: ScenarioEvent) -> int:
        return round(event.offset_s / self.session.dt_s)

    # ------------------------------------------------------------------ driving
    def _apply_due_events(self) -> None:
        while self.next_event is not None and self.event_step(self.next_event) <= (
            self.session.step_count
        ):
            event = self.scenario.events[self._next_index]
            for action in event.actions:
                self.session.apply(action.command, *action.args)
            kind = "historical" if event.historical else "model"
            self.session.recorder.add_event(
                self.session.step_count, kind, f"{event.label} — {event.narrative}"
            )
            self.applied.append(AppliedEvent(event=event, step=self.session.step_count))
            self._next_index += 1

    def advance_seconds(self, seconds: float) -> None:
        """Advances by wall-of-simulation seconds, applying events on the way."""
        target = self.session.step_count + max(0, round(seconds / self.session.dt_s))
        while self.session.step_count < target:
            bound = target
            if self.next_event is not None:
                bound = min(bound, self.event_step(self.next_event))
            if bound > self.session.step_count:
                self.session.advance(bound - self.session.step_count)
            self._apply_due_events()
            if self.next_event is None and self.session.step_count >= target:
                break

    def advance_to_next_event(self) -> ScenarioEvent | None:
        """Fast-forwards to (and applies) the next event; returns it."""
        upcoming = self.next_event
        if upcoming is None:
            return None
        target = self.event_step(upcoming)
        if target > self.session.step_count:
            self.session.advance(target - self.session.step_count)
        self._apply_due_events()
        return upcoming

    def run_to_completion(self, epilogue_s: float = 0.0) -> None:
        while not self.finished:
            self.advance_to_next_event()
        if epilogue_s > 0.0:
            self.advance_seconds(epilogue_s)

    def close(self) -> None:
        self.session.close()
