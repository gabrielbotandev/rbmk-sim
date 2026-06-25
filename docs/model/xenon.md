# Iodine and xenon dynamics

Normalized two-nuclide chain (I-135 → Xe-135), the slow subsystem that gives
the simulator its hours-scale storylines:

$$
\frac{dI}{dt} = \lambda_I\,(P - I)
\qquad
\frac{dX}{dt} = g_X P + \lambda_I I - \lambda_X X - \sigma_b P X
$$

- Units are normalized so iodine equilibrium equals the power fraction
  ($I_\text{eq} = P$).
- $\lambda_I, \lambda_X$ are the public physical decay constants
  (half-lives ≈ 6.7 h and 9.2 h).
- $g_X$ sets the direct-from-fission yield at ~5% of the iodine path.
- $\sigma_b$ is a toy "high-flux" burnup rate chosen so the qualitative
  behaviors below are clearly visible.
- $\rho_\text{xenon} = w_\text{Xe}\,X/X_\text{eq,nom}$ with
  $w_\text{Xe} = -0.026$ (textbook ballpark for equilibrium xenon worth).

Integration: RK4 at the kernel macro step — the chain's rates (~10⁻⁵ s⁻¹)
make this trivially stable. The integrator is the designated Fortran seam:
`xenon_step` dispatches to `fortran/xenon_kinetics.f90` when built with a
Fortran compiler, with a parity-tested C++ twin otherwise
(see {doc}`../development/fortran`).

## Behaviors the model reproduces qualitatively

1. **Equilibrium poisoning** — at constant power, xenon settles at a
   power-dependent equilibrium (worth ≈ −4 β at nominal in this toy).
2. **Post-shutdown peak** — after a trip from full power, xenon rises to
   ≈ 1.8× equilibrium around 8 hours before decaying away: the classic
   "iodine pit" that can prevent restart.
3. **Post-reduction transient** — after a power *reduction*, xenon first
   climbs (iodine inventory still decaying at the old rate, burnup halved)
   then settles lower: the transient the operators at Chernobyl were fighting
   through 25 April 1986, visible in the timeline replay as the AR bank
   drifting outward and back.

The timeline tests pin behaviors 1 and 3; the kernel unit tests pin all
three at module level (peak ratio, peak timing window, decay).
