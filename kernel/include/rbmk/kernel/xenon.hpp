// Iodine-135 / xenon-135 chain in normalized units.
//
// The integration routine is a numerics seam: the default implementation is
// C++ (xenon_cpp.cpp); when the project is configured with a Fortran compiler
// the same algorithm is provided by fortran/xenon_kinetics.f90 and selected at
// build time (RBMK_HAVE_FORTRAN). Both implementations must remain in
// algorithmic lockstep; a parity test enforces this when both are built.
#ifndef RBMK_KERNEL_XENON_HPP
#define RBMK_KERNEL_XENON_HPP

namespace rbmk::kernel {

struct XenonState {
    double iodine = 0.0;
    double xenon = 0.0;
};

// Analytic equilibrium for constant power (normalized units; iodine_eq = P).
XenonState xenon_equilibrium(double power_frac) noexcept;

// One RK4 step of the chain:
//   dI/dt = lambda_I * (P - I)
//   dX/dt = gX * P + lambda_I * I - lambda_X * X - sigma_b * P * X
void xenon_step(XenonState& state, double power_frac, double dt_s) noexcept;

// Reactivity contribution (negative; scaled so nominal equilibrium gives
// kXenonEqWorth).
double xenon_reactivity(const XenonState& state) noexcept;

// Reference C++ implementation of the RK4 step (always compiled; used by the
// Fortran parity test and as the fallback when Fortran is unavailable).
void xenon_step_cpp(XenonState& state, double power_frac, double dt_s) noexcept;

}  // namespace rbmk::kernel

#endif  // RBMK_KERNEL_XENON_HPP
