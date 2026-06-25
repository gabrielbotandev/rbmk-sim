# Fortran numerics

Selected numerical routines live in `fortran/` (Fortran 2008 with `bind(C)`
interfaces). Currently: the iodine-135/xenon-135 chain integrator
(`rbmk_f_xenon_step` in [`xenon_kinetics.f90`](../../fortran/src/xenon_kinetics.f90)).

## How the toggle works

- The superbuild probes for a Fortran compiler (`check_language(Fortran)`).
- **Found** — `fortran/` is built, the kernel is compiled with
  `RBMK_HAVE_FORTRAN`, and `rbmk::kernel::xenon_step` dispatches to the
  Fortran routine. The model version string gains a `+fortran` suffix so HDF5
  run logs identify which numerics produced them.
- **Not found** — the kernel uses `xenon_step_cpp`, the reference C++
  implementation of the *same algorithm*, and the build proceeds normally with
  a status message.

## Installing the compiler

```sh
sudo dnf install gcc-gfortran   # Fedora
cmake --preset dev              # reconfigure: "Fortran numerics enabled"
cmake --build --preset dev && ctest --preset dev
```

## Lockstep guarantee

The Fortran routine and `xenon_step_cpp` are algorithmic twins (same RK4, same
constants mirrored from `constants.hpp`). The kernel test *"active integrator
matches the reference C++ implementation"* drives both through 5000 varied
steps and requires agreement to 1e-12 relative — with Fortran installed it is
a genuine cross-language parity check, run by plain `ctest`.

Determinism note: bit-exact replay of a recorded run is guaranteed for the
binary that produced it. A `+fortran` build may differ from a C++-fallback
build at the last few ULPs; the model version in `/meta` makes this visible.
