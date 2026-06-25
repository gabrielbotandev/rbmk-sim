#include "rbmk/kernel/kinetics.hpp"

#include <algorithm>
#include <cmath>

namespace rbmk::kernel {

PointKinetics::PointKinetics(double initial_power_frac) noexcept
    : power_(std::clamp(initial_power_frac, kMinPowerFrac, kMaxPowerFrac)), precursors_{} {
    // Delayed-precursor equilibrium: c_i = beta_i * n / (Lambda * lambda_i).
    for (std::size_t i = 0U; i < kNumDelayedGroups; ++i) {
        precursors_[i] = kBeta[i] * power_ / (kGenerationTimeS * kLambdaPerS[i]);
    }
}

PointKinetics::Derivatives
PointKinetics::evaluate(double rho, double n,
                        const std::array<double, kNumDelayedGroups>& c) noexcept {
    Derivatives d;
    double delayed_sum = 0.0;
    for (std::size_t i = 0U; i < kNumDelayedGroups; ++i) {
        delayed_sum += kLambdaPerS[i] * c[i];
        d.dc[i] = (kBeta[i] / kGenerationTimeS) * n - kLambdaPerS[i] * c[i];
    }
    d.dn = ((rho - kBetaTotal) / kGenerationTimeS) * n + delayed_sum + kNeutronSourcePerS;
    return d;
}

bool PointKinetics::step(double rho, double dt_s, std::uint32_t substeps) noexcept {
    const std::uint32_t steps = (substeps == 0U) ? 1U : substeps;
    const double h = dt_s / static_cast<double>(steps);
    const double power_before = power_;
    bool clamped = false;

    for (std::uint32_t s = 0U; s < steps; ++s) {
        // Classic RK4 on the 7-ODE system with rho frozen over the macro step.
        const Derivatives k1 = evaluate(rho, power_, precursors_);

        const double n2 = power_ + 0.5 * h * k1.dn;
        std::array<double, kNumDelayedGroups> c2 = precursors_;
        for (std::size_t i = 0U; i < kNumDelayedGroups; ++i) {
            c2[i] += 0.5 * h * k1.dc[i];
        }
        const Derivatives k2 = evaluate(rho, n2, c2);

        const double n3 = power_ + 0.5 * h * k2.dn;
        std::array<double, kNumDelayedGroups> c3 = precursors_;
        for (std::size_t i = 0U; i < kNumDelayedGroups; ++i) {
            c3[i] += 0.5 * h * k2.dc[i];
        }
        const Derivatives k3 = evaluate(rho, n3, c3);

        const double n4 = power_ + h * k3.dn;
        std::array<double, kNumDelayedGroups> c4 = precursors_;
        for (std::size_t i = 0U; i < kNumDelayedGroups; ++i) {
            c4[i] += h * k3.dc[i];
        }
        const Derivatives k4 = evaluate(rho, n4, c4);

        power_ += (h / 6.0) * (k1.dn + 2.0 * k2.dn + 2.0 * k3.dn + k4.dn);
        for (std::size_t i = 0U; i < kNumDelayedGroups; ++i) {
            precursors_[i] += (h / 6.0) * (k1.dc[i] + 2.0 * k2.dc[i] + 2.0 * k3.dc[i] + k4.dc[i]);
            precursors_[i] = std::max(precursors_[i], 0.0);
        }

        if (power_ < kMinPowerFrac || power_ > kMaxPowerFrac) {
            power_ = std::clamp(power_, kMinPowerFrac, kMaxPowerFrac);
            clamped = true;
        }
    }

    // Logarithmic power rate over the macro step, for the period meter.
    last_log_rate_ = (std::log(power_) - std::log(power_before)) / dt_s;
    return clamped;
}

}  // namespace rbmk::kernel
