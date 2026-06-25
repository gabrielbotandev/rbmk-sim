# Verification strategy

Verification is layered: each claim in this safety case maps to an automated
check that runs from a clean checkout with native tools.

## Test pyramid

| Layer | Framework | What it proves |
|-------|-----------|----------------|
| Kernel unit (C++) | doctest via ctest | equilibrium criticality, rod-curve shapes (1986 lobe vs monotonic), xenon equilibrium/peak/decay, thermal feedback signs, validity clamps, scram contrast between designs, bit-identical dual runs, Fortran/C++ parity |
| Protection unit (C) | Unity via ctest | every threshold and hysteresis band, latch persistence, arming logic, AZ-5, reset denial/acceptance, fail-safe NULL behavior, bytewise deterministic replay of input traces |
| Coupled system (C ABI) | doctest via ctest | end-to-end trips (withdrawal, AZ-5 → safe shutdown, flow loss, detector fault), reset flow incl. kernel scram release, NULL tolerance, ABI struct-size assertions, bit-identical scripted sessions |
| Python core | pytest | binding contract (sizes, ABI version), session command log, HDF5 round-trip, replay bit-exactness, tamper detection, logtool CLI paths |
| Scenario/timeline | pytest | schema validation, the full 24 h replay reproducing the documented power profile, xenon transient, the AZ-5 positive excursion, latched causes, saved-log verification |
| Comparison mode | pytest | 1986 bump + rod-reactivity rise vs monotonic modified design, determinism, worth-curve invariants |
| UI smoke | pytest-qt (offscreen) | window builds against the live core, trip banner reacts, fractional clock accumulates correctly |
| Formal | TLA+/TLC | protection state machine: latch safety, no trip→normal shortcut, safe-state reachability under the plant-response fairness assumption |

## Static verification

- **Warnings**: `-Wall -Wextra -Wpedantic -Wshadow -Wconversion
  -Wsign-conversion -Wdouble-promotion …` on every first-party target.
- **clang-tidy** (`cmake --preset tidy`): cert/bugprone/hicpp/clang-analyzer
  families; deliberate exclusions documented with rationale in `.clang-tidy`.
- **clang-format / ruff**: enforced formatting and Python linting.
- **Sanitizers**: the `dev-asan` preset builds with ASan/UBSan (requires
  `libasan`/`libubsan` runtimes installed).

## Commands (native workflows, no wrapper scripts)

```sh
cmake --preset dev && cmake --build --preset dev && ctest --preset dev
.venv/bin/python -m pytest dashboard/tests
.venv/bin/ruff check dashboard
clang-format --dry-run -Werror <sources>
cmake --preset tidy && cmake --build --preset tidy
java -jar specs/tla/tools/tla2tools.jar -config RPS.cfg RPS.tla   # in specs/tla
.venv/bin/sphinx-build -W -b html docs docs/_build/html
```

## What is *not* verified (and why that is acceptable)

- **Physical accuracy** — out of scope by design; see
  {doc}`assumptions_limitations`. The tests pin qualitative shapes, not
  numbers.
- **The Fortran compile path on hosts without gfortran** — the build degrades
  to the parity-tested C++ twin and says so at configure time.
- **Cross-platform bit-exactness** — the determinism contract is per binary;
  the model version in every log makes the producing build identifiable.
