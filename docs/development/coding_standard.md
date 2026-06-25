# Coding standard (MISRA-inspired subset)

True MISRA compliance requires commercial tooling and a process beyond an
educational project. RBMK-SIM instead enforces a documented **MISRA-inspired
subset**: the spirit of the rules that matter for deterministic,
reviewable, safety-style code, backed by open tooling.

## Rules (C and C++ in `kernel/`, `protection/`, `orchestrator/`)

**Types and data**
- Fixed-width types (`uint32_t`, `double`) in every interface; no `int` in
  ABI or protection code.
- No implicit narrowing — `-Wconversion -Wsign-conversion` are errors-by-
  culture (warning-clean builds required).
- Flat, padding-free structs at the ABI; layout guarded by `static_assert`.

**Control flow**
- No recursion anywhere in native code.
- All loops bounded by construction (fixed counts or clamped inputs).
- Every `switch` has a `default`; protection functions use a single exit
  point.
- No `goto`, no `setjmp`, no signals.

**Memory and resources**
- No dynamic allocation in protection code or any per-step path. The single
  documented exception: handle creation in the C ABI
  (`NOTE(misra-dev)` at the site).
- Kernel containers are sized once at construction.
- No exceptions across the ABI; `new (std::nothrow)` at the one allocation.

**Defensive interfaces**
- Validate/clamp every external input at the boundary; NULL pointers are
  tolerated and fail toward the safe state (protection commands a scram).
- Hysteresis on every analog threshold; latched trips; explicit state
  machines with enumerated states.

**Determinism**
- No wall clock, locale, environment, or threading in simulation code.
- Randomness only via the seeded in-repo PRNG, only for detector noise.

## Deviations

Any deviation from the subset carries a `NOTE(misra-dev): <justification>`
comment at the site. Current deviations:

| Site | Deviation | Justification |
|------|-----------|---------------|
| `orchestrator/src/capi.cpp` (`rbmk_create`) | dynamic allocation | one handle per session, never in the step path; `std::nothrow` |
| `orchestrator/tests/test_capi.cpp` | `memcmp` of struct objects | layout is padding-free by `static_assert`; bytewise equality *is* the property under test |

## Tooling that backs the subset

- `.clang-format` — enforced formatting (CI-style dry-run check).
- `.clang-tidy` — cert/bugprone/hicpp/clang-analyzer families; every disabled
  check is listed with a rationale in the file header.
- `-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion
  -Wdouble-promotion -Wformat=2 -Wundef -Wcast-align` plus C-specific
  prototype warnings — all first-party targets build clean.
- `dev-asan` preset (ASan/UBSan) for dynamic verification where the runtimes
  are installed.
- Python: `ruff` with the repository profile in `dashboard/pyproject.toml`.
