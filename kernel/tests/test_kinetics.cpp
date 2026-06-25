#include <doctest/doctest.h>

#include "rbmk/kernel/core.hpp"
#include "rbmk/kernel/kinetics.hpp"

namespace rk = rbmk::kernel;

namespace {

rk::Config quiet_config() {
    rk::Config cfg;
    cfg.ar_enabled = false;  // keep the regulator out of open-loop tests
    return cfg;
}

}  // namespace

TEST_SUITE("kinetics") {
    TEST_CASE("equilibrium initialization is critical and flat") {
        rk::ReactorCore core(quiet_config());
        rk::Observation obs;
        core.observe(obs);
        CHECK(obs.power_frac == doctest::Approx(1.0).epsilon(1e-12));
        CHECK(obs.rho_total == doctest::Approx(0.0).epsilon(1e-12));

        core.step_n(2000);  // 100 s
        core.observe(obs);
        CHECK(obs.power_frac == doctest::Approx(1.0).epsilon(1e-6));
    }

    TEST_CASE("module-level precursor equilibrium holds at zero reactivity") {
        rk::PointKinetics kin(0.7);
        for (int i = 0; i < 1000; ++i) {
            kin.step(0.0, 0.05, rk::kKineticsSubsteps);
        }
        CHECK(kin.power() == doctest::Approx(0.7).epsilon(1e-6));
    }

    TEST_CASE("rod withdrawal raises power, reinsertion lowers it") {
        rk::ReactorCore core(quiet_config());
        core.set_rod_target(rk::Bank::kManualA, 0.25);  // withdraw from 0.35
        core.step_n(60);                                // 3 s: prompt rise under way
        rk::Observation obs;
        core.observe(obs);
        CHECK(obs.power_frac > 1.005);
        CHECK(obs.period_s > 0.0);  // period meter reads "rising" during the ramp

        core.set_rod_target(rk::Bank::kManualA, 0.6);  // insert beyond start
        core.step_n(1200);                             // 60 s
        rk::Observation after;
        core.observe(after);
        CHECK(after.power_frac < obs.power_frac);
    }

    TEST_CASE("power stays inside the validity envelope under extreme input") {
        rk::Config cfg = quiet_config();
        cfg.initial_manual_rod_insertion = 1.0;
        rk::ReactorCore core(cfg);
        // Full withdrawal of everything from deep insertion: large positive ramp.
        core.set_rod_target(rk::Bank::kManualA, 0.0);
        core.set_rod_target(rk::Bank::kManualB, 0.0);
        core.set_rod_target(rk::Bank::kAutomatic, 0.0);
        core.step_n(20000);  // 1000 s
        rk::Observation obs;
        core.observe(obs);
        CHECK(obs.power_frac <= rk::kMaxPowerFrac);
        CHECK(obs.power_frac >= rk::kMinPowerFrac);
    }
}
