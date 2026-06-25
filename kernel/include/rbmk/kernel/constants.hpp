// Model coefficients for the RBMK-SIM educational kernel.
//
// EVERY coefficient in the simulator lives in this file so the model is fully
// transparent. All values are simplified, public, textbook-level numbers chosen
// for qualitative behavior only. They are deliberately NON-OPERATIONAL: they do
// not describe any real reactor and are unsuitable for operation, prediction,
// licensing, or safety analysis of any kind. See docs/model/coefficients.md for
// the full provenance table.
#ifndef RBMK_KERNEL_CONSTANTS_HPP
#define RBMK_KERNEL_CONSTANTS_HPP

#include <array>
#include <cstdint>

namespace rbmk::kernel {

// ---------------------------------------------------------------------------
// Global structure
// ---------------------------------------------------------------------------
inline constexpr std::uint32_t kMaxChannels = 64U;
inline constexpr std::uint32_t kMinChannels = 4U;
inline constexpr std::uint32_t kDefaultChannels = 12U;
inline constexpr std::uint32_t kNumBanks = 4U;
inline constexpr std::uint32_t kNumDetectors = 4U;
inline constexpr std::uint32_t kNumDelayedGroups = 6U;

// Nominal thermal power used only to convert the normalized power fraction to
// megawatts for display (RBMK-1000 nominal thermal power, public figure).
inline constexpr double kNominalPowerMw = 3200.0;

// Fixed integration step (seconds) and bounds accepted from configuration.
inline constexpr double kDefaultDtS = 0.05;
inline constexpr double kMinDtS = 0.001;
inline constexpr double kMaxDtS = 0.5;

// Point-kinetics substeps per macro step. Chosen so the stiffest plausible
// transient (deep scram, rho ~ -0.08) keeps |rho - beta| / Lambda * dt_sub
// well inside the RK4 stability region.
inline constexpr std::uint32_t kKineticsSubsteps = 20U;

// ---------------------------------------------------------------------------
// Point kinetics (six delayed-neutron groups)
// Textbook U-235 thermal-fission values (Keepin); standard teaching data.
// ---------------------------------------------------------------------------
inline constexpr std::array<double, kNumDelayedGroups> kBeta = {0.000215, 0.001424, 0.001274,
                                                                0.002568, 0.000748, 0.000273};

inline constexpr std::array<double, kNumDelayedGroups> kLambdaPerS = {0.0124, 0.0305, 0.111,
                                                                      0.301,  1.14,   3.01};

inline constexpr double kBetaTotal =
    kBeta[0] + kBeta[1] + kBeta[2] + kBeta[3] + kBeta[4] + kBeta[5];

// Prompt-neutron generation time. Toy value on the long side, loosely evoking a
// graphite-moderated thermal core; chosen for stable, watchable dynamics.
inline constexpr double kGenerationTimeS = 1.0e-3;

// Tiny constant neutron source (normalized units) so power never reaches zero
// and restart behavior is smooth. Purely a numerical/educational device.
inline constexpr double kNeutronSourcePerS = 1.0e-10;

// Bounds on normalized power: explicit model-validity envelope, not physics.
inline constexpr double kMinPowerFrac = 1.0e-9;
inline constexpr double kMaxPowerFrac = 100.0;

// ---------------------------------------------------------------------------
// Control rods
// Four banks: two manual groups, one automatic regulator, one emergency (AZ).
// Worths are toy values; the ALL-banks total (~ -0.081, i.e. about -12.5 beta)
// merely guarantees comfortable shutdown margin in the toy model.
// ---------------------------------------------------------------------------
inline constexpr std::array<double, kNumBanks> kBankWorth = {-0.020, -0.020, -0.018, -0.035};

// Banks fitted with graphite displacers ("tips") in the 1986-style design:
// both manual groups and the emergency bank; the automatic regulator is not.
inline constexpr std::array<bool, kNumBanks> kBankHasDisplacer = {true, true, false, true};

// Peak positive reactivity contributed by one displacer-fitted bank while its
// graphite column passes through the lower core (1986-style design only).
// Toy value sized so a scram from nearly-withdrawn rods produces a visible
// qualitative power bump before the absorbers dominate.
inline constexpr double kDisplacerTipWorth = +0.0035;

// Scram rod speed: ~18 s for complete insertion (public INSAG-7 figure for
// the slow RBMK rod drive; identical for both designs so the comparison
// isolates the worth-curve shape).
inline constexpr double kRodSpeedPerS = 1.0 / 18.0;

// Normal (non-scram) bank slew: deliberately slower than the drive speed.
// A "bank" in this model stands for many real rods that operators moved a
// few at a time, so the bank-average insertion changes slowly.
inline constexpr double kManualRodSpeedPerS = 1.0 / 120.0;

// Fraction by which a fully inserted bank suppresses the power share of the
// channels it serves (toy spatial coupling).
inline constexpr double kLocalAbsorption = 0.12;

// ---------------------------------------------------------------------------
// Iodine-135 / xenon-135 chain (normalized units)
// Decay constants are public physical constants; the burnup rate is a toy
// "high flux" value chosen so the post-shutdown xenon peak is clearly visible
// (factor ~1.8 around 8 hours after a trip from full power).
// ---------------------------------------------------------------------------
inline constexpr double kLambdaIodinePerS = 2.87e-5;  // T1/2 ~ 6.7 h
inline constexpr double kLambdaXenonPerS = 2.09e-5;   // T1/2 ~ 9.2 h
inline constexpr double kXenonBurnupPerS = 7.0e-5;    // at full power (toy)
// Direct-from-fission xenon yield relative to the iodine path (~5% / 95%).
inline constexpr double kXenonDirectYieldPerS = kLambdaIodinePerS * (0.05 / 0.95);

// Equilibrium xenon level at full power in these normalized units.
inline constexpr double kXenonEqAtNominal =
    (kXenonDirectYieldPerS + kLambdaIodinePerS) / (kLambdaXenonPerS + kXenonBurnupPerS);

// Reactivity worth of equilibrium full-power xenon (textbook ballpark ~ -2.6%).
inline constexpr double kXenonEqWorth = -0.026;

// ---------------------------------------------------------------------------
// Thermal-hydraulics (per-channel first-order lags; all toy values)
// ---------------------------------------------------------------------------
inline constexpr double kInletTempC = 270.0;     // public RBMK-ish inlet temperature
inline constexpr double kCoolantDeltaTC = 14.0;  // core temperature rise at nominal
inline constexpr double kFuelDeltaTC = 700.0;    // fuel-to-coolant offset at nominal
inline constexpr double kFuelTimeConstS = 8.0;
inline constexpr double kCoolantTimeConstS = 6.0;
inline constexpr double kVoidTimeConstS = 4.0;
inline constexpr double kFlowTimeConstS = 5.0;  // pump/loop response lag
inline constexpr double kMinFlowFrac = 0.05;
inline constexpr double kMaxFlowFrac = 1.5;
inline constexpr double kMaxFuelTempC = 5000.0;  // validity clamp, not physics

// Steady-state void fraction: a saturating curve of the power/flow ratio r,
//   alpha_ss(r) = alpha_max * (1 - exp(-shape * (r - onset)))   for r > onset.
// The DECREASING slope with power is what makes the toy core stable at high
// power but unstable in a low-power band (onset < r < ~0.35 with the Doppler
// coefficient below), echoing the qualitative RBMK characteristic that
// low-power operation was restricted.
inline constexpr double kVoidAlphaMax = 0.40;
inline constexpr double kVoidOnsetRatio = 0.05;
inline constexpr double kVoidShape = 1.0;
inline constexpr double kMaxVoidFrac = 0.95;

// ---------------------------------------------------------------------------
// Reactivity feedback coefficients (toy, sign-correct, magnitude qualitative)
// ---------------------------------------------------------------------------
// Positive void coefficient: the defining RBMK instability, exaggerated only
// as far as needed to make the feedback loop visible on a dashboard.
inline constexpr double kVoidCoeffPerVoid = +0.045;
// Negative fuel-temperature (Doppler) coefficient. Sized so the net power
// coefficient is mildly negative at nominal power (stable) yet positive in
// the low-power void band (unstable) - the educational RBMK trap.
inline constexpr double kDopplerCoeffPerC = -2.0e-5;

// Radial channel power profile: w_i = 1 - kProfileShape * x_i^2, x_i in [-1/2, 1/2].
inline constexpr double kProfileShape = 0.6;

// ---------------------------------------------------------------------------
// Automatic power regulator: deadband + bounded proportional step on the
// AUTO bank target. No integral term - within the deadband the bank holds,
// which avoids limit-cycle dithering against the rod-speed slew limit.
// A period governor (startup rate limiter) stops further withdrawal while
// the reactor period is shorter than the hold threshold.
// ---------------------------------------------------------------------------
inline constexpr double kArKp = 1.0;
inline constexpr double kArDeadband = 0.002;    // power-fraction error tolerance
inline constexpr double kArMaxStep = 0.05;      // bound on one target adjustment
inline constexpr double kArHoldPeriodS = 25.0;  // no withdrawal below this period

// ---------------------------------------------------------------------------
// Instrumentation
// ---------------------------------------------------------------------------
inline constexpr double kDetectorNoiseSigma = 0.005;  // 0.5% when noise enabled
inline constexpr double kPeriodEmaAlpha = 0.05;       // period-meter smoothing (~1 s)
inline constexpr double kMaxPeriodS = 9999.0;         // display/trip clamp

}  // namespace rbmk::kernel

#endif  // RBMK_KERNEL_CONSTANTS_HPP
