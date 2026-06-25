"""Session behavior: history, command log, deterministic re-runs."""

from __future__ import annotations

from rbmk_dash.core import bindings
from rbmk_dash.core.session import CommandRecord, Session


def obs_bytes(session: Session) -> bytes:
    return bytes(memoryview(session.observation))


def replay(config, commands: list[CommandRecord], total_steps: int) -> Session:
    """Re-runs a session from its config and command log."""
    session = Session(config)
    for record in sorted(commands, key=lambda r: r.step):
        if record.step > session.step_count:
            session.advance(record.step - session.step_count)
        session.apply(record.name, *record.args)
    if total_steps > session.step_count:
        session.advance(total_steps - session.step_count)
    return session


class TestHistory:
    def test_history_accumulates_per_sample(self, lib) -> None:
        session = Session()
        for _ in range(10):
            session.advance(20)
        assert len(session.history.time_s) == 11  # initial sample + 10
        t = session.history.times()
        assert t[-1] > t[0]
        assert session.history.series("power_mw").shape == t.shape
        session.close()

    def test_ring_buffer_caps_length(self, lib) -> None:
        session = Session(history_maxlen=16)
        for _ in range(40):
            session.advance(1)
        assert len(session.history.time_s) == 16
        session.close()


class TestCommands:
    def test_commands_are_recorded_with_step(self, lib) -> None:
        session = Session()
        session.advance(100)
        session.apply("set_rod_target", bindings.BANK_MANUAL_A, 0.5)
        session.advance(50)
        session.apply("press_az5")
        assert session.commands == [
            CommandRecord(step=100, name="set_rod_target", args=(bindings.BANK_MANUAL_A, 0.5)),
            CommandRecord(step=150, name="press_az5", args=()),
        ]
        session.close()

    def test_unknown_command_rejected(self, lib) -> None:
        session = Session()
        try:
            session.apply("definitely_not_a_command")
            raise AssertionError("expected ValueError")
        except ValueError:
            pass
        finally:
            session.close()


class TestReplay:
    def test_command_log_replay_is_bit_identical(self, lib) -> None:
        original = Session()
        original.advance(120)
        original.apply("set_rod_target", bindings.BANK_MANUAL_A, 0.2)
        original.advance(300)
        original.apply("set_pump_flow", 0.7)
        original.advance(400)
        original.apply("press_az5")
        original.advance(600)
        final_steps = original.step_count

        clone = replay(bindings.default_config(), original.commands, final_steps)
        assert obs_bytes(clone) == obs_bytes(original)
        original.close()
        clone.close()
