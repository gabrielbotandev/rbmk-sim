# Model assumptions and limitations

Every simplification below is deliberate. The model's job is to make a small
set of qualitative phenomena visible and reproducible — not to be right about
numbers.

## Structural assumptions

| Area | Assumption | Consequence |
|------|------------|-------------|
| Neutronics | Point kinetics, six delayed groups (textbook data), one global reactivity | No spatial power tilts, no local criticality — the very effects that dominated the real accident's severity |
| Core discretization | N "channels" (default 12) that share a static radial profile modulated by bank insertion | A channel here is a visualization bucket, not a physical pressure tube |
| Thermal-hydraulics | First-order lags toward algebraic steady states for void, fuel and coolant temperature | No two-phase flow, no pressure dynamics, no critical heat flux, no pump models beyond a flow lag |
| Void model | A saturating curve of the power/flow ratio with decreasing slope | Calibrated for the *sign and shape* of feedback (stable at high power, unstable low-power band), nothing else |
| Poisons | I-135/Xe-135 chain in normalized units; public decay constants; toy burnup rate | Realistic *timescales* (hours) and *shape* (post-trip peak), toy magnitudes |
| Rods | Four banks (two manual, regulator, emergency); smoothstep absorber worth; parabolic displacer lobe for the 1986 design | Bank = many real rods averaged; the 18 s scram travel is the public figure, the worth magnitudes are toys |
| Protection | Five trips + sensor-count fault, fixed thresholds, 50 ms scan | An illustration of protection *structure*, not a protection design |
| Instrumentation | Ideal detectors plus optional seeded noise; smoothed period meter | No drift, no calibration, no failure modes beyond a validity flag |

## Known limitations (read before drawing conclusions)

1. **Magnitudes are not calibrated.** The AZ-5 power spike in this model is a
   few tens of percent; the real excursion was orders of magnitude and
   destroyed the core. The model rings down smoothly because it has no damage
   physics — the epilogue of the timeline says exactly where reality and toy
   part ways.
2. **The accident state is reached by scripted actions.** The timeline drives
   setpoints, rod targets, and flows; the real sequence involved regulation
   modes, grid events, and operator decisions the model cannot represent.
3. **One global reactivity.** The "positive scram" effect is modeled as a
   positive lobe in the bank worth curve — a faithful *qualitative* stand-in
   for the axial-flux/displacer interaction, but not its mechanism.
4. **Validity envelope.** Power is clamped to [1e-9, 100] of nominal and fuel
   temperature to 5000 °C. Crossing a clamp sets `validity_exceeded` in every
   observation and run log; beyond that point even qualitative claims weaken.
5. **Determinism is per binary.** Bit-exact replay is guaranteed on the
   binary that produced a log; across compilers, flags, or the
   Fortran/C++ numerics switch, agreement is to rounding only (the model
   version string in every log makes the build visible).

## Why simplified coefficients are used

Three reasons, in order of importance:

1. **Safety boundary.** Using public, textbook-level, deliberately
   non-operational values guarantees this repository cannot serve as input to
   any real engineering activity — there is nothing here to misuse.
2. **Pedagogy.** Five transparent constants (`constants.hpp`) teach more than
   five hundred opaque ones; every number can be read, questioned, and changed
   by a student in one file.
3. **Honesty.** A model this small cannot be quantitatively right, so it must
   not pretend to be. Keeping the coefficients visibly toy-grade prevents the
   most dangerous failure mode of educational software: unearned credibility.
