# Contributing to RBMK-SIM

Thank you for your interest in this project. RBMK-SIM is an **educational,
qualitative** RBMK-1000-*inspired* reactor simulator focused on software
engineering for safety-related systems — simulation kernels, protection logic,
formal specification, deterministic logging, and safety-case documentation.

## Safety boundary (non-negotiable)

> **This is an educational toy. It is not a reactor model.**
>
> - All physical coefficients are simplified, public, textbook-level values chosen for
>   qualitative behavior only. They are deliberately **non-operational**.
> - The simulator must **not** be used for reactor operation, operator training,
>   prediction, design, licensing, or any real-world safety analysis.
> - The Chernobyl timeline mode is an educational narrative based on public summaries
>   (IAEA INSAG-7 level); the model output alongside it is qualitative illustration,
>   not reconstruction.

Contributors must also:

- Never introduce proprietary, classified, export-controlled, or operationally
  precise nuclear engineering data, and never tune the model toward operational
  fidelity.
- Keep historical narrative clearly separated from model behavior.
- Restate the educational-use boundary in model and safety-case documents where
  relevant. See [docs/safety_case/prohibited_uses.md](../docs/safety_case/prohibited_uses.md).

## Getting oriented

Before editing any area, read [CONTEXT_MAP.md](../CONTEXT_MAP.md) and follow
its "read first" links for that directory:

| Path | Read first |
|------|------------|
| `kernel/` | [docs/model/](../docs/model/) |
| `protection/` | [docs/safety_case/protection_rationale.md](../docs/safety_case/protection_rationale.md) |
| `orchestrator/` | [docs/architecture/](../docs/architecture/) |
| `dashboard/` | [dashboard/README.md](../dashboard/README.md) |
| `scenarios/` | [docs/model/scenarios.md](../docs/model/scenarios.md) |
| `specs/tla/` | [specs/tla/README.md](../specs/tla/README.md) |
| `docs/` | [docs/index.md](../docs/index.md) |

**Component contracts in brief:** the **kernel** owns all physics state and is
deterministic and allocation-free per step. The **protection** library is a pure
C scan function over plain inputs; it never reaches into kernel internals. The
**orchestrator** is the only component that couples them, in a fixed order each
step, and the only component exporting a C ABI. The **dashboard** (Python)
talks exclusively to the C ABI via `ctypes`, records every run to HDF5, and can
replay any recorded run deterministically. **Scenarios** are data, not code.
**Specs** model the protection state machine only. **Docs** carry the safety
case — when code behavior and docs disagree, fix one of them in the same change.

Automation agents should also read [AGENTS.md](../AGENTS.md) for repository-wide
rules that apply to agent-driven workflows.

## Development setup

### Requirements

| Component | Needs |
|-----------|-------|
| Native core | gcc/g++ (C11/C++17), CMake ≥ 3.25, ninja |
| Fortran numerics (optional) | gfortran |
| Sanitizer preset (optional) | `libasan` / `libubsan` runtimes |
| Python layer | Python ≥ 3.12 |
| Docs | the project venv (Sphinx from `requirements.txt`) |
| TLA+ checking (optional) | Java 11+ and `tla2tools.jar` |

See [docs/development/building.md](../docs/development/building.md) for details.

### Native core

```sh
cmake --preset dev && cmake --build --preset dev
cmake --preset release && cmake --build --preset release   # shared lib for dashboard
ctest --preset dev
```

CMake presets: `dev` (debug + tests), `dev-asan` (ASan/UBSan), `release`
(dashboard library), `tidy` (clang-tidy). At configure time the build reports
whether Fortran numerics are enabled or the C++ fallback is in use.

### Python environment and dashboard

```sh
python3 -m venv .venv
.venv/bin/pip install -r requirements.txt
.venv/bin/pip install -e dashboard
.venv/bin/python -m rbmk_dash
```

### Optional components

- **Fortran numerics** — see [docs/development/fortran.md](../docs/development/fortran.md).
- **TLA+ model checking** — see [docs/development/tla.md](../docs/development/tla.md).
- **Sanitizer builds** — use the `dev-asan` preset when the runtime libraries are installed.

## Project conventions

### Native workflows (no wrapper scripts)

Use the commands documented here and in the development docs directly. Do **not**
add helper or wrapper scripts (shell, Make, task runners) for building, testing,
or linting. Scripts are allowed only when they are themselves a deliverable.

### Determinism

Applies to the kernel, protection system, and orchestrator:

- Fixed time step, fixed iteration order, single-threaded simulation core.
- No wall-clock, locale, or environment dependence inside simulation code.
- Randomness only from the seeded in-repo PRNG, and only for detector noise
  (off by default).
- Identical config + identical command sequence must produce bit-identical
  observations on the same binary. Tests enforce this; do not break them.

### Code and documentation stay in sync

When you change behavior, update the relevant Sphinx documentation in the same
change. The safety case in `docs/safety_case/` is authoritative for scope and
verification claims.

## Code style

