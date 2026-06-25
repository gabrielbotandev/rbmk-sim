// ABI surface tests: metadata, lifecycle, NULL tolerance, determinism.
#include <doctest/doctest.h>

#include <cstdint>
#include <cstring>
#include <string>

#include "rbmk/capi/rbmk_capi.h"

namespace {

// NOTE(misra-dev): rbmk_observation is deliberately laid out padding-free
// (verified by the static_assert below), so the object representation IS
// unique and bytewise comparison is exactly the determinism property we
// guarantee.
bool obs_bit_identical(const rbmk_observation& a, const rbmk_observation& b) {
    // NOLINTNEXTLINE(bugprone-suspicious-memory-comparison,cert-exp42-c,cert-flp37-c)
    return std::memcmp(&a, &b, sizeof(rbmk_observation)) == 0;
}

// 21 scalar 8-byte fields + (4+4+64+64+4) array doubles + 12 uint32 flags.
static_assert(sizeof(rbmk_observation) == (21U + 4U + 4U + 64U + 64U + 4U) * 8U + 12U * 4U,
              "rbmk_observation must remain padding-free");

}  // namespace

TEST_SUITE("capi") {
    TEST_CASE("metadata") {
        CHECK(rbmk_abi_version() == RBMK_ABI_VERSION);
        REQUIRE(rbmk_model_version() != nullptr);
        CHECK(std::string(rbmk_model_version()).find('.') != std::string::npos);
    }

    TEST_CASE("config defaults") {
        rbmk_config cfg;
        rbmk_config_default(&cfg);
        CHECK(cfg.struct_size == sizeof(rbmk_config));
        CHECK(cfg.num_channels == 12U);
        CHECK(cfg.dt_s == doctest::Approx(0.05));
        CHECK(cfg.rod_design == RBMK_ROD_DESIGN_1986);
        CHECK(cfg.initial_power_frac == doctest::Approx(1.0));
        CHECK(cfg.ar_enabled == 1U);
        CHECK(cfg.detector_noise == 0U);
        rbmk_config_default(nullptr);  // must not crash
    }

    TEST_CASE("NULL handle tolerance") {
        rbmk_step(nullptr, 10U);
        rbmk_set_rod_target(nullptr, 0U, 0.5);
        rbmk_set_pump_flow(nullptr, 0.5);
        rbmk_press_az5(nullptr);
        rbmk_request_reset(nullptr);
        rbmk_destroy(nullptr);
        rbmk_observation obs;
        rbmk_observe(nullptr, &obs);

        rbmk_sim* sim = rbmk_create(nullptr);  // NULL config => defaults
        REQUIRE(sim != nullptr);
        rbmk_observe(sim, nullptr);  // must not crash
        rbmk_destroy(sim);
    }

    TEST_CASE("initial observation is a healthy critical core") {
        rbmk_sim* sim = rbmk_create(nullptr);
        REQUIRE(sim != nullptr);
        rbmk_observation obs;
        rbmk_observe(sim, &obs);
        CHECK(obs.abi_version == RBMK_ABI_VERSION);
        CHECK(obs.power_frac == doctest::Approx(1.0).epsilon(1e-9));
        CHECK(obs.rps_state == RBMK_RPS_NORMAL);
        CHECK(obs.rps_scram_command == 0U);
        CHECK(obs.num_channels == 12U);
        CHECK(obs.detectors_valid_mask == 0x0FU);
        rbmk_destroy(sim);
    }

    TEST_CASE("scripted sessions are bit-identical across instances") {
        rbmk_config cfg;
        rbmk_config_default(&cfg);
        cfg.detector_noise = 1U;
        cfg.noise_seed = 77U;

        rbmk_sim* a = rbmk_create(&cfg);
        rbmk_sim* b = rbmk_create(&cfg);
        REQUIRE(a != nullptr);
        REQUIRE(b != nullptr);

        const auto script = [](rbmk_sim* sim, std::uint32_t phase) {
            switch (phase) {
            case 1U:
                rbmk_set_rod_target(sim, RBMK_BANK_MANUAL_A, 0.2);
                break;
            case 3U:
                rbmk_set_pump_flow(sim, 0.75);
                break;
            case 5U:
                rbmk_press_az5(sim);
                break;
            default:
                break;
            }
        };

        rbmk_observation oa;
        rbmk_observation ob;
        for (std::uint32_t phase = 0U; phase < 8U; ++phase) {
            script(a, phase);
            script(b, phase);
            rbmk_step(a, 200U);
            rbmk_step(b, 200U);
            rbmk_observe(a, &oa);
            rbmk_observe(b, &ob);
            REQUIRE(obs_bit_identical(oa, ob));
        }
        CHECK(oa.rps_trip_latched != 0U);  // the scripted AZ-5 latched

        rbmk_destroy(a);
        rbmk_destroy(b);
    }
}
