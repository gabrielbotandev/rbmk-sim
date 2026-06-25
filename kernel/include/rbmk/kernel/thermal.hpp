// Per-channel thermal-hydraulics: coolant void and fuel temperature as
// first-order lags toward power/flow-dependent steady states, plus the
// associated reactivity feedback (positive void, negative Doppler).
#ifndef RBMK_KERNEL_THERMAL_HPP
#define RBMK_KERNEL_THERMAL_HPP

#include <cstdint>
#include <vector>

#include "rbmk/kernel/constants.hpp"

namespace rbmk::kernel {

class ThermalHydraulics {
  public:
    explicit ThermalHydraulics(std::uint32_t num_channels);

    // Places every state at its steady value for the given channel powers
    // (relative to channel nominal) and flow fraction.
    void init_steady(const std::vector<double>& channel_power, double flow_frac) noexcept;

    // Advances all lags by dt. Returns true if a validity clamp engaged.
    bool step(const std::vector<double>& channel_power, double flow_frac, double dt_s) noexcept;

    double avg_void_frac() const noexcept {
        return avg_void_;
    }

    double avg_fuel_temp_c() const noexcept {
        return avg_fuel_temp_;
    }

    double coolant_temp_c() const noexcept {
        return coolant_temp_;
    }

    const std::vector<double>& channel_void() const noexcept {
        return void_frac_;
    }

    // Feedback relative to the fixed nominal reference state.
    double void_reactivity() const noexcept;

    double doppler_reactivity() const noexcept;

    // Steady-state void fraction for one channel.
    static double steady_void(double channel_power, double flow_frac) noexcept;

  private:
    std::uint32_t num_channels_;
    std::vector<double> void_frac_;
    std::vector<double> fuel_temp_c_;
    double coolant_temp_ = kInletTempC;
    double avg_void_ = 0.0;
    double avg_fuel_temp_ = kInletTempC;
    // Nominal references captured once (channel power 1.0, flow 1.0).
    double ref_void_;
    double ref_fuel_temp_;

    void update_averages() noexcept;
};

}  // namespace rbmk::kernel

#endif  // RBMK_KERNEL_THERMAL_HPP
