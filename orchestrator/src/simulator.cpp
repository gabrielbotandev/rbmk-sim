#include "rbmk/orchestrator/simulator.hpp"

#include <algorithm>

namespace rbmk::orchestrator {

namespace {

constexpr double kRodsFullInThreshold = 0.999;

}  // namespace

Simulator::Simulator(const kernel::Config& config) : core_(config) {
    rps_init(&rps_);
    // Establish a consistent first output snapshot (NORMAL, no scram).
    core_.observe(scratch_);
    rps_inputs_t inputs = {};
    inputs.power_frac = scratch_.power_frac;
    inputs.period_s = scratch_.period_s;
    inputs.coolant_flow_frac = scratch_.flow_frac;
    inputs.avg_void_frac = scratch_.avg_void_frac;
    inputs.detectors_valid = detectors_valid_;
    rps_step(&rps_, &inputs, &rps_out_);
}

void Simulator::scan_and_step() noexcept {
    // (1) Sense: conservative readings from the instrumentation.
    core_.observe(scratch_);

    double max_valid_reading = 0.0;
    for (std::uint32_t k = 0U; k < kernel::kNumDetectors; ++k) {
        if ((detectors_valid_ & (1U << k)) != 0U) {
            max_valid_reading = std::max(max_valid_reading, scratch_.detector_power_frac[k]);
        }
    }

    bool rods_full_in = true;
    for (std::size_t b = 0U; b < kernel::kNumBanks; ++b) {
        rods_full_in = rods_full_in && (scratch_.rod_position[b] >= kRodsFullInThreshold);
    }

    rps_inputs_t inputs = {};
    inputs.power_frac = max_valid_reading;
    inputs.period_s = scratch_.period_s;
    inputs.coolant_flow_frac = scratch_.flow_frac;
    inputs.avg_void_frac = scratch_.avg_void_frac;
    inputs.detectors_valid = detectors_valid_;
    inputs.manual_az5 = az5_pulse_;
    inputs.reset_request = reset_pulse_;
    inputs.rods_full_in = rods_full_in ? 1U : 0U;

    // (2) Protect.
    rps_step(&rps_, &inputs, &rps_out_);
    az5_pulse_ = 0U;
    reset_pulse_ = 0U;

    // (3) Actuate: the protection command always wins over operator targets.
    if (rps_out_.scram_command == 1U) {
        if (!core_.scram_latched()) {
            core_.command_scram();
        }
    } else {
        if (core_.scram_latched()) {
            core_.release_scram();  // after an accepted reset
        }
    }

    // (4) Advance physics.
    core_.step();
}

void Simulator::step(std::uint32_t n) noexcept {
    for (std::uint32_t i = 0U; i < n; ++i) {
        scan_and_step();
    }
}

void Simulator::observe(rbmk_observation& out) const noexcept {
    out = rbmk_observation{};

    kernel::Observation obs;
    core_.observe(obs);

    out.time_s = obs.time_s;
    out.step_count = obs.step_count;
    out.power_frac = obs.power_frac;
    out.power_mw = obs.power_mw;
    out.period_s = obs.period_s;
    out.rho_total = obs.rho_total;
    out.rho_rods = obs.rho_rods;
    out.rho_void = obs.rho_void;
    out.rho_doppler = obs.rho_doppler;
    out.rho_xenon = obs.rho_xenon;
    out.rho_base = obs.rho_base;
    out.iodine = obs.iodine;
    out.xenon = obs.xenon;
    out.xenon_rel = obs.xenon_rel;
    out.avg_void_frac = obs.avg_void_frac;
    out.avg_fuel_temp_c = obs.avg_fuel_temp_c;
    out.coolant_temp_c = obs.coolant_temp_c;
    out.flow_frac = obs.flow_frac;
    out.flow_command_frac = obs.flow_command_frac;
    out.inserted_rod_equivalent = obs.inserted_rod_equivalent;
    out.power_setpoint_frac = obs.power_setpoint_frac;

    for (std::size_t b = 0U; b < kernel::kNumBanks; ++b) {
        out.rod_position[b] = obs.rod_position[b];
        out.rod_target[b] = obs.rod_target[b];
    }
    for (std::size_t i = 0U; i < kernel::kMaxChannels; ++i) {
        out.channel_power[i] = obs.channel_power[i];
        out.channel_void[i] = obs.channel_void[i];
    }
    for (std::size_t k = 0U; k < kernel::kNumDetectors; ++k) {
        out.detector_power_frac[k] = obs.detector_power_frac[k];
    }

    out.scram_latched = obs.scram_latched;
    out.ar_enabled = obs.ar_enabled;
    out.num_channels = obs.num_channels;
    out.validity_exceeded = obs.validity_exceeded;

    out.rps_state = rps_out_.state;
    out.rps_scram_command = rps_out_.scram_command;
    out.rps_alarms = rps_out_.alarms;
    out.rps_trip_active = rps_out_.trip_active;
    out.rps_trip_latched = rps_out_.trip_latched;
    out.rps_reset_denied = rps_out_.reset_denied;
    out.detectors_valid_mask = detectors_valid_;
    out.abi_version = RBMK_ABI_VERSION;
}

void Simulator::set_rod_target(std::uint32_t bank, double fraction) noexcept {
    if (bank < kernel::kNumBanks) {
        core_.set_rod_target(static_cast<kernel::Bank>(bank), fraction);
    }
}

void Simulator::set_pump_flow(double fraction) noexcept {
    core_.set_pump_flow(fraction);
}

void Simulator::set_power_setpoint(double fraction) noexcept {
    core_.set_power_setpoint(fraction);
}

void Simulator::set_ar_enabled(bool enabled) noexcept {
    core_.set_ar_enabled(enabled);
}

void Simulator::press_az5() noexcept {
    az5_pulse_ = 1U;
}

void Simulator::request_reset() noexcept {
    reset_pulse_ = 1U;
}

void Simulator::set_detectors_valid(std::uint32_t mask) noexcept {
    detectors_valid_ = mask & 0x0FU;
}

}  // namespace rbmk::orchestrator
