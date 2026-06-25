// Determinism guarantees: identical configuration plus identical command
// sequences produce bit-identical observations on the same binary.
#include <doctest/doctest.h>

#include <cstring>

#include "rbmk/kernel/core.hpp"

namespace rk = rbmk::kernel;

namespace {

// NOTE(misra-dev): Observation is laid out without interior padding (see
// types.hpp), so the object representation is unique and the bytewise
// comparison is exactly the determinism property under test.
bool bit_identical(const rk::Observation& a, const rk::Observation& b) {
    // NOLINTNEXTLINE(bugprone-suspicious-memory-comparison,cert-exp42-c,cert-flp37-c)
    return std::memcmp(&a, &b, sizeof(rk::Observation)) == 0;
}

// A representative scripted session touching every control input.
void drive(rk::ReactorCore& core, std::uint32_t step) {
    switch (step) {
    case 100U:
        core.set_rod_target(rk::Bank::kManualA, 0.25);
        break;
    case 400U:
        core.set_pump_flow(0.8);
        break;
    case 800U:
        core.set_power_setpoint(0.9);
        break;
    case 1200U:
        core.set_rod_target(rk::Bank::kManualB, 0.5);
        break;
    case 1800U:
        core.command_scram();
        break;
    default:
        break;
    }
}

}  // namespace

TEST_SUITE("determinism") {
    TEST_CASE("two identically driven cores stay bit-identical") {
        const rk::Config cfg;  // defaults: AR enabled — include the regulator path
        rk::ReactorCore a(cfg);
        rk::ReactorCore b(cfg);
        rk::Observation oa;
        rk::Observation ob;

        for (std::uint32_t step = 0U; step < 2500U; ++step) {
            drive(a, step);
            drive(b, step);
            a.step();
            b.step();
            if ((step % 250U) == 0U) {
                a.observe(oa);
                b.observe(ob);
                REQUIRE(bit_identical(oa, ob));
            }
        }
        a.observe(oa);
        b.observe(ob);
        CHECK(bit_identical(oa, ob));
        CHECK(oa.scram_latched == 1U);
    }

    TEST_CASE("detector noise is reproducible for equal seeds") {
        rk::Config cfg;
        cfg.detector_noise = true;
        cfg.noise_seed = 12345U;
        rk::ReactorCore a(cfg);
        rk::ReactorCore b(cfg);
        a.step_n(500);
        b.step_n(500);
        rk::Observation oa;
        rk::Observation ob;
        a.observe(oa);
        b.observe(ob);
        CHECK(bit_identical(oa, ob));
    }

    TEST_CASE("different noise seeds change readings but not physics") {
        rk::Config cfg;
        cfg.detector_noise = true;
        cfg.noise_seed = 1U;
        rk::Config cfg2 = cfg;
        cfg2.noise_seed = 2U;

        rk::ReactorCore a(cfg);
        rk::ReactorCore b(cfg2);
        a.step_n(500);
        b.step_n(500);
        rk::Observation oa;
        rk::Observation ob;
        a.observe(oa);
        b.observe(ob);
        CHECK(oa.detector_power_frac[0] != ob.detector_power_frac[0]);
        CHECK(oa.power_frac == ob.power_frac);  // physics never sees the noise
        CHECK(oa.rho_total == ob.rho_total);
    }

    TEST_CASE("rod design changes dynamics but not the determinism contract") {
        rk::Config c86;
        c86.rod_design = rk::RodDesign::kOriginal1986;
        c86.initial_manual_rod_insertion = 0.05;
        c86.ar_enabled = false;
        rk::Config cmod = c86;
        cmod.rod_design = rk::RodDesign::kModified;

        rk::ReactorCore a(c86);
        rk::ReactorCore b(cmod);
        a.command_scram();
        b.command_scram();
        a.step_n(100);  // 5 s into the scram
        b.step_n(100);
        rk::Observation oa;
        rk::Observation ob;
        a.observe(oa);
        b.observe(ob);
        CHECK(oa.power_frac != ob.power_frac);  // designs genuinely differ
    }
}
