# Control-rod model and the graphite displacer effect

## Banks

| Bank | Worth (toy) | Displacer (1986 design) | Role |
|------|-------------|--------------------------|------|
| Manual A | −0.020 | yes | coarse reactivity management |
| Manual B | −0.020 | yes | coarse reactivity management |
| Automatic (AR) | −0.018 | no | power regulation (deadband + period governor) |
| Emergency (AZ) | −0.035 | yes | normally parked out; scram |

A bank stands for many real rods moved a few at a time, which is why normal
bank slew is slow (full travel 120 s) while a **scram drives every bank at
the public ~18 s full-travel figure** — the slowness of the RBMK scram is part
of the story being taught.

## Worth curves

Insertion fraction $x \in [0,1]$ (0 = fully withdrawn). The absorber part
is a smoothstep, $A(x) = x^2(3-2x)$; the 1986-design displacer adds a
positive lobe $D(x) = \tfrac{27}{4}\,x\,(1-x)^2$ (peak 1.0 at $x=1/3$):

$$
W_{1986}(x) = w_\text{bank}\,A(x) + w_\text{tip}\,D(x),
\qquad
W_\text{mod}(x) = w_\text{bank}\,A(x)
$$

with $w_\text{bank} < 0$ and $w_\text{tip} > 0$. Qualitative meaning: in
the 1986 design, inserting a rod from the fully withdrawn position first
drives its graphite displacer through the lower core, displacing absorbing
water and **adding** reactivity until the boron absorber follows. The
modified design is monotonically negative — every centimetre of travel
removes reactivity.

Both designs share the same fully-inserted worth, so the comparison isolates
the *shape* of the curve, not its magnitude. The same scram from a
nearly-withdrawn, void-prone state then produces a brief power excursion in
the 1986 design and an immediate decrease in the modified one — reproduced
live in the dashboard's Design comparison tab and pinned by regression tests.

A subtle consequence students can discover: near the top of travel the 1986
curve is non-monotonic, so *withdrawing* a slightly-inserted rod can
momentarily *remove* reactivity (the inverse of the scram paradox).

## Spatial coupling (toy)

A fully inserted manual bank suppresses the power share of the channels it
serves by 12%, giving rod moves a mild, deterministic spatial signature in
the channel displays without pretending to be spatial kinetics.
