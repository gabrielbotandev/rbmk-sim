# Core physics model

Everything below is a deliberately simplified teaching model. Symbols refer
to the constants in
[`kernel/include/rbmk/kernel/constants.hpp`](../../kernel/include/rbmk/kernel/constants.hpp);
see {doc}`coefficients` for values and provenance.

## Point kinetics

Normalized power $n$ (1.0 = nominal) and six delayed-neutron precursor
groups $c_i$:

$$
\frac{dn}{dt} = \frac{\rho - \beta}{\Lambda}\,n + \sum_{i=1}^{6} \lambda_i c_i + S,
\qquad
\frac{dc_i}{dt} = \frac{\beta_i}{\Lambda}\,n - \lambda_i c_i
$$

with textbook U-235 thermal values for $\beta_i, \lambda_i$, a toy
generation time $\Lambda = 10^{-3}\,\mathrm{s}$, and a tiny constant source
$S$ so power never reaches exactly zero. Integration: classical RK4 with 20
fixed substeps per 50 ms macro step, placing even a deep-scram transient well
inside the stability region. Power is clamped to $[10^{-9}, 100]$ — a
validity envelope, not physics; crossing it sets `validity_exceeded`.

## Reactivity composition

$$
\rho = \rho_\text{base} + \rho_\text{rods} + \rho_\text{void}
     + \rho_\text{Doppler} + \rho_\text{xenon}
$$

$\rho_\text{base}$ is trimmed once at construction so the configured
initial state is exactly critical — it stands in for fuel excess reactivity
and stays constant during a run.

## Channels and thermal-hydraulics

Each of the $N$ channels (default 12) carries a static parabolic profile
weight, modulated by the local absorption of the manual bank serving it
(interleaved assignment). Channel power $q_i$ is the global power shared by
those weights, normalized to mean 1 at nominal.

Per channel, two first-order lags:

$$
\tau_\alpha \frac{d\alpha_i}{dt} = \alpha_\text{ss}(q_i, \phi) - \alpha_i,
\qquad
\tau_f \frac{dT_{f,i}}{dt} = \bigl(T_c + \Delta T_f\, q_i\bigr) - T_{f,i}
$$

with the steady void fraction a saturating function of the power/flow ratio
$r = q/\phi$:

$$
\alpha_\text{ss}(r) = \alpha_\text{max}\left(1 - e^{-k\,(r - r_0)}\right)^+
$$

The **decreasing slope** of this curve is the single most important shape in
the model: combined with the linear Doppler feedback it makes the net power
coefficient *negative at high power and positive in a low-power band*
(roughly $r \in (0.05, 0.35)$) — the qualitative RBMK characteristic that
made low-power operation hazardous.

Coolant temperature is a global first-order lag toward
$T_\text{in} + \Delta T_c \cdot \bar q/\phi$; pump flow follows its command
with a 5 s lag.

## Feedback

- **Void**: $\rho_\text{void} = c_v\,(\bar\alpha - \bar\alpha_\text{ref})$
  with $c_v > 0$ — the positive void coefficient.
- **Doppler**: $\rho_\text{D} = c_D\,(\bar T_f - T_\text{ref})$ with
  $c_D < 0$.
- **Xenon**: see {doc}`xenon`.
- **Rods**: see {doc}`rods`.

## Step order (fixed, part of the determinism contract)

1. automatic regulator adjusts the AUTO bank target (deadband + slew bound +
   25 s period governor)
2. rod banks move (scram speed 1/18 s⁻¹; normal bank slew 1/120 s⁻¹)
3. flow lags toward its command
4. reactivity components are evaluated
5. point kinetics advances (ρ frozen across the macro step)
6. iodine/xenon chain advances
7. channel powers and thermal-hydraulics update
8. instrumentation updates
