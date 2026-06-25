#include <doctest/doctest.h>

#include <vector>

#include "rbmk/kernel/constants.hpp"
#include "rbmk/kernel/thermal.hpp"

namespace rk = rbmk::kernel;

TEST_SUITE("thermal") {
    TEST_CASE("steady void responds to power and flow with the right signs") {
        const double nominal = rk::ThermalHydraulics::steady_void(1.0, 1.0);
        CHECK(nominal > 0.0);
        CHECK(nominal < 0.5);
        CHECK(rk::ThermalHydraulics::steady_void(1.5, 1.0) > nominal);  // more power
        CHECK(rk::ThermalHydraulics::steady_void(1.0, 0.7) > nominal);  // less flow
        CHECK(rk::ThermalHydraulics::steady_void(0.04, 1.0) == doctest::Approx(0.0));
        CHECK(rk::ThermalHydraulics::steady_void(10.0, 0.05) <= rk::kMaxVoidFrac);
    }

    TEST_CASE("steady initialization is a fixed point of step") {
        rk::ThermalHydraulics th(8U);
        const std::vector<double> power(8, 1.0);
        th.init_steady(power, 1.0);
        const double void0 = th.avg_void_frac();
        const double fuel0 = th.avg_fuel_temp_c();
        for (int i = 0; i < 1000; ++i) {
            th.step(power, 1.0, 0.05);
        }
        CHECK(th.avg_void_frac() == doctest::Approx(void0).epsilon(1e-9));
        CHECK(th.avg_fuel_temp_c() == doctest::Approx(fuel0).epsilon(1e-9));
        // At the nominal reference point both feedbacks are zero by definition.
        CHECK(th.void_reactivity() == doctest::Approx(0.0).epsilon(1e-9));
        CHECK(th.doppler_reactivity() == doctest::Approx(0.0).epsilon(1e-9));
    }

    TEST_CASE("power rise drives positive void feedback and negative Doppler") {
        rk::ThermalHydraulics th(8U);
        std::vector<double> power(8, 1.0);
        th.init_steady(power, 1.0);

        std::fill(power.begin(), power.end(), 1.4);
        for (int i = 0; i < 2000; ++i) {  // settle at the new state
            th.step(power, 1.0, 0.05);
        }
        CHECK(th.void_reactivity() > 0.0);     // positive void coefficient
        CHECK(th.doppler_reactivity() < 0.0);  // fuel heats, Doppler negative
    }

    TEST_CASE("flow reduction increases void at constant power") {
        rk::ThermalHydraulics th(8U);
        const std::vector<double> power(8, 1.0);
        th.init_steady(power, 1.0);
        const double void0 = th.avg_void_frac();
        for (int i = 0; i < 2000; ++i) {
            th.step(power, 0.7, 0.05);
        }
        CHECK(th.avg_void_frac() > void0);
        CHECK(th.void_reactivity() > 0.0);
    }
}
