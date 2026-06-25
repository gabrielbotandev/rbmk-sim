# Coefficient table and provenance

Single source of truth:
[`kernel/include/rbmk/kernel/constants.hpp`](../../kernel/include/rbmk/kernel/constants.hpp)
(the Fortran twin mirrors its subset and is parity-tested). Three provenance
classes:

- **public-textbook** — standard teaching data found in any reactor-physics
  textbook; not plant-specific.
- **public-figure** — a widely published, non-operational figure used for
  flavor (e.g. nominal thermal power).
- **toy** — invented for qualitative behavior; no real-world meaning at all.

| Constant | Value | Class | Role |
|----------|-------|-------|------|
| `kBeta[6]` | 0.000215 … 0.000273 (Σ = 0.0065) | public-textbook | delayed-neutron fractions (U-235 thermal, Keepin) |
| `kLambdaPerS[6]` | 0.0124 … 3.01 s⁻¹ | public-textbook | precursor decay constants |
| `kGenerationTimeS` | 1.0e-3 s | toy | prompt generation time, long side for watchable dynamics |
| `kNeutronSourcePerS` | 1e-10 | toy | numerical source; smooth restarts |
| `kNominalPowerMw` | 3200 | public-figure | display conversion only |
| `kBankWorth[4]` | −0.020/−0.020/−0.018/−0.035 | toy | bank reactivity spans |
| `kDisplacerTipWorth` | +0.0035 | toy | the 1986 positive lobe, sized for a visible scram bump |
| `kRodSpeedPerS` | 1/18 s⁻¹ | public-figure | ~18 s full-travel scram (INSAG-7 level) |
| `kManualRodSpeedPerS` | 1/120 s⁻¹ | toy | bank-average slew (a bank = many rods) |
| `kLocalAbsorption` | 0.12 | toy | channel-share suppression by inserted bank |
| `kLambdaIodinePerS` | 2.87e-5 s⁻¹ | public-textbook | I-135 decay (T½ ≈ 6.7 h) |
| `kLambdaXenonPerS` | 2.09e-5 s⁻¹ | public-textbook | Xe-135 decay (T½ ≈ 9.2 h) |
| `kXenonBurnupPerS` | 7.0e-5 s⁻¹ | toy | burnup at nominal flux; sets peak ratio ≈ 1.8 |
| `kXenonEqWorth` | −0.026 | public-textbook (ballpark) | equilibrium xenon worth |
| `kInletTempC` / `kCoolantDeltaTC` | 270 / 14 °C | public-figure | RBMK-ish coolant temperatures |
| `kFuelDeltaTC` | 700 °C | toy | fuel-to-coolant offset at nominal |
| `kFuelTimeConstS` / `kCoolantTimeConstS` / `kVoidTimeConstS` / `kFlowTimeConstS` | 8 / 6 / 4 / 5 s | toy | first-order lags |
| `kVoidAlphaMax`, `kVoidOnsetRatio`, `kVoidShape` | 0.40, 0.05, 1.0 | toy | saturating void curve (stable high-power, unstable low-power band) |
| `kVoidCoeffPerVoid` | +0.045 | toy | positive void coefficient (sign is the point) |
| `kDopplerCoeffPerC` | −2.0e-5 °C⁻¹ | toy | negative fuel-temperature coefficient |
| `kArKp`, `kArDeadband`, `kArMaxStep`, `kArHoldPeriodS` | 1.0, 0.002, 0.05, 25 s | toy | regulator tuning incl. period governor |
| `kDetectorNoiseSigma` | 0.005 | toy | optional seeded detector noise |
| `kMaxPeriodS` | 9999 s | toy | period meter clamp ("stable") |
| `kMinPowerFrac`/`kMaxPowerFrac`, `kMaxFuelTempC` | 1e-9/100, 5000 °C | toy | validity envelope clamps |
| RPS setpoints (`RPS_*` in `rps.h`) | see {doc}`../safety_case/protection_rationale` | toy | protection thresholds with hysteresis |

**Why every "toy" entry is safe to publish**: each is either an arbitrary
round number chosen for didactic shape, or a magnitude deliberately far from
any calibrated value; none was derived from operational data. The
public-textbook entries are exactly that — standard academic teaching data.
