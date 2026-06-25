// Orchestrator: couples the C++ physics kernel with the C protection system
// in a fixed order each step, independent of any frontend:
//
//   sense (kernel observation -> RPS inputs)
//     -> protect (rps_step)
//       -> actuate (scram command -> kernel)
//         -> advance physics (kernel step)
//
// The protection scan runs every kernel step (50 ms at the default dt).
#ifndef RBMK_ORCHESTRATOR_SIMULATOR_HPP
#define RBMK_ORCHESTRATOR_SIMULATOR_HPP

#include <cstdint>

#include "rbmk/capi/rbmk_capi.h"
#include "rbmk/kernel/core.hpp"
#include "rbmk/rps/rps.h"

namespace rbmk::orchestrator {

class Simulator {
  public:
    explicit Simulator(const kernel::Config& config);

    // Advances the coupled system by n steps.
    void step(std::uint32_t n) noexcept;

    // Fills the flat ABI observation (kernel + protection state).
    void observe(rbmk_observation& out) const noexcept;

    // Operator inputs (forwarded with clamping).
    void set_rod_target(std::uint32_t bank, double fraction) noexcept;
    void set_pump_flow(double fraction) noexcept;
    void set_power_setpoint(double fraction) noexcept;
    void set_ar_enabled(bool enabled) noexcept;
    void press_az5() noexcept;      // held for exactly one protection scan
    void request_reset() noexcept;  // held for exactly one protection scan
    void set_detectors_valid(std::uint32_t mask) noexcept;

  private:
    void scan_and_step() noexcept;

    kernel::ReactorCore core_;
    rps_t rps_ = {};
    rps_outputs_t rps_out_ = {};
    kernel::Observation scratch_ = {};  // reused per step; no allocation
    std::uint32_t az5_pulse_ = 0U;
    std::uint32_t reset_pulse_ = 0U;
    std::uint32_t detectors_valid_ = 0x0FU;
};

}  // namespace rbmk::orchestrator

#endif  // RBMK_ORCHESTRATOR_SIMULATOR_HPP
