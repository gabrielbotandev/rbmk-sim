# Agent and Contributor Instructions

These instructions apply to the whole repository. More specific instructions in
subdirectories (if any) take precedence over this file for files they cover.

## What this project is

An **educational, qualitative** RBMK-1000-*inspired* reactor simulator focused on software
engineering, safety-case documentation, and deterministic simulation. It is a toy/sandbox.

## Safety boundary (non-negotiable)

- All physical coefficients are **simplified, public, textbook-level, non-operational**.
- Never introduce proprietary, classified, export-controlled, or operationally precise
  nuclear engineering data, and never tune the model toward operational fidelity.
- Every model document must state that the simulator is unsuitable for reactor operation,
  prediction, licensing, training of operators, or real-world safety analysis.
- The Chernobyl timeline content is an educational narrative based on public summaries
  (e.g. IAEA INSAG-7); keep historical narrative clearly separated from model behavior.

## Navigation

Read `CONTEXT_MAP.md` first for the directory map and per-component context. The full
design rationale lives in the Sphinx docs under `docs/` (safety case included).

## Languages and standards

| Area                | Language / standard | Notes |
|---------------------|---------------------|-------|
| Simulation kernel   | C++17               | No exceptions across the C ABI; no dynamic allocation in the per-step path |
| Protection system   | C11                 | MISRA-inspired subset, see `docs/development/coding_standard.md` |
| Orchestrator + C ABI| C++17 / C11 ABI     | Versioned, flat, fixed-size structs only |
| Numerics            | Fortran 2008        | Optional at build time; C++ fallback must stay in algorithmic lockstep |
| Dashboard / tooling | Python >= 3.12      | PySide6 + pyqtgraph + h5py; `ruff` clean |
| Formal specs        | TLA+                | Small, readable, TLC-checked |

## Determinism rules (apply to kernel, protection, orchestrator)

- Fixed time step, fixed iteration order, single-threaded simulation core.
- No wall-clock, locale, or environment dependence inside simulation code.
- Randomness only from the seeded in-repo PRNG, and only for detector noise (off by default).
- Identical config + identical command sequence must produce bit-identical observations
  on the same binary. Tests enforce this; do not break them.

## MISRA-inspired C/C++ rules (summary; full text in docs)

- Fixed-width integer types from `<stdint.h>` / `<cstdint>` in interfaces.
- No recursion, no `malloc`/`free` (or `new`/`delete`) in steady-state simulation paths.
- All loops bounded; all `switch` statements have `default`; no fallthrough without comment.
- Validate all pointers and ranges at API boundaries; clamp rather than trust.
- Deviations require a `NOTE(misra-dev): <justification>` comment at the site.

## Native workflows (do not wrap these in scripts)

- Configure/build: `cmake --preset dev` then `cmake --build --preset dev`
- C/C++ tests: `ctest --preset dev`
- Python env: `python3 -m venv .venv && .venv/bin/pip install -r requirements.txt`
- Python tests: `.venv/bin/python -m pytest dashboard/tests`
- Lint: `.venv/bin/ruff check dashboard`, `clang-format --dry-run -Werror <files>`,
  `clang-tidy` via `cmake --preset tidy` (when configured)
- Docs: `.venv/bin/sphinx-build -W -b html docs docs/_build/html`
- TLA+: `java -jar specs/tla/tools/tla2tools.jar -config RPS.cfg RPS.tla` (run inside `specs/tla/`)
- Dashboard app: `.venv/bin/python -m rbmk_dash` (after `pip install -e dashboard`)

Do **not** add helper/wrapper scripts (shell, Make, task runners) for running, building,
testing, or managing the project. Use the commands above. Scripts are allowed only when
they are themselves a deliverable.

## Git conventions

- Commits are authored by the repository owner (check `git var GIT_AUTHOR_IDENT` first;
  it must not be an agent identity).
- Small, meaningful, milestone-aligned commits; imperative subject line <= 72 chars;
  body explains the why when non-obvious.
- Do not push, rebase, or amend without an explicit request.

## Plans

Plans are kept in `.cursor/plans/` when writable; the active project plan is `PLAN.md`
at the repository root. Keep plan status updates out of code commits unless the plan
itself changed.
