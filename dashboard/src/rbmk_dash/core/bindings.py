"""ctypes bindings to the rbmk_sim C ABI.

This module is the ONLY place that touches the native library. The struct
definitions mirror ``orchestrator/include/rbmk/capi/rbmk_capi.h`` exactly and
are protected by an ABI version check at load time.
"""

from __future__ import annotations

import ctypes as ct
import os
from functools import lru_cache
from pathlib import Path

ABI_VERSION = 1

MAX_CHANNELS = 64
NUM_BANKS = 4
NUM_DETECTORS = 4

BANK_MANUAL_A = 0
BANK_MANUAL_B = 1
BANK_AUTOMATIC = 2
BANK_EMERGENCY = 3

BANK_NAMES = {
    BANK_MANUAL_A: "Manual A",
    BANK_MANUAL_B: "Manual B",
    BANK_AUTOMATIC: "Automatic (AR)",
    BANK_EMERGENCY: "Emergency (AZ)",
}

ROD_DESIGN_1986 = 0
ROD_DESIGN_MODIFIED = 1

RPS_NORMAL = 0
RPS_ALARM = 1
RPS_TRIPPED = 2
RPS_SAFE_SHUTDOWN = 3

RPS_STATE_NAMES = {
    RPS_NORMAL: "NORMAL",
    RPS_ALARM: "ALARM",
    RPS_TRIPPED: "TRIPPED",
    RPS_SAFE_SHUTDOWN: "SAFE SHUTDOWN",
}

COND_MANUAL_AZ5 = 0x01
COND_OVERPOWER = 0x02
COND_SHORT_PERIOD = 0x04
COND_LOW_FLOW = 0x08
COND_HIGH_VOID = 0x10
COND_SENSOR_FAULT = 0x20

COND_NAMES = {
    COND_MANUAL_AZ5: "AZ-5 MANUAL",
    COND_OVERPOWER: "OVERPOWER",
    COND_SHORT_PERIOD: "SHORT PERIOD",
    COND_LOW_FLOW: "LOW FLOW",
    COND_HIGH_VOID: "HIGH VOID",
    COND_SENSOR_FAULT: "SENSOR FAULT",
}


class Config(ct.Structure):
    """Mirror of ``rbmk_config``."""

    _fields_ = (
        ("struct_size", ct.c_uint32),
        ("num_channels", ct.c_uint32),
        ("dt_s", ct.c_double),
        ("rod_design", ct.c_uint32),
        ("start_at_xenon_equilibrium", ct.c_uint32),
        ("initial_power_frac", ct.c_double),
        ("initial_manual_rod_insertion", ct.c_double),
        ("ar_enabled", ct.c_uint32),
        ("detector_noise", ct.c_uint32),
        ("noise_seed", ct.c_uint64),
    )

    def to_dict(self) -> dict[str, float | int]:
        return {name: getattr(self, name) for name, _ in self._fields_}


class Observation(ct.Structure):
    """Mirror of ``rbmk_observation`` (padding-free by construction)."""

    _fields_ = (
        ("time_s", ct.c_double),
        ("step_count", ct.c_uint64),
        ("power_frac", ct.c_double),
        ("power_mw", ct.c_double),
        ("period_s", ct.c_double),
        ("rho_total", ct.c_double),
        ("rho_rods", ct.c_double),
        ("rho_void", ct.c_double),
        ("rho_doppler", ct.c_double),
        ("rho_xenon", ct.c_double),
        ("rho_base", ct.c_double),
        ("iodine", ct.c_double),
        ("xenon", ct.c_double),
        ("xenon_rel", ct.c_double),
        ("avg_void_frac", ct.c_double),
        ("avg_fuel_temp_c", ct.c_double),
        ("coolant_temp_c", ct.c_double),
        ("flow_frac", ct.c_double),
        ("flow_command_frac", ct.c_double),
        ("inserted_rod_equivalent", ct.c_double),
        ("power_setpoint_frac", ct.c_double),
        ("rod_position", ct.c_double * NUM_BANKS),
        ("rod_target", ct.c_double * NUM_BANKS),
        ("channel_power", ct.c_double * MAX_CHANNELS),
        ("channel_void", ct.c_double * MAX_CHANNELS),
        ("detector_power_frac", ct.c_double * NUM_DETECTORS),
        ("scram_latched", ct.c_uint32),
        ("ar_enabled", ct.c_uint32),
        ("num_channels", ct.c_uint32),
        ("validity_exceeded", ct.c_uint32),
        ("rps_state", ct.c_uint32),
        ("rps_scram_command", ct.c_uint32),
        ("rps_alarms", ct.c_uint32),
        ("rps_trip_active", ct.c_uint32),
        ("rps_trip_latched", ct.c_uint32),
        ("rps_reset_denied", ct.c_uint32),
        ("detectors_valid_mask", ct.c_uint32),
        ("abi_version", ct.c_uint32),
    )


_EXPECTED_OBS_SIZE = (21 + 4 + 4 + 64 + 64 + 4) * 8 + 12 * 4
assert ct.sizeof(Observation) == _EXPECTED_OBS_SIZE, "Observation mirror is out of sync"


def repo_root() -> Path:
    """Locates the repository root by walking up from this file."""
    here = Path(__file__).resolve()
    for parent in here.parents:
        if (parent / "CMakeLists.txt").exists() and (parent / "orchestrator").is_dir():
            return parent
    raise RuntimeError(
        "Could not locate the rbmk-sim repository root; set RBMK_SIM_LIB to the "
        "full path of librbmk_sim.so instead."
    )


