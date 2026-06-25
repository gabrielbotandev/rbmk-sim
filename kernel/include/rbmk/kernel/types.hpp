// Plain configuration and observation types shared across the kernel.
#ifndef RBMK_KERNEL_TYPES_HPP
#define RBMK_KERNEL_TYPES_HPP

#include <array>
#include <cstdint>

#include "rbmk/kernel/constants.hpp"

namespace rbmk::kernel {

enum class RodDesign : std::uint32_t {
    kOriginal1986 = 0U,  // graphite displacer "tip" effect on scram from low insertion
    kModified = 1U,      // monotonic negative worth curve
};

enum class Bank : std::uint32_t {
    kManualA = 0U,
    kManualB = 1U,
    kAutomatic = 2U,
    kEmergency = 3U,
};

struct Config {
    std::uint32_t num_channels = kDefaultChannels;  // clamped to [kMin, kMax]
    double dt_s = kDefaultDtS;                      // clamped to [kMinDtS, kMaxDtS]
    RodDesign rod_design = RodDesign::kOriginal1986;
    double initial_power_frac = 1.0;             // clamped to [1e-6, 1.2]
    double initial_manual_rod_insertion = 0.35;  // clamped to [0, 1]
    bool start_at_xenon_equilibrium = true;
    bool ar_enabled = true;  // automatic regulator on at start
    bool detector_noise = false;
    std::uint64_t noise_seed = 1U;
};

// Flat snapshot of everything observable.
//
// Field order is deliberate: 8-byte fields first, then the four 32-bit flags
// together at the end, so the struct has no interior padding and instances can
// be compared bytewise in determinism tests.
struct Observation {
    double time_s = 0.0;
    std::uint64_t step_count = 0U;

    // Power and kinetics
    double power_frac = 0.0;
    double power_mw = 0.0;
    double period_s = 0.0;  // signed; +small = fast rise, magnitude clamped

    // Reactivity breakdown (absolute rho, dimensionless)
    double rho_total = 0.0;
    double rho_rods = 0.0;
    double rho_void = 0.0;
    double rho_doppler = 0.0;
    double rho_xenon = 0.0;
    double rho_base = 0.0;

    // Poisons (normalized; xenon_rel = 1 at nominal equilibrium)
    double iodine = 0.0;
    double xenon = 0.0;
    double xenon_rel = 0.0;

    // Thermal-hydraulics
    double avg_void_frac = 0.0;
    double avg_fuel_temp_c = 0.0;
    double coolant_temp_c = 0.0;
    double flow_frac = 0.0;
    double flow_command_frac = 0.0;

    // Rods / regulator
    double inserted_rod_equivalent = 0.0;  // educational operating-margin proxy
    double power_setpoint_frac = 0.0;
    std::array<double, kNumBanks> rod_position = {};  // 0 = withdrawn, 1 = inserted
    std::array<double, kNumBanks> rod_target = {};

    // Channels (first num_channels entries valid)
    std::array<double, kMaxChannels> channel_power = {};  // relative to channel nominal
    std::array<double, kMaxChannels> channel_void = {};

    // Instrumentation
    std::array<double, kNumDetectors> detector_power_frac = {};

    // 32-bit flags, grouped to keep the layout padding-free.
    std::uint32_t scram_latched = 0U;
    std::uint32_t ar_enabled = 0U;
    std::uint32_t num_channels = 0U;
    // Set when an internal clamp engaged (outside the toy model's envelope;
    // results are qualitative-only beyond this point).
    std::uint32_t validity_exceeded = 0U;
};

}  // namespace rbmk::kernel

#endif  // RBMK_KERNEL_TYPES_HPP
