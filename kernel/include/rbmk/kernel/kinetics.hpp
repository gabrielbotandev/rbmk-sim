// Six-group point kinetics with a constant external source term.
#ifndef RBMK_KERNEL_KINETICS_HPP
#define RBMK_KERNEL_KINETICS_HPP

#include <array>
#include <cstdint>

#include "rbmk/kernel/constants.hpp"

namespace rbmk::kernel {

class PointKinetics {
  public:
    // Initializes power and delayed-neutron precursors at equilibrium.
    explicit PointKinetics(double initial_power_frac) noexcept;

    // Advances by dt with reactivity held constant, using `substeps` internal
    // RK4 substeps. Power is clamped to the model validity envelope; returns
    // true when the clamp engaged.
    bool step(double rho, double dt_s, std::uint32_t substeps) noexcept;

    double power() const noexcept {
        return power_;
    }

    const std::array<double, kNumDelayedGroups>& precursors() const noexcept {
        return precursors_;
    }

    // Instantaneous net rate dn/dt / n (per second) from the last step; used
    // by the period meter.
    double last_log_rate_per_s() const noexcept {
        return last_log_rate_;
    }

  private:
    struct Derivatives {
        double dn = 0.0;
        std::array<double, kNumDelayedGroups> dc = {};
    };

    static Derivatives evaluate(double rho, double n,
                                const std::array<double, kNumDelayedGroups>& c) noexcept;

    double power_;
    std::array<double, kNumDelayedGroups> precursors_;
    double last_log_rate_ = 0.0;
};

}  // namespace rbmk::kernel

#endif  // RBMK_KERNEL_KINETICS_HPP
