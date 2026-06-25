#include "rbmk/kernel/thermal.hpp"

#include <algorithm>
#include <cmath>

namespace rbmk::kernel {

namespace {

double effective_flow(double flow_frac) noexcept {
    return std::max(flow_frac, kMinFlowFrac);
}

}  // namespace

ThermalHydraulics::ThermalHydraulics(std::uint32_t num_channels)
    : num_channels_(num_channels), void_frac_(num_channels, 0.0),
      fuel_temp_c_(num_channels, kInletTempC), ref_void_(steady_void(1.0, 1.0)),
      ref_fuel_temp_(kInletTempC + kCoolantDeltaTC + kFuelDeltaTC) {}

double ThermalHydraulics::steady_void(double channel_power, double flow_frac) noexcept {
    const double q = std::max(channel_power, 0.0);
    const double ratio = q / effective_flow(flow_frac);
    if (ratio <= kVoidOnsetRatio) {
        return 0.0;
    }
    const double raw = kVoidAlphaMax * (1.0 - std::exp(-kVoidShape * (ratio - kVoidOnsetRatio)));
    return std::clamp(raw, 0.0, kMaxVoidFrac);
}

void ThermalHydraulics::init_steady(const std::vector<double>& channel_power,
                                    double flow_frac) noexcept {
    double total_power = 0.0;
    for (std::size_t i = 0U; i < channel_power.size() && i < void_frac_.size(); ++i) {
        total_power += channel_power[i];
    }
    const double avg_power =
        (num_channels_ > 0U) ? total_power / static_cast<double>(num_channels_) : 0.0;
    coolant_temp_ = kInletTempC + kCoolantDeltaTC * (avg_power / effective_flow(flow_frac));

    for (std::size_t i = 0U; i < void_frac_.size(); ++i) {
        const double q = (i < channel_power.size()) ? channel_power[i] : 0.0;
        void_frac_[i] = steady_void(q, flow_frac);
        fuel_temp_c_[i] = coolant_temp_ + kFuelDeltaTC * q;
    }
    update_averages();
}

bool ThermalHydraulics::step(const std::vector<double>& channel_power, double flow_frac,
                             double dt_s) noexcept {
    bool clamped = false;

    double total_power = 0.0;
    for (std::size_t i = 0U; i < channel_power.size() && i < void_frac_.size(); ++i) {
        total_power += channel_power[i];
    }
    const double avg_power =
        (num_channels_ > 0U) ? total_power / static_cast<double>(num_channels_) : 0.0;

    const double coolant_ss =
        kInletTempC + kCoolantDeltaTC * (avg_power / effective_flow(flow_frac));
    coolant_temp_ += (coolant_ss - coolant_temp_) * (dt_s / kCoolantTimeConstS);

    for (std::size_t i = 0U; i < void_frac_.size(); ++i) {
        const double q = (i < channel_power.size()) ? channel_power[i] : 0.0;

        const double void_ss = steady_void(q, flow_frac);
        void_frac_[i] += (void_ss - void_frac_[i]) * (dt_s / kVoidTimeConstS);

        const double fuel_ss = coolant_temp_ + kFuelDeltaTC * q;
        fuel_temp_c_[i] += (fuel_ss - fuel_temp_c_[i]) * (dt_s / kFuelTimeConstS);
        if (fuel_temp_c_[i] > kMaxFuelTempC) {
            fuel_temp_c_[i] = kMaxFuelTempC;  // validity envelope, not physics
            clamped = true;
        }
    }
    update_averages();
    return clamped;
}

void ThermalHydraulics::update_averages() noexcept {
    double void_sum = 0.0;
    double fuel_sum = 0.0;
    for (std::size_t i = 0U; i < void_frac_.size(); ++i) {
        void_sum += void_frac_[i];
        fuel_sum += fuel_temp_c_[i];
    }
    const double n = (num_channels_ > 0U) ? static_cast<double>(num_channels_) : 1.0;
    avg_void_ = void_sum / n;
    avg_fuel_temp_ = fuel_sum / n;
}

double ThermalHydraulics::void_reactivity() const noexcept {
    return kVoidCoeffPerVoid * (avg_void_ - ref_void_);
}

double ThermalHydraulics::doppler_reactivity() const noexcept {
    return kDopplerCoeffPerC * (avg_fuel_temp_ - ref_fuel_temp_);
}

}  // namespace rbmk::kernel
