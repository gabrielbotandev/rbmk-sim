#include "rbmk/kernel/rods.hpp"

#include <algorithm>
#include <cmath>

namespace rbmk::kernel {

namespace {

constexpr double clamp01(double v) noexcept {
    return std::clamp(v, 0.0, 1.0);
}

std::size_t index_of(Bank bank) noexcept {
    const auto raw = static_cast<std::size_t>(bank);
    return (raw < static_cast<std::size_t>(kNumBanks)) ? raw : 0U;
}

std::array<double, kNumBanks> initial_positions(double manual_insertion) noexcept {
    std::array<double, kNumBanks> positions{};
    positions[index_of(Bank::kManualA)] = manual_insertion;
    positions[index_of(Bank::kManualB)] = manual_insertion;
    positions[index_of(Bank::kAutomatic)] = 0.5;  // regulator mid-travel
    positions[index_of(Bank::kEmergency)] = 0.0;  // AZ bank parked out
    return positions;
}

}  // namespace

RodSystem::RodSystem(RodDesign design, double initial_manual_insertion) noexcept
    : design_(design), position_(initial_positions(clamp01(initial_manual_insertion))),
      target_(position_) {}

void RodSystem::set_target(Bank bank, double fraction) noexcept {
    if (!scram_latched_) {
        target_[index_of(bank)] = clamp01(fraction);
    }
}

void RodSystem::command_scram() noexcept {
    scram_latched_ = true;
    for (std::size_t b = 0U; b < kNumBanks; ++b) {
        target_[b] = 1.0;
    }
}

void RodSystem::release_scram() noexcept {
    scram_latched_ = false;
}

void RodSystem::step(double dt_s) noexcept {
    // Scram drives at full speed; normal bank moves use the slower slew.
    const double speed = scram_latched_ ? kRodSpeedPerS : kManualRodSpeedPerS;
    const double max_travel = speed * dt_s;
    for (std::size_t b = 0U; b < kNumBanks; ++b) {
        const double delta = target_[b] - position_[b];
        const double bounded = std::clamp(delta, -max_travel, max_travel);
        position_[b] = clamp01(position_[b] + bounded);
    }
}

double RodSystem::position(Bank bank) const noexcept {
    return position_[index_of(bank)];
}

double RodSystem::target(Bank bank) const noexcept {
    return target_[index_of(bank)];
}

double RodSystem::absorber_fraction(double x) noexcept {
    const double c = clamp01(x);
    return c * c * (3.0 - 2.0 * c);  // smoothstep: S-shaped rod worth
}

double RodSystem::displacer_shape(double x) noexcept {
    const double c = clamp01(x);
    const double one_minus = 1.0 - c;
    return (27.0 / 4.0) * c * one_minus * one_minus;  // peak 1.0 at x = 1/3
}

double RodSystem::bank_worth(RodDesign design, Bank bank, double x) noexcept {
    const std::size_t b = index_of(bank);
    double rho = kBankWorth[b] * absorber_fraction(x);
    if (design == RodDesign::kOriginal1986 && kBankHasDisplacer[b]) {
        // Graphite displacer passing through the lower core: transient
        // positive contribution before the absorber dominates.
        rho += kDisplacerTipWorth * displacer_shape(x);
    }
    return rho;
}

double RodSystem::reactivity() const noexcept {
    double rho = 0.0;
    for (std::size_t b = 0U; b < kNumBanks; ++b) {
        rho += bank_worth(design_, static_cast<Bank>(b), position_[b]);
    }
    return rho;
}

double RodSystem::inserted_equivalent() const noexcept {
    double equivalent = 0.0;
    for (std::size_t b = 0U; b < kNumBanks; ++b) {
        equivalent += absorber_fraction(position_[b]);
    }
    return equivalent;
}

}  // namespace rbmk::kernel
