// Control-rod banks: motion, scram latch, and position-dependent worth curves,
// including the qualitative 1986-style graphite displacer ("tip") effect.
#ifndef RBMK_KERNEL_RODS_HPP
#define RBMK_KERNEL_RODS_HPP

#include <array>
#include <cstdint>

#include "rbmk/kernel/constants.hpp"
#include "rbmk/kernel/types.hpp"

namespace rbmk::kernel {

class RodSystem {
  public:
    RodSystem(RodDesign design, double initial_manual_insertion) noexcept;

    // Sets the demanded insertion fraction [0, 1] for one bank. Ignored while
    // a scram is latched (the scram demand wins).
    void set_target(Bank bank, double fraction) noexcept;

    // Latches a scram: every bank is driven to full insertion until released.
    void command_scram() noexcept;

    // Releases the scram latch; bank targets remain at full insertion until
    // explicitly re-targeted (operator action).
    void release_scram() noexcept;

    bool scram_latched() const noexcept {
        return scram_latched_;
    }

    // Moves every bank toward its effective target at the bounded rod speed.
    void step(double dt_s) noexcept;

    double position(Bank bank) const noexcept;

    double target(Bank bank) const noexcept;

    // Total rod reactivity (sum over banks of the design worth curve).
    double reactivity() const noexcept;

    // Educational operating-margin proxy: number of "equivalent fully
    // inserted manual banks" currently in the core.
    double inserted_equivalent() const noexcept;

    RodDesign design() const noexcept {
        return design_;
    }

    // Normalized smoothstep absorber fraction A(x) in [0, 1].
    static double absorber_fraction(double x) noexcept;

    // Displacer ("tip") shape D(x) = (27/4) x (1-x)^2, peak 1.0 at x = 1/3.
    static double displacer_shape(double x) noexcept;

    // Reactivity of one bank at insertion x under the given design.
    static double bank_worth(RodDesign design, Bank bank, double x) noexcept;

  private:
    RodDesign design_;
    std::array<double, kNumBanks> position_;
    std::array<double, kNumBanks> target_;
    bool scram_latched_ = false;
};

}  // namespace rbmk::kernel

#endif  // RBMK_KERNEL_RODS_HPP
