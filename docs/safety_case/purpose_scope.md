# Purpose and scope

## What this project is for

RBMK-SIM exists to teach **software engineering for safety-related systems**
using a famous, well-documented piece of engineering history as its subject.
The learning objectives are:

1. **Deterministic simulation engineering** — fixed-step kernels, reproducible
   runs, bit-exact replay, and the discipline needed to keep them that way.
2. **Protection-system thinking** — limits, alarms, latched trips, fail-safe
   defaults, reset permissives, and safe-state transitions, expressed in a
   MISRA-inspired C subset and mirrored by a TLC-checked TLA+ model.
3. **Qualitative reactor phenomenology** — why a positive void coefficient,
   xenon poisoning, and a non-monotonic rod worth curve interact the way they
   did at Chernobyl, at the level of public summaries such as IAEA INSAG-7.
4. **Safety-case writing** — this document set itself is a deliverable: an
   exercise in stating assumptions, limitations, and verification evidence.

## What this project is

- A **toy, channel-inspired point-kinetics model** with per-channel
  thermal-hydraulic lags, six delayed-neutron groups, an iodine/xenon chain,
  rod banks with position-dependent worth curves, and simple instrumentation.
- A **simplified protection layer** (C11) coupled to the kernel by a
  deterministic orchestrator exporting a versioned C ABI.
- An **engineering dashboard** (PySide6) with live trends, an accident
  timeline replay, and a rod-design comparison mode.
- A **deterministic logging system** (HDF5) with bit-exact replay
  verification.

## Scope boundary

The simulator models *one* qualitative storyline well: how a reactor with a
positive void coefficient at low power, a poisoned core, and a flawed scram
design can briefly add reactivity when shut down. Everything else — spatial
kinetics, two-phase flow, fuel behavior, structural response, radiological
consequences — is out of scope and deliberately absent.

The intended audience is software engineers and students. The intended use is
reading the code, running the dashboard, breaking the model, and studying how
the safety arguments in this document set are constructed. Any use beyond
that — see {doc}`prohibited_uses` — is outside the design basis of this
project.
