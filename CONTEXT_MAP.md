# Context Map

Quick navigation for humans and agents. One line per area; read the linked context
before editing that area. The authoritative design text is the Sphinx documentation.

| Path | What lives here | Read first |
|------|-----------------|------------|
| `AGENTS.md` | Repository-wide rules: safety boundary, standards, workflows, git | — |
| `CMakeLists.txt`, `CMakePresets.json` | Superbuild for kernel, protection, orchestrator, Fortran, native tests | `docs/development/building.md` |
| `kernel/` | C++17 physics kernel: point kinetics, channels, voids, xenon, rods, detectors | `docs/model/` |
| `protection/` | C11 reactor protection system (RPS): trips, alarms, latched state machine | `docs/safety_case/protection_rationale.md` |
| `orchestrator/` | C++ coupling loop (sensors -> RPS -> actuators -> physics) and the versioned C ABI (`rbmk/capi/rbmk_capi.h`) | `docs/architecture/` |
| `fortran/` | Optional Fortran 2008 numerics (xenon/iodine integrator); C++ fallback in kernel | `docs/development/fortran.md` |
| `dashboard/` | Python package `rbmk_dash`: ctypes bindings, HDF5 recorder/replayer, PySide6 UI, CLIs | `dashboard/README.md` |
| `scenarios/` | Versioned scenario + timeline JSON (incl. Chernobyl educational timeline) | `docs/model/scenarios.md` |
| `specs/tla/` | TLA+ spec of the RPS state machine + TLC config | `specs/tla/README.md` |
| `docs/` | Sphinx documentation incl. the educational safety case | `docs/index.md` |
| `requirements.txt` | Python dependency set for dashboard, tests, docs | — |
| `.clang-format`, `.clang-tidy`, `.editorconfig` | Enforced style and static-analysis profiles | `docs/development/coding_standard.md` |

## Component contracts in one paragraph

The **kernel** owns all physics state and is deterministic and allocation-free per step.
The **protection** library is a pure C scan function over plain inputs producing trip/alarm
outputs and never reaches into kernel internals. The **orchestrator** is the only component
that couples them, in a fixed order each step, and the only component exporting a C ABI.
The **dashboard** (Python) talks exclusively to the C ABI via `ctypes`, records every run
to HDF5, and can replay any recorded run deterministically. **Scenarios** are data, not
code. **Specs** model the protection state machine only. **Docs** carry the safety case;
when code behavior and docs disagree, fix one of them in the same change.
