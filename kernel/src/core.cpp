#include "rbmk/kernel/core.hpp"

#include <algorithm>
#include <cmath>

namespace rbmk::kernel {

Config ReactorCore::sanitize(const Config& config) noexcept {
    Config c = config;
    c.num_channels = std::clamp(c.num_channels, kMinChannels, kMaxChannels);
    c.dt_s = std::clamp(c.dt_s, kMinDtS, kMaxDtS);
    c.initial_power_frac = std::clamp(c.initial_power_frac, 1.0e-6, 1.2);
    c.initial_manual_rod_insertion = std::clamp(c.initial_manual_rod_insertion, 0.0, 1.0);
    return c;
}

// config_ is declared first in the class, so it is initialized before every
// member below that reads from it.
ReactorCore::ReactorCore(const Config& config)
    : config_(sanitize(config)), kinetics_(config_.initial_power_frac),
      rods_(config_.rod_design, config_.initial_manual_rod_insertion),
      thermal_(config_.num_channels), detectors_(config_.detector_noise, config_.noise_seed),
      profile_weight_(config_.num_channels, 1.0), channel_power_(config_.num_channels, 1.0),
      ar_enabled_(config_.ar_enabled), power_setpoint_(config_.initial_power_frac) {
    const auto n = static_cast<std::size_t>(config_.num_channels);

    // Static radial profile: parabolic, center-peaked, normalized to mean 1.
    double weight_sum = 0.0;
    for (std::size_t i = 0U; i < n; ++i) {
        const double x =
            (static_cast<double>(i) + 0.5) / static_cast<double>(n) - 0.5;  // [-1/2, 1/2)
        profile_weight_[i] = 1.0 - kProfileShape * x * x;
        weight_sum += profile_weight_[i];
    }
    for (std::size_t i = 0U; i < n; ++i) {
        profile_weight_[i] *= static_cast<double>(n) / weight_sum;
    }

    update_channel_power();
    thermal_.init_steady(channel_power_, flow_actual_);

    if (config_.start_at_xenon_equilibrium) {
        xenon_ = xenon_equilibrium(config_.initial_power_frac);
    } else {
        xenon_ = XenonState{};  // clean core
    }

    // Trim so the configured state is exactly critical at t = 0. The trim
    // stands in for fuel excess reactivity; it stays constant during a run.
    rho_base_ = 0.0;
    evaluate_reactivity();
    rho_base_ = -rho_total_;
    evaluate_reactivity();

    detectors_.step(kinetics_.power(), config_.dt_s);
}

void ReactorCore::set_rod_target(Bank bank, double fraction) noexcept {
    rods_.set_target(bank, fraction);
}

void ReactorCore::command_scram() noexcept {
    rods_.command_scram();
}

void ReactorCore::release_scram() noexcept {
    rods_.release_scram();
}

void ReactorCore::set_pump_flow(double fraction) noexcept {
    flow_command_ = std::clamp(fraction, kMinFlowFrac, kMaxFlowFrac);
}

void ReactorCore::set_power_setpoint(double fraction) noexcept {
    power_setpoint_ = std::clamp(fraction, 0.0, 1.2);
}

void ReactorCore::set_ar_enabled(bool enabled) noexcept {
    ar_enabled_ = enabled;
}

void ReactorCore::update_channel_power() noexcept {
    // Channel power relative to channel nominal: phi_i = P * s_i * N, where
    // the share s_i comes from the static profile and local rod absorption.
    const auto n = static_cast<std::size_t>(config_.num_channels);
    const double power = kinetics_.power();

    double share_sum = 0.0;
    for (std::size_t i = 0U; i < n; ++i) {
        // Interleaved assignment of channels to the two manual banks gives a
        // mild, deterministic spatial response to manual rod moves.
        const Bank bank = ((i % 2U) == 0U) ? Bank::kManualA : Bank::kManualB;
        const double absorption =
            1.0 - kLocalAbsorption * RodSystem::absorber_fraction(rods_.position(bank));
        channel_power_[i] = profile_weight_[i] * absorption;
        share_sum += channel_power_[i];
    }
    const double norm = (share_sum > 0.0) ? static_cast<double>(n) / share_sum : 0.0;
    for (std::size_t i = 0U; i < n; ++i) {
        channel_power_[i] *= power * norm;
    }
}

void ReactorCore::evaluate_reactivity() noexcept {
    rho_rods_ = rods_.reactivity();
    rho_void_ = thermal_.void_reactivity();
    rho_doppler_ = thermal_.doppler_reactivity();
    rho_xenon_ = xenon_reactivity(xenon_);
    rho_total_ = rho_base_ + rho_rods_ + rho_void_ + rho_doppler_ + rho_xenon_;
}

void ReactorCore::step() noexcept {
    const double dt = config_.dt_s;

    // (1) Automatic regulator: deadband + bounded proportional adjustment of
    // the AUTO bank target, driven by the previous step's power. Holding
    // inside the deadband avoids dithering against the rod slew limit, and a
    // period governor blocks withdrawal while the reactor is already rising
    // fast (a deterministic stand-in for a startup rate limiter).
    // Disabled while scrammed.
    if (ar_enabled_ && !rods_.scram_latched()) {
        const double error = kinetics_.power() - power_setpoint_;
        const double period = detectors_.period_s();
        const bool rising_fast = (period > 0.0) && (period < kArHoldPeriodS);
        if (error > kArDeadband) {
            const double step_demand = std::clamp(kArKp * error, 0.0, kArMaxStep);
            rods_.set_target(Bank::kAutomatic, rods_.position(Bank::kAutomatic) + step_demand);
        } else if (error < -kArDeadband && !rising_fast) {
            const double step_demand = std::clamp(kArKp * error, -kArMaxStep, 0.0);
            rods_.set_target(Bank::kAutomatic, rods_.position(Bank::kAutomatic) + step_demand);
        } else {
            rods_.set_target(Bank::kAutomatic, rods_.position(Bank::kAutomatic));
        }
    }

    // (2) Rod motion.
    rods_.step(dt);

    // (3) Coolant flow lag toward command.
    flow_actual_ += (flow_command_ - flow_actual_) * (dt / kFlowTimeConstS);

    // (4) Reactivity from the current state.
    evaluate_reactivity();

    // (5) Point kinetics (rho frozen across the macro step).
    bool clamped = kinetics_.step(rho_total_, dt, kKineticsSubsteps);

    // (6) Poisons.
    xenon_step(xenon_, kinetics_.power(), dt);

    // (7) Channel powers and thermal-hydraulics.
    update_channel_power();
    clamped = thermal_.step(channel_power_, flow_actual_, dt) || clamped;

    // (8) Instrumentation.
    detectors_.step(kinetics_.power(), dt);

    if (clamped) {
        validity_exceeded_ = true;
    }
    time_s_ += dt;
    ++step_count_;
}

void ReactorCore::step_n(std::uint32_t n) noexcept {
    for (std::uint32_t i = 0U; i < n; ++i) {
        step();
    }
}

void ReactorCore::observe(Observation& out) const noexcept {
    out = Observation{};  // zero everything, including padding-adjacent fields

    out.time_s = time_s_;
    out.step_count = step_count_;

    out.power_frac = kinetics_.power();
    out.power_mw = kinetics_.power() * kNominalPowerMw;
    out.period_s = detectors_.period_s();

    out.rho_total = rho_total_;
    out.rho_rods = rho_rods_;
    out.rho_void = rho_void_;
    out.rho_doppler = rho_doppler_;
    out.rho_xenon = rho_xenon_;
    out.rho_base = rho_base_;

    out.iodine = xenon_.iodine;
    out.xenon = xenon_.xenon;
    out.xenon_rel = xenon_.xenon / kXenonEqAtNominal;

    out.avg_void_frac = thermal_.avg_void_frac();
    out.avg_fuel_temp_c = thermal_.avg_fuel_temp_c();
    out.coolant_temp_c = thermal_.coolant_temp_c();
    out.flow_frac = flow_actual_;
    out.flow_command_frac = flow_command_;

    for (std::size_t b = 0U; b < kNumBanks; ++b) {
        out.rod_position[b] = rods_.position(static_cast<Bank>(b));
        out.rod_target[b] = rods_.target(static_cast<Bank>(b));
    }
    out.scram_latched = rods_.scram_latched() ? 1U : 0U;
    out.inserted_rod_equivalent = rods_.inserted_equivalent();

    out.ar_enabled = ar_enabled_ ? 1U : 0U;
    out.power_setpoint_frac = power_setpoint_;

    out.num_channels = config_.num_channels;
    const auto n = static_cast<std::size_t>(config_.num_channels);
    for (std::size_t i = 0U; i < n; ++i) {
        out.channel_power[i] = channel_power_[i];
        out.channel_void[i] = thermal_.channel_void()[i];
    }

    for (std::uint32_t k = 0U; k < kNumDetectors; ++k) {
        out.detector_power_frac[k] = detectors_.reading(k);
    }

    out.validity_exceeded = validity_exceeded_ ? 1U : 0U;
}

}  // namespace rbmk::kernel
