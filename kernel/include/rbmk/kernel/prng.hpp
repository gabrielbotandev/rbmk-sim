// Deterministic PRNG used exclusively for optional detector noise.
// Physics never consumes random numbers.
#ifndef RBMK_KERNEL_PRNG_HPP
#define RBMK_KERNEL_PRNG_HPP

#include <cstdint>

namespace rbmk::kernel {

// xorshift64* (public-domain algorithm). Small, fast, reproducible everywhere.
class Prng {
  public:
    explicit Prng(std::uint64_t seed) noexcept
        : state_((seed != 0U) ? seed : 0x9E3779B97F4A7C15ULL) {}

    // Uniform double in [0, 1).
    double uniform() noexcept {
        state_ ^= state_ >> 12U;
        state_ ^= state_ << 25U;
        state_ ^= state_ >> 27U;
        const std::uint64_t z = state_ * 0x2545F4914F6CDD1DULL;
        // Top 53 bits scaled by 2^-53.
        return static_cast<double>(z >> 11U) * (1.0 / 9007199254740992.0);
    }

    // Approximately standard-normal deviate (Irwin-Hall with n = 4).
    double approx_normal() noexcept {
        const double s = uniform() + uniform() + uniform() + uniform();
        return (s - 2.0) * 1.7320508075688772;  // scale to unit variance
    }

  private:
    std::uint64_t state_;
};

}  // namespace rbmk::kernel

#endif  // RBMK_KERNEL_PRNG_HPP