| Area | Standard | Tooling |
|------|----------|---------|
| `kernel/`, `protection/`, `orchestrator/` | MISRA-inspired subset | `.clang-format`, `.clang-tidy`, warning-clean builds |
| `dashboard/` | Python ≥ 3.12 | `ruff` per `dashboard/pyproject.toml` (line-length 100) |
| All text files | `.editorconfig` | UTF-8, LF, trailing-whitespace rules |

**C/C++ highlights** (full text in
[docs/development/coding_standard.md](../docs/development/coding_standard.md)):

- Fixed-width integer types in every interface; no `int` in ABI or protection code.
- No recursion; no dynamic allocation in protection or per-step paths (one
  documented exception at `rbmk_create` in the orchestrator C ABI).
- All loops bounded; every `switch` has a `default`.
- Validate and clamp inputs at API boundaries; NULL pointers fail toward the
  safe state (scram).
- Deviations require a `NOTE(misra-dev): <justification>` comment at the site.

**Python:** run `ruff check dashboard` before submitting. Rules: `E`, `F`, `W`,
`I`, `UP`, `B`, `SIM`, `RUF`.

## Verification

There is **no CI workflow** in this repository. Contributors must run the
applicable checks locally before opening a pull request. The full claim-to-check
mapping lives in
[docs/safety_case/verification.md](../docs/safety_case/verification.md).

### Required checks

From a clean checkout with dependencies installed:

```sh
cmake --preset dev && cmake --build --preset dev && ctest --preset dev
.venv/bin/python -m pytest dashboard/tests
.venv/bin/ruff check dashboard
clang-format --dry-run -Werror $(git ls-files '*.c' '*.h' '*.cpp' '*.hpp')
cmake --preset tidy && cmake --build --preset tidy
.venv/bin/sphinx-build -W -b html docs docs/_build/html
```

### Testing conventions

From [docs/development/testing.md](../docs/development/testing.md):

- **Determinism is a test target.** Any change that breaks bit-identical dual
  runs, HDF5 replay verification, or the timeline consistency test is a defect.
- **Qualitative shapes are pinned, not numbers.** Tests assert signs, ratios,
  orderings, and windows so toy coefficients can be tuned without rewriting the
  suite while educational behaviors stay protected.
- **GUI tests run offscreen** (`QT_QPA_PLATFORM=offscreen`, pytest-qt) and only
  smoke-test construction and reactions; logic lives below the UI.
- Native suites must stay warning-clean; the `tidy` preset must report no
  findings in first-party code (documented exclusions in `.clang-tidy`).

### Optional checks

Run these when your change touches the relevant area:

- **TLA+** — from `specs/tla/`: `java -jar tools/tla2tools.jar -config RPS.cfg RPS.tla`
  (see [docs/development/tla.md](../docs/development/tla.md) for setup).
- **Sanitizers** — `cmake --preset dev-asan && cmake --build --preset dev-asan && ctest --preset dev-asan`.
- **Fortran parity** — when gfortran is installed, reconfigure and re-run `ctest --preset dev`.

## Commits

- Keep commits small and focused on a single logical change.
- Use an imperative subject line of 72 characters or fewer.
- Explain *why* in the body when it is not obvious from the diff.

There is no documented branch-naming convention. Work from `main` or a
descriptive feature branch.

## Pull requests

There is no pull-request template or documented review policy. When you open a
PR against `main`:

1. Summarize what changed and why.
2. Link a related issue if one exists.
3. Note which verification commands you ran and their outcome.
4. Include documentation updates when behavior or safety-case claims change.

All applicable checks in the verification section above should pass. Changes
that break determinism tests or introduce operational-fidelity tuning will not
be accepted.

Review and merge are handled by the maintainers; there are no automated gates
beyond what you run locally.

## Issues

There are no issue templates. Useful reports include:

- **Bugs** — affected component (`kernel/`, `protection/`, `dashboard/`, etc.),
  steps to reproduce, expected vs actual behavior, and verification commands run.
- **Features** — how the proposal fits the educational scope; note if it touches
  physics coefficients or the Chernobyl timeline narrative.

Proposals that require proprietary data, operational fidelity, or uses listed in
[docs/safety_case/prohibited_uses.md](../docs/safety_case/prohibited_uses.md)
are out of scope.

## Security and responsible disclosure

This project is an offline educational simulator. It is not deployed operational
software, and there is no formal security-disclosure process or `SECURITY.md`.

Report software defects through GitHub issues. For scope and prohibited-use
questions, see the safety case, especially
[docs/safety_case/prohibited_uses.md](../docs/safety_case/prohibited_uses.md).

## Documentation

- Build the docs warning-free: `.venv/bin/sphinx-build -W -b html docs docs/_build/html`.
- Model and safety-case pages must include educational-use disclaimers where relevant.
- The Sphinx toctree starts at [docs/index.md](../docs/index.md).

## License

By contributing, you agree that your contributions are licensed under the
[MIT License](../LICENSE). The educational-use boundary is a statement of intent
and scope, not a license restriction.