def _candidate_library_paths() -> list[Path]:
    env = os.environ.get("RBMK_SIM_LIB")
    if env:
        return [Path(env)]
    root = repo_root()
    return [
        root / "build" / preset / "orchestrator" / "librbmk_sim.so"
        for preset in ("release", "dev", "dev-asan")
    ]


@lru_cache(maxsize=1)
def load_library() -> ct.CDLL:
    """Loads librbmk_sim.so, configures prototypes, and checks the ABI version."""
    candidates = _candidate_library_paths()
    path = next((p for p in candidates if p.exists()), None)
    if path is None:
        tried = "\n  ".join(str(p) for p in candidates)
        raise RuntimeError(
            "librbmk_sim.so not found. Build it first:\n"
            "  cmake --preset release && cmake --build --preset release\n"
            f"Paths tried:\n  {tried}"
        )

    lib = ct.CDLL(str(path))

    lib.rbmk_abi_version.restype = ct.c_uint32
    lib.rbmk_abi_version.argtypes = ()
    lib.rbmk_model_version.restype = ct.c_char_p
    lib.rbmk_model_version.argtypes = ()
    lib.rbmk_config_default.restype = None
    lib.rbmk_config_default.argtypes = (ct.POINTER(Config),)
    lib.rbmk_create.restype = ct.c_void_p
    lib.rbmk_create.argtypes = (ct.POINTER(Config),)
    lib.rbmk_destroy.restype = None
    lib.rbmk_destroy.argtypes = (ct.c_void_p,)
    lib.rbmk_step.restype = None
    lib.rbmk_step.argtypes = (ct.c_void_p, ct.c_uint32)
    lib.rbmk_observe.restype = None
    lib.rbmk_observe.argtypes = (ct.c_void_p, ct.POINTER(Observation))
    lib.rbmk_set_rod_target.restype = None
    lib.rbmk_set_rod_target.argtypes = (ct.c_void_p, ct.c_uint32, ct.c_double)
    lib.rbmk_set_pump_flow.restype = None
    lib.rbmk_set_pump_flow.argtypes = (ct.c_void_p, ct.c_double)
    lib.rbmk_set_power_setpoint.restype = None
    lib.rbmk_set_power_setpoint.argtypes = (ct.c_void_p, ct.c_double)
    lib.rbmk_set_ar_enabled.restype = None
    lib.rbmk_set_ar_enabled.argtypes = (ct.c_void_p, ct.c_uint32)
    lib.rbmk_press_az5.restype = None
    lib.rbmk_press_az5.argtypes = (ct.c_void_p,)
    lib.rbmk_request_reset.restype = None
    lib.rbmk_request_reset.argtypes = (ct.c_void_p,)
    lib.rbmk_set_detectors_valid.restype = None
    lib.rbmk_set_detectors_valid.argtypes = (ct.c_void_p, ct.c_uint32)

    abi = int(lib.rbmk_abi_version())
    if abi != ABI_VERSION:
        raise RuntimeError(f"ABI mismatch: library reports {abi}, bindings expect {ABI_VERSION}")
    return lib


def default_config() -> Config:
    cfg = Config()
    load_library().rbmk_config_default(ct.byref(cfg))
    return cfg


def model_version() -> str:
    return load_library().rbmk_model_version().decode("ascii")


class Simulator:
    """Thin, safe wrapper around one native simulator handle."""

    def __init__(self, config: Config | None = None) -> None:
        self._lib = load_library()
        if config is None:
            config = default_config()
        self.config = config
        self._handle = self._lib.rbmk_create(ct.byref(config))
        if not self._handle:
            raise RuntimeError("rbmk_create failed (allocation)")

    # -- lifecycle ---------------------------------------------------------
    def close(self) -> None:
        if getattr(self, "_handle", None):
            self._lib.rbmk_destroy(self._handle)
            self._handle = None

    def __del__(self) -> None:  # pragma: no cover - GC timing dependent
        self.close()

    def __enter__(self) -> Simulator:
        return self

    def __exit__(self, *exc: object) -> None:
        self.close()

    # -- time / observation --------------------------------------------------
    def step(self, n: int = 1) -> None:
        self._lib.rbmk_step(self._handle, int(n))

    def observe(self, out: Observation | None = None) -> Observation:
        obs = out if out is not None else Observation()
        self._lib.rbmk_observe(self._handle, ct.byref(obs))
        return obs

    # -- operator inputs -----------------------------------------------------
    def set_rod_target(self, bank: int, fraction: float) -> None:
        self._lib.rbmk_set_rod_target(self._handle, int(bank), float(fraction))

    def set_pump_flow(self, fraction: float) -> None:
        self._lib.rbmk_set_pump_flow(self._handle, float(fraction))

    def set_power_setpoint(self, fraction: float) -> None:
        self._lib.rbmk_set_power_setpoint(self._handle, float(fraction))

    def set_ar_enabled(self, enabled: bool) -> None:
        self._lib.rbmk_set_ar_enabled(self._handle, 1 if enabled else 0)

    def press_az5(self) -> None:
        self._lib.rbmk_press_az5(self._handle)

    def request_reset(self) -> None:
        self._lib.rbmk_request_reset(self._handle)

    def set_detectors_valid(self, mask: int) -> None:
        self._lib.rbmk_set_detectors_valid(self._handle, int(mask) & 0x0F)
