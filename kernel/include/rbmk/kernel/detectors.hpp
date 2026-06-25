// Instrumentation: power detectors (optional deterministic noise) and a
// smoothed reactor-period meter.
#ifndef RBMK_KERNEL_DETECTORS_HPP
#define RBMK_KERNEL_DETECTORS_HPP

#include <array>
#include <cstdint>

#include "rbmk/kernel/constants.hpp"
#include "rbmk/kernel/prng.hpp"

namespace rbmk::kernel {

class Detectors {
  public:
    Detectors(bool noise_enabled, std::uint64_t seed) noexcept;

    // Updates detector readings and the period estimate from the true power.
    void step(double power_frac, double dt_s) noexcept;

    double reading(std::uint32_t index) const noexcept;

    const std::array<double, kNumDetectors>& readings() const noexcept {
        return readings_;
    }

    // Signed reactor period in seconds (+20 means e-fold up in 20 s); the
    // magnitude is clamped to kMaxPeriodS, which also encodes "stable".
    double period_s() const noexcept {
        return period_s_;
    }

  private:
    bool noise_enabled_;
    std::array<Prng, kNumDetectors> prngs_;
    std::array<double, kNumDetectors> readings_ = {};
    double prev_power_ = 0.0;
    double rate_ema_per_s_ = 0.0;
    double period_s_ = kMaxPeriodS;
    bool primed_ = false;
};

}  // namespace rbmk::kernel

#endif  // RBMK_KERNEL_DETECTORS_HPP
