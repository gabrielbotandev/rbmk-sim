# Determinism and replay guarantees

## The contract

> Identical configuration + identical command sequence ⇒ bit-identical
> observations, on the same binary.

A *run* is fully described by `(config, ordered command log, total steps)`.
Everything else — plots, HDF5 timeseries, exports — is derived data.

## How it is achieved

| Rule | Where enforced |
|------|----------------|
| Fixed step, fixed substep count | kernel (`dt_s`, `kKineticsSubsteps`) |
| Fixed evaluation order inside a step | `ReactorCore::step` (documented 8-stage order) |
| Fixed coupling order across components | orchestrator (sense → protect → actuate → advance) |
| Single-threaded core | no threads anywhere in native code |
| No wall clock, locale, or environment reads in simulation code | kernel/protection/orchestrator by construction |
| Randomness only from the seeded xorshift64* PRNG, detector noise only, off by default | `prng.hpp`, `detectors.cpp`; physics never consumes random numbers |
| No allocation in the step path (no allocator nondeterminism) | kernel containers sized at construction; the only heap allocation is handle creation |
| Padding-free observation structs (bytewise comparable) | `types.hpp`, `rbmk_capi.h`, `static_assert`s |
| Commands pinned to step indices | `Session.apply` records `(step, name, args)` |

## Replay and verification

- `replay_run` rebuilds the simulator from the logged config and re-applies
  the command log at the recorded step boundaries, sampling at the recorded
  stride.
- `verify_run` (also `rbmk-logtool verify`) compares **every** recorded
  signal of the replay against the log and reports the maximum absolute
  deviation per signal. The pass criterion is exact zero everywhere.
- Tests cover: bit-identical dual runs (kernel, ABI, session, comparison
  levels), round-trips through HDF5, tamper detection (a modified log fails
  verification), and full-timeline replay equality.

## Honest limits of the guarantee

- **Per-binary.** Different compilers, optimization levels, or the
  Fortran-vs-C++ numerics switch may differ in the last ULPs. Every run log
  records the model version (with `+fortran` suffix when applicable) so a
  replay environment can be matched to the producing one.
- **Wall-clock metadata.** `created_utc` in `/meta` is documentation, not
  simulation state; it does not participate in replay.
- **UI sampling cadence** affects only which samples land in the plot
  history, never the simulation trajectory; the recorder samples on exact
  stride boundaries independent of UI timing.
