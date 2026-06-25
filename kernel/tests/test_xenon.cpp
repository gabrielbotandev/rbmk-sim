#include <doctest/doctest.h>

#include <cmath>

#include "rbmk/kernel/constants.hpp"
#include "rbmk/kernel/xenon.hpp"

namespace rk = rbmk::kernel;

TEST_SUITE("xenon") {
    TEST_CASE("equilibrium is stationary under constant power") {
        rk::XenonState s = rk::xenon_equilibrium(1.0);
        const double x0 = s.xenon;
        for (int i = 0; i < 3600; ++i) {
            rk::xenon_step(s, 1.0, 1.0);  // one hour, 1 s steps
        }
        CHECK(s.xenon == doctest::Approx(x0).epsilon(1e-9));
        CHECK(s.iodine == doctest::Approx(1.0).epsilon(1e-9));
    }

    TEST_CASE("nominal equilibrium matches the documented constant") {
        const rk::XenonState s = rk::xenon_equilibrium(1.0);
        CHECK(s.xenon == doctest::Approx(rk::kXenonEqAtNominal).epsilon(1e-12));
        CHECK(rk::xenon_reactivity(s) == doctest::Approx(rk::kXenonEqWorth).epsilon(1e-12));
    }

    TEST_CASE("shutdown produces the classic xenon peak then decay") {
        rk::XenonState s = rk::xenon_equilibrium(1.0);
        const double x0 = s.xenon;
        double peak = x0;
        double peak_time_h = 0.0;
        const double dt = 60.0;              // 1 min steps are ample for 1e-5/s rates
        for (int i = 0; i < 60 * 60; ++i) {  // 60 hours
            rk::xenon_step(s, 0.0, dt);
            if (s.xenon > peak) {
                peak = s.xenon;
                peak_time_h = static_cast<double>(i + 1) * dt / 3600.0;
            }
        }
        CHECK(peak / x0 > 1.3);    // visible poisoning bump
        CHECK(peak / x0 < 2.5);    // but qualitatively bounded
        CHECK(peak_time_h > 4.0);  // hours-scale dynamics
        CHECK(peak_time_h < 14.0);
        CHECK(s.xenon < 0.2 * x0);  // decayed away by 60 h
    }

    TEST_CASE("clean core builds toward equilibrium under power") {
        rk::XenonState s;  // zero iodine, zero xenon
        const double eq = rk::xenon_equilibrium(1.0).xenon;
        for (int i = 0; i < 100 * 3600 / 60; ++i) {  // 100 hours
            rk::xenon_step(s, 1.0, 60.0);
        }
        CHECK(s.xenon == doctest::Approx(eq).epsilon(1e-3));
    }

    TEST_CASE("active integrator matches the reference C++ implementation") {
        // When the build includes the Fortran numerics (RBMK_HAVE_FORTRAN),
        // xenon_step dispatches to Fortran and this is a true cross-language
        // parity check of the twin RK4 implementations. Without Fortran both
        // calls take the same path and the case documents the contract.
        rk::XenonState active = rk::xenon_equilibrium(1.0);
        rk::XenonState reference = active;
        for (int i = 0; i < 5000; ++i) {
            const double power = 0.5 + 0.5 * std::cos(static_cast<double>(i) * 0.01);
            rk::xenon_step(active, power, 30.0);
            rk::xenon_step_cpp(reference, power, 30.0);
            REQUIRE(active.iodine == doctest::Approx(reference.iodine).epsilon(1e-12));
            REQUIRE(active.xenon == doctest::Approx(reference.xenon).epsilon(1e-12));
        }
        CHECK(active.xenon > 0.0);
    }
}
