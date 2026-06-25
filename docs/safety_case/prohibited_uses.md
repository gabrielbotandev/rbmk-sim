# Prohibited uses

RBMK-SIM is an educational software artifact. It **must not** be used for:

- **Reactor operation or operational decision support** of any kind.
- **Operator training** — the dynamics, magnitudes, interfaces, and procedures
  are all wrong for that purpose, by design.
- **Prediction or reconstruction** — the timeline mode is an annotated
  educational narrative, not an accident reconstruction.
- **Design, licensing, or regulatory work** — nothing in this repository is
  qualified, validated, or traceable to engineering data.
- **Real-world safety analysis** — the protection system here demonstrates
  *software structure* (latching, permissives, fail-safe defaults), not a
  protection design basis.
- **Deriving operational parameters** — every coefficient is a public,
  simplified teaching value; treating any of them as plant data is an error
  by construction.

## Why the project is safe as an educational artifact

The argument has three legs:

1. **Nothing to misuse.** All physics constants are textbook-level or
   invented toy values (see {doc}`../model/coefficients`); no proprietary,
   classified, or operationally precise data exists anywhere in the
   repository, and the model structure (point kinetics + first-order lags)
   could not host such data meaningfully anyway.
2. **Self-describing limits.** Every run log embeds a disclaimer and the
   model version; the dashboard, README, license file, and this safety case
   restate the boundary; the timeline UI labels every statement as HISTORICAL
   narrative or MODEL action.
3. **Qualitative by construction.** The model is honest about being wrong in
   magnitude everywhere except the qualitative shapes it exists to teach —
   and those shapes (positive void feedback, xenon transients, the 1986 rod
   worth lobe) are documented in public literature at far greater fidelity
   than this toy provides.
