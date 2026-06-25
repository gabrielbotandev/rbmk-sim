#include "rbmk/kernel/detectors.hpp"

#include <algorithm>
#include <cmath>

namespace rbmk::kernel {

namespace {

// Distinct, fixed stream offsets so each detector has an independent
// deterministic noise sequence derived from one seed.
constexpr std::uint64_t kStreamSalt = 0x9E3779B97F4A7C15ULL;

}  // namespace

Detectors::Detectors(bool noise_enabled, std::uint64_t seed) noexcept
    : noise_enabled_(noise_enabled),
      prngs_{Prng(seed ^ (1U * kStreamSalt)), Prng(seed ^ (2U * kStreamSalt)),
             Prng(seed ^ (3U * kStreamSalt)), Prng(seed ^ (4U * kStreamSalt))} {}

void Detectors::step(double power_frac, double dt_s) noexcept {
    for (std::size_t k = 0U; k < kNumDetectors; ++k) {
        double noise = 0.0;
        if (noise_enabled_) {
            noise = kDetectorNoiseSigma * prngs_[k].approx_normal();
        }
        readings_[k] = std::max(power_frac * (1.0 + noise), 0.0);
    }

    if (primed_) {
        const double safe_now = std::max(power_frac, kMinPowerFrac);
        const double safe_prev = std::max(prev_power_, kMinPowerFrac);
        const double inst_rate = (std::log(safe_now) - std::log(safe_prev)) / dt_s;
        rate_ema_per_s_ += kPeriodEmaAlpha * (inst_rate - rate_ema_per_s_);
    } else {
        primed_ = true;
        rate_ema_per_s_ = 0.0;
    }
    prev_power_ = power_frac;

    if (std::abs(rate_ema_per_s_) < (1.0 / kMaxPeriodS)) {
        period_s_ = kMaxPeriodS;  // effectively stable
    } else {
        period_s_ = std::clamp(1.0 / rate_ema_per_s_, -kMaxPeriodS, kMaxPeriodS);
    }
}

double Detectors::reading(std::uint32_t index) const noexcept {
    const std::size_t k = (index < kNumDetectors) ? index : 0U;
    return readings_[k];
}

}  // namespace rbmk::kernel
