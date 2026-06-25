# The C ABI

[`orchestrator/include/rbmk/capi/rbmk_capi.h`](../../orchestrator/include/rbmk/capi/rbmk_capi.h)
is the single integration boundary between the native core and every
frontend. The Python bindings (`rbmk_dash.core.bindings`) are a 1:1 ctypes
mirror of this header and the only Python file allowed to touch the library.

## Design rules

- **Flat, fixed-size, padding-free structs** of `double`/`uint32`/`uint64`
  only. `rbmk_observation` orders 8-byte fields first and ends with an even
  count of `uint32` flags; `static_assert`s (C++) and a size assert (Python)
  keep both sides honest. Padding-free layout is what makes bytewise
  determinism comparisons meaningful.
- **Versioned**: `rbmk_abi_version()` must match `RBMK_ABI_VERSION` at load
  time; any layout or semantic change bumps it. `rbmk_model_version()`
  identifies the producing build (with `+fortran` when the Fortran numerics
  are compiled in) and is embedded in every run log.
- **NULL-tolerant, exception-free**: every function accepts a NULL handle as
  a no-op; `rbmk_create` returns NULL on allocation failure instead of
  throwing; nothing else allocates.
- **No callbacks, no threads, no ownership transfer** beyond
  `rbmk_create`/`rbmk_destroy`.

## Surface

| Group | Functions |
|-------|-----------|
| metadata | `rbmk_abi_version`, `rbmk_model_version` |
| lifecycle | `rbmk_config_default`, `rbmk_create`, `rbmk_destroy` |
| time | `rbmk_step(sim, n)` |
| observation | `rbmk_observe(sim, out)` |
| operator inputs | `rbmk_set_rod_target`, `rbmk_set_pump_flow`, `rbmk_set_power_setpoint`, `rbmk_set_ar_enabled`, `rbmk_press_az5`, `rbmk_request_reset` |
| fault injection | `rbmk_set_detectors_valid` (educational) |

`rbmk_press_az5` and `rbmk_request_reset` are one-scan pulses: the trip
latches in the RPS (so a pulse is sufficient), and the reset permissive is
evaluated on the scan that sees the pulse.

## Library discovery (Python side)

`RBMK_SIM_LIB` overrides; otherwise `build/release`, `build/dev`,
`build/dev-asan` are searched relative to the repository root. A mismatch in
ABI version or struct size fails loudly at import, never silently.
