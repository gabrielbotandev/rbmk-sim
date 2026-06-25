"""ctypes binding tests against the real native library."""

from __future__ import annotations

import ctypes as ct

from rbmk_dash.core import bindings
from rbmk_dash.core.bindings import Config, Observation, Simulator


def obs_bytes(obs: Observation) -> bytes:
    return bytes(memoryview(obs))


class TestLibrary:
    def test_abi_version(self, lib) -> None:
        assert lib.rbmk_abi_version() == bindings.ABI_VERSION

    def test_model_version_string(self, lib) -> None:
        version = bindings.model_version()
        assert "." in version

    def test_struct_sizes_match_capi_contract(self, lib) -> None:
        # rbmk_config: 6 x u32 + 3 x double + u64 (with natural alignment).
        assert ct.sizeof(Config) == 6 * 4 + 3 * 8 + 8
        assert ct.sizeof(Observation) == (21 + 4 + 4 + 64 + 64 + 4) * 8 + 12 * 4


class TestSimulator:
    def test_defaults_and_initial_state(self, lib) -> None:
        with Simulator() as sim:
            obs = sim.observe()
            assert obs.abi_version == bindings.ABI_VERSION
            assert abs(obs.power_frac - 1.0) < 1e-9
            assert obs.rps_state == bindings.RPS_NORMAL
            assert obs.num_channels == 12

    def test_stepping_advances_time(self, lib) -> None:
        with Simulator() as sim:
            sim.step(200)
            obs = sim.observe()
            assert obs.step_count == 200
            assert abs(obs.time_s - 10.0) < 1e-9

    def test_az5_trips_protection(self, lib) -> None:
        with Simulator() as sim:
            sim.press_az5()
            sim.step(2)
            obs = sim.observe()
            assert obs.rps_state == bindings.RPS_TRIPPED
            assert obs.rps_trip_latched & bindings.COND_MANUAL_AZ5
            assert obs.scram_latched == 1

    def test_determinism_bytewise(self, lib) -> None:
        cfg_a = bindings.default_config()
        cfg_b = bindings.default_config()
        with Simulator(cfg_a) as a, Simulator(cfg_b) as b:
            for sim in (a, b):
                sim.set_rod_target(bindings.BANK_MANUAL_A, 0.2)
                sim.step(500)
                sim.set_pump_flow(0.8)
                sim.step(500)
            assert obs_bytes(a.observe()) == obs_bytes(b.observe())

    def test_noise_seed_changes_detectors_not_physics(self, lib) -> None:
        cfg_a = bindings.default_config()
        cfg_a.detector_noise = 1
        cfg_a.noise_seed = 1
        cfg_b = bindings.default_config()
        cfg_b.detector_noise = 1
        cfg_b.noise_seed = 2
        with Simulator(cfg_a) as a, Simulator(cfg_b) as b:
            a.step(100)
            b.step(100)
            oa, ob = a.observe(), b.observe()
            assert oa.detector_power_frac[0] != ob.detector_power_frac[0]
            assert oa.power_frac == ob.power_frac

    def test_modified_rod_design_flag(self, lib) -> None:
        cfg = bindings.default_config()
        cfg.rod_design = bindings.ROD_DESIGN_MODIFIED
        cfg.initial_manual_rod_insertion = 0.05
        cfg.ar_enabled = 0
        with Simulator(cfg) as sim:
            sim.press_az5()
            peak = 0.0
            for _ in range(120):  # 6 s
                sim.step(1)
                peak = max(peak, sim.observe().power_frac)
            assert peak <= 1.002  # modified design: no positive excursion
