// ReactorCore: owns all physics state and advances it deterministically.
//
// Step order is fixed and documented (see docs/model/physics.md):
//   1. automatic regulator updates the AUTO bank target (from last power)
//   2. rod banks move toward their targets (scram demand wins)
//   3. coolant flow lags toward its command
//   4. reactivity components are evaluated
//   5. point kinetics advances (rho frozen over the macro step)
//   6. iodine/xenon chain advances
//   7. channel powers and thermal-hydraulics update
//   8. instrumentation updates
// No allocation, no exceptions, no wall clock, and no randomness (beyond the
// seeded detector-noise PRNG) anywhere in the per-step path.
#ifndef RBMK_KERNEL_CORE_HPP
#define RBMK_KERNEL_CORE_HPP

#include <cstdint>
#include <vector>

#include "rbmk/kernel/constants.hpp"
#include "rbmk/kernel/detectors.hpp"
#include "rbmk/kernel/kinetics.hpp"
#include "rbmk/kernel/rods.hpp"
#include "rbmk/kernel/thermal.hpp"
#include "rbmk/kernel/types.hpp"
#include "rbmk/kernel/xenon.hpp"

namespace rbmk::kernel {

class ReactorCore {
  public:
    // Clamps the configuration into the supported envelope and initializes a
    // steady state: equilibrium kinetics and poisons, settled thermals, and a
    // base-reactivity trim that makes the core exactly critical at t = 0.
    explicit ReactorCore(const Config& config);

    // --- control surface (all inputs clamped) ---
    void set_rod_target(Bank bank, double fraction) noexcept;
    void command_scram() noexcept;
    void release_scram() noexcept;
    void set_pump_flow(double fraction) noexcept;
    void set_power_setpoint(double fraction) noexcept;
    void set_ar_enabled(bool enabled) noexcept;

    // --- time ---
    void step() noexcept;
    void step_n(std::uint32_t n) noexcept;

    // --- observation ---
    void observe(Observation& out) const noexcept;

    const Config& config() const noexcept {
        return config_;
    }

    double time_s() const noexcept {
        return time_s_;
    }

    double power() const noexcept {
        return kinetics_.power();
    }

    bool scram_latched() const noexcept {
        return rods_.scram_latched();
    }

  private:
    static Config sanitize(const Config& config) noexcept;
    void update_channel_power() noexcept;
    void evaluate_reactivity() noexcept;

    Config config_;
    double time_s_ = 0.0;
    std::uint64_t step_count_ = 0U;

    PointKinetics kinetics_;
    RodSystem rods_;
    ThermalHydraulics thermal_;
    XenonState xenon_;
    Detectors detectors_;

    std::vector<double> profile_weight_;  // static radial profile
    std::vector<double> channel_power_;   // relative to channel nominal

    double flow_command_ = 1.0;
    double flow_actual_ = 1.0;

    bool ar_enabled_;
    double power_setpoint_ = 1.0;

    double rho_base_ = 0.0;
    double rho_rods_ = 0.0;
    double rho_void_ = 0.0;
    double rho_doppler_ = 0.0;
    double rho_xenon_ = 0.0;
    double rho_total_ = 0.0;

    bool validity_exceeded_ = false;
};

}  // namespace rbmk::kernel

#endif  // RBMK_KERNEL_CORE_HPP
