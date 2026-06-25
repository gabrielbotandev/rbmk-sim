// End-to-end protection behavior through the coupled system:
// sense -> protect -> actuate -> physics, all via the public C ABI.
#include <doctest/doctest.h>

#include <cstdint>

#include "rbmk/capi/rbmk_capi.h"

namespace {

// Steps until the predicate holds or the budget runs out; returns steps used.
template <typename Pred>
std::uint32_t step_until(rbmk_sim* sim, std::uint32_t max_steps, Pred pred) {
    rbmk_observation obs;
    for (std::uint32_t i = 0U; i < max_steps; ++i) {
        rbmk_step(sim, 1U);
        rbmk_observe(sim, &obs);
        if (pred(obs)) {
            return i + 1U;
        }
    }
    return max_steps;
}

rbmk_sim* make_sim(std::uint32_t ar_enabled) {
    rbmk_config cfg;
    rbmk_config_default(&cfg);
    cfg.ar_enabled = ar_enabled;
    return rbmk_create(&cfg);
}

}  // namespace

TEST_SUITE("coupling") {
    TEST_CASE("uncontrolled rod withdrawal ends in an overpower or period trip") {
        rbmk_sim* sim = make_sim(0U);
        REQUIRE(sim != nullptr);

        rbmk_set_rod_target(sim, RBMK_BANK_MANUAL_A, 0.0);
        rbmk_set_rod_target(sim, RBMK_BANK_MANUAL_B, 0.0);

        rbmk_observation obs;
        const std::uint32_t used = step_until(
            sim, 4000U, [](const rbmk_observation& o) { return o.rps_state == RBMK_RPS_TRIPPED; });
        rbmk_observe(sim, &obs);
        REQUIRE(used < 4000U);
        CHECK((obs.rps_trip_latched & (RBMK_COND_OVERPOWER | RBMK_COND_SHORT_PERIOD)) != 0U);
        CHECK(obs.rps_scram_command == 1U);
        CHECK(obs.scram_latched == 1U);  // the kernel accepted the scram

        // The scram must actually shut the core down.
        step_until(sim, 2000U, [](const rbmk_observation& o) { return o.power_frac < 0.3; });
        rbmk_observe(sim, &obs);
        CHECK(obs.power_frac < 0.3);
        rbmk_destroy(sim);
    }

    TEST_CASE("AZ-5 press scrams and the plant reaches safe shutdown") {
        rbmk_sim* sim = make_sim(0U);
        REQUIRE(sim != nullptr);

        rbmk_press_az5(sim);
        rbmk_step(sim, 2U);
        rbmk_observation obs;
        rbmk_observe(sim, &obs);
        CHECK(obs.rps_state == RBMK_RPS_TRIPPED);
        CHECK((obs.rps_trip_latched & RBMK_COND_MANUAL_AZ5) != 0U);
        CHECK(obs.scram_latched == 1U);

        // Rods reach full insertion (~18 s) and power decays below 1%.
        const std::uint32_t used = step_until(sim, 20000U, [](const rbmk_observation& o) {
            return o.rps_state == RBMK_RPS_SAFE_SHUTDOWN;
        });
        REQUIRE(used < 20000U);
        rbmk_observe(sim, &obs);
        CHECK(obs.rod_position[RBMK_BANK_EMERGENCY] == doctest::Approx(1.0));
        CHECK(obs.power_frac < 0.01);
        CHECK(obs.rps_scram_command == 1U);  // rods held in
        rbmk_destroy(sim);
    }

    TEST_CASE("reset is denied while tripped, accepted from safe shutdown") {
        rbmk_sim* sim = make_sim(0U);
        REQUIRE(sim != nullptr);

        rbmk_press_az5(sim);
        rbmk_step(sim, 40U);  // 2 s: tripped, rods still travelling
        rbmk_observation obs;
        rbmk_observe(sim, &obs);
        REQUIRE(obs.rps_state == RBMK_RPS_TRIPPED);

        rbmk_request_reset(sim);
        rbmk_step(sim, 1U);
        rbmk_observe(sim, &obs);
        CHECK(obs.rps_state == RBMK_RPS_TRIPPED);
        CHECK(obs.rps_reset_denied == 1U);

        step_until(sim, 20000U,
                   [](const rbmk_observation& o) { return o.rps_state == RBMK_RPS_SAFE_SHUTDOWN; });

        rbmk_request_reset(sim);
        rbmk_step(sim, 1U);
        rbmk_observe(sim, &obs);
        CHECK(obs.rps_state == RBMK_RPS_NORMAL);
        CHECK(obs.rps_trip_latched == 0U);
        CHECK(obs.rps_scram_command == 0U);
        CHECK(obs.scram_latched == 0U);  // kernel scram released
        // Rods remain inserted until the operator withdraws them.
        CHECK(obs.rod_position[RBMK_BANK_MANUAL_A] == doctest::Approx(1.0));
        rbmk_destroy(sim);
    }

    TEST_CASE("detector faults below the minimum trip the plant") {
        rbmk_sim* sim = make_sim(1U);
        REQUIRE(sim != nullptr);

        rbmk_set_detectors_valid(sim, 0x01U);  // one healthy detector left
        rbmk_step(sim, 2U);
        rbmk_observation obs;
        rbmk_observe(sim, &obs);
        CHECK(obs.rps_state == RBMK_RPS_TRIPPED);
        CHECK((obs.rps_trip_latched & RBMK_COND_SENSOR_FAULT) != 0U);
        rbmk_destroy(sim);
    }

    TEST_CASE("losing coolant flow at power trips the plant") {
        rbmk_sim* sim = make_sim(1U);
        REQUIRE(sim != nullptr);

        rbmk_set_pump_flow(sim, 0.4);  // command a deep flow reduction
        const std::uint32_t used = step_until(
            sim, 4000U, [](const rbmk_observation& o) { return o.rps_state == RBMK_RPS_TRIPPED; });
        REQUIRE(used < 4000U);
        rbmk_observation obs;
        rbmk_observe(sim, &obs);
        // The positive void feedback races the flow lag, so the latched cause
        // can be any direct consequence of losing flow at power.
        constexpr std::uint32_t kFlowLossCauses =
            RBMK_COND_LOW_FLOW | RBMK_COND_HIGH_VOID | RBMK_COND_OVERPOWER | RBMK_COND_SHORT_PERIOD;
        CHECK((obs.rps_trip_latched & kFlowLossCauses) != 0U);
        CHECK(obs.flow_frac < 0.9);  // the flow reduction genuinely happened
        rbmk_destroy(sim);
    }
}
