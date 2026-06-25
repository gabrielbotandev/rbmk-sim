// The flagship qualitative behavior: a scram commanded with nearly-withdrawn
// rods briefly ADDS reactivity in the 1986-style design (graphite displacers
// sweeping the lower core) before shutting down, while the modified design
// shuts down monotonically. This is the educational heart of the simulator.
#include <doctest/doctest.h>

#include "rbmk/kernel/core.hpp"

namespace rk = rbmk::kernel;

namespace {

rk::Config scram_config(rk::RodDesign design) {
    rk::Config cfg;
    cfg.rod_design = design;
    cfg.ar_enabled = false;
    cfg.initial_manual_rod_insertion = 0.05;  // rods nearly fully withdrawn
    return cfg;
}

struct ScramTrace {
    double peak_power = 0.0;
    double power_at_20s = 0.0;
};

ScramTrace run_scram(rk::RodDesign design) {
    rk::ReactorCore core(scram_config(design));
    core.command_scram();
    ScramTrace trace;
    rk::Observation obs;
    const std::uint32_t steps_20s = 400U;  // dt = 0.05 s
    for (std::uint32_t i = 0U; i < steps_20s; ++i) {
        core.step();
        core.observe(obs);
        if (obs.time_s <= 6.0 && obs.power_frac > trace.peak_power) {
            trace.peak_power = obs.power_frac;
        }
    }
    trace.power_at_20s = obs.power_frac;
    return trace;
}

}  // namespace

TEST_SUITE("scram") {
    TEST_CASE("1986 design: positive power excursion before shutdown") {
        const ScramTrace t = run_scram(rk::RodDesign::kOriginal1986);
        CHECK(t.peak_power > 1.02);    // visible bump above initial power
        CHECK(t.power_at_20s < 0.35);  // but the absorbers win in the end
    }

    TEST_CASE("modified design: monotonic shutdown, no positive excursion") {
        const ScramTrace t = run_scram(rk::RodDesign::kModified);
        CHECK(t.peak_power <= 1.002);  // no bump beyond numerical noise
        CHECK(t.power_at_20s < 0.35);
    }

    TEST_CASE("scram drives every bank to full insertion") {
        rk::ReactorCore core(scram_config(rk::RodDesign::kOriginal1986));
        core.command_scram();
        core.step_n(500);  // 25 s > 18 s full travel
        rk::Observation obs;
        core.observe(obs);
        CHECK(obs.scram_latched == 1U);
        for (std::size_t b = 0U; b < rk::kNumBanks; ++b) {
            CHECK(obs.rod_position[b] == doctest::Approx(1.0));
        }
    }
}
