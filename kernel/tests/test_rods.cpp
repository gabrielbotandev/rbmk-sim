#include <doctest/doctest.h>

#include "rbmk/kernel/rods.hpp"

namespace rk = rbmk::kernel;

TEST_SUITE("rods") {
    TEST_CASE("absorber fraction is a monotonic smoothstep") {
        CHECK(rk::RodSystem::absorber_fraction(0.0) == doctest::Approx(0.0));
        CHECK(rk::RodSystem::absorber_fraction(1.0) == doctest::Approx(1.0));
        double prev = 0.0;
        for (int i = 1; i <= 100; ++i) {
            const double x = static_cast<double>(i) / 100.0;
            const double a = rk::RodSystem::absorber_fraction(x);
            CHECK(a >= prev);
            prev = a;
        }
    }

    TEST_CASE("1986 design has a positive worth lobe at low insertion") {
        const double w_shallow =
            rk::RodSystem::bank_worth(rk::RodDesign::kOriginal1986, rk::Bank::kManualA, 0.10);
        CHECK(w_shallow > 0.0);  // graphite displacer adds reactivity first
        const double w_full =
            rk::RodSystem::bank_worth(rk::RodDesign::kOriginal1986, rk::Bank::kManualA, 1.0);
        CHECK(w_full == doctest::Approx(rk::kBankWorth[0]));  // displacer gone at x = 1
    }

    TEST_CASE("modified design is monotonically negative") {
        double prev = 0.0;
        for (int i = 0; i <= 100; ++i) {
            const double x = static_cast<double>(i) / 100.0;
            const double w =
                rk::RodSystem::bank_worth(rk::RodDesign::kModified, rk::Bank::kManualA, x);
            CHECK(w <= 0.0);
            CHECK(w <= prev + 1e-15);  // never adds reactivity while inserting
            prev = w;
        }
    }

    TEST_CASE("automatic bank has no displacer in either design") {
        for (int i = 0; i <= 10; ++i) {
            const double x = static_cast<double>(i) / 10.0;
            const double w86 =
                rk::RodSystem::bank_worth(rk::RodDesign::kOriginal1986, rk::Bank::kAutomatic, x);
            const double wmod =
                rk::RodSystem::bank_worth(rk::RodDesign::kModified, rk::Bank::kAutomatic, x);
            CHECK(w86 == doctest::Approx(wmod));
        }
    }

    TEST_CASE("normal moves use the slow bank slew, scram uses drive speed") {
        rk::RodSystem rods(rk::RodDesign::kModified, 0.0);
        rods.set_target(rk::Bank::kManualA, 1.0);
        rods.step(1.0);  // one second of normal motion
        CHECK(rods.position(rk::Bank::kManualA) == doctest::Approx(rk::kManualRodSpeedPerS));

        rods.command_scram();
        rods.step(1.0);  // one second of scram motion
        CHECK(rods.position(rk::Bank::kManualA) ==
              doctest::Approx(rk::kManualRodSpeedPerS + rk::kRodSpeedPerS));
        // Scram completes full travel in ~18 s.
        for (int i = 0; i < 20; ++i) {
            rods.step(1.0);
        }
        CHECK(rods.position(rk::Bank::kManualA) == doctest::Approx(1.0));
    }

    TEST_CASE("scram latch overrides targets until released") {
        rk::RodSystem rods(rk::RodDesign::kOriginal1986, 0.2);
        rods.command_scram();
        CHECK(rods.scram_latched());
        rods.set_target(rk::Bank::kManualA, 0.0);  // must be ignored
        CHECK(rods.target(rk::Bank::kManualA) == doctest::Approx(1.0));

        rods.release_scram();
        CHECK_FALSE(rods.scram_latched());
        rods.set_target(rk::Bank::kManualA, 0.3);  // accepted again
        CHECK(rods.target(rk::Bank::kManualA) == doctest::Approx(0.3));
    }
}
