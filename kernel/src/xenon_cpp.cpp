#include "rbmk/kernel/xenon.hpp"

#include <algorithm>

#include "rbmk/kernel/constants.hpp"

#if defined(RBMK_HAVE_FORTRAN)
extern "C" {
// Provided by fortran/src/xenon_kinetics.f90 (same RK4 algorithm).
void rbmk_f_xenon_step(double* iodine, double* xenon, const double* power_frac, const double* dt_s);
}
#endif

namespace rbmk::kernel {

namespace {

struct ChainRates {
    double di = 0.0;
    double dx = 0.0;
};

ChainRates rates(double iodine, double xenon, double power) noexcept {
    ChainRates r;
    r.di = kLambdaIodinePerS * (power - iodine);
    r.dx = kXenonDirectYieldPerS * power + kLambdaIodinePerS * iodine - kLambdaXenonPerS * xenon -
           kXenonBurnupPerS * power * xenon;
    return r;
}

}  // namespace

XenonState xenon_equilibrium(double power_frac) noexcept {
    const double p = std::max(power_frac, 0.0);
    XenonState s;
    s.iodine = p;
    s.xenon = (kXenonDirectYieldPerS * p + kLambdaIodinePerS * p) /
              (kLambdaXenonPerS + kXenonBurnupPerS * p);
    return s;
}

void xenon_step_cpp(XenonState& state, double power_frac, double dt_s) noexcept {
    const double p = std::max(power_frac, 0.0);
    const double h = dt_s;

    const ChainRates k1 = rates(state.iodine, state.xenon, p);
    const ChainRates k2 = rates(state.iodine + 0.5 * h * k1.di, state.xenon + 0.5 * h * k1.dx, p);
    const ChainRates k3 = rates(state.iodine + 0.5 * h * k2.di, state.xenon + 0.5 * h * k2.dx, p);
    const ChainRates k4 = rates(state.iodine + h * k3.di, state.xenon + h * k3.dx, p);

    state.iodine += (h / 6.0) * (k1.di + 2.0 * k2.di + 2.0 * k3.di + k4.di);
    state.xenon += (h / 6.0) * (k1.dx + 2.0 * k2.dx + 2.0 * k3.dx + k4.dx);
    state.iodine = std::max(state.iodine, 0.0);
    state.xenon = std::max(state.xenon, 0.0);
}

void xenon_step(XenonState& state, double power_frac, double dt_s) noexcept {
#if defined(RBMK_HAVE_FORTRAN)
    const double p = std::max(power_frac, 0.0);
    rbmk_f_xenon_step(&state.iodine, &state.xenon, &p, &dt_s);
    state.iodine = std::max(state.iodine, 0.0);
    state.xenon = std::max(state.xenon, 0.0);
#else
    xenon_step_cpp(state, power_frac, dt_s);
#endif
}

double xenon_reactivity(const XenonState& state) noexcept {
    return kXenonEqWorth * (state.xenon / kXenonEqAtNominal);
}

}  // namespace rbmk::kernel
