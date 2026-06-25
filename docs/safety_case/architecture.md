# Software architecture

The system is layered so that determinism and protection logic never depend
on the frontend, and so each language is used where it is strongest.

```{mermaid}
flowchart LR
    subgraph native [Native core - librbmk_sim.so]
        kernel["C++17 physics kernel<br/>(rbmk_kernel, static)"]
        rps["C11 protection system<br/>(rbmk_rps, static)"]
        fort["Fortran 2008 numerics<br/>(optional, C++ twin fallback)"]
        orch["C++17 orchestrator<br/>fixed-order coupling loop"]
        fort --> kernel
        kernel --> orch
        rps --> orch
    end
    orch -->|"versioned C ABI (ctypes)"| pyCore["Python core layer<br/>bindings / session / recorder"]
    scenarios["Scenario JSON<br/>(timeline data)"] --> pyCore
    pyCore --> ui["PySide6 dashboard<br/>operate / timeline / comparison"]
    pyCore --> cli["rbmk-logtool CLI"]
    pyCore --> h5[("HDF5 run logs<br/>deterministic, replayable")]
    tla["TLA+ spec (TLC-checked)"] -.->|mirrors| rps
```

## Component responsibilities

| Component | Language | Responsibility | Must never |
|-----------|----------|----------------|------------|
| `kernel/` | C++17 | All physics state; deterministic fixed-step advance; observation snapshots | Allocate or throw in the step path; read wall clock; see the frontend |
| `protection/` | C11 | Pure scan: inputs → alarms/trips/state; latching; reset permissives | Reach into kernel internals; allocate; lose a latch |
| `orchestrator/` | C++17 | Fixed coupling order (sense → protect → actuate → step); the only C ABI exporter; the only heap allocation (handle) | Let an exception cross the ABI; reorder the loop |
| `fortran/` | F2008 | Selected numerics behind `bind(C)`; algorithmic twin of the C++ fallback | Diverge from the C++ reference (parity-tested) |
| `dashboard/` (core) | Python | ctypes mirror, session command log, HDF5 record/replay/verify | Touch physics; bypass the command log |
| `dashboard/` (ui) | Python | Presentation, controls, annunciators, timeline, comparison | Compute anything the kernel should |
| `scenarios/` | JSON | Versioned timeline data with historical/model separation | Contain executable logic |
| `specs/tla/` | TLA+ | Formal model of the protection state machine | Drift from `rps.c` (mapping table documented) |

## The coupling loop

Each step the orchestrator executes exactly:

1. **Sense** — kernel observation; conservative power reading (max over valid
   detectors); rods-full-in derived from positions.
2. **Protect** — one `rps_step` scan (50 ms cadence at default dt).
3. **Actuate** — scram command latches the kernel scram; an accepted reset
   releases it.
4. **Advance** — one kernel physics step.

This order is part of the determinism contract and is tested end-to-end
through the C ABI.

## Dependency rules

- The native core has **zero external runtime dependencies**; test frameworks
  (doctest, Unity) are fetched at pinned tags for builds with testing only.
- Python depends on the C ABI only through `bindings.py` — one file to audit
  for the language boundary.
- The ABI is versioned (`RBMK_ABI_VERSION`); the bindings refuse to load a
  mismatched library, and struct sizes are asserted on both sides.
