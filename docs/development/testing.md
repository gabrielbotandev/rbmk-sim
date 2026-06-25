# Testing

The complete matrix of suites, frameworks, and what each one proves lives in
{doc}`../safety_case/verification`. Quick reference:

```sh
ctest --preset dev                              # kernel + RPS + orchestrator
.venv/bin/python -m pytest dashboard/tests      # bindings, recorder, timeline,
                                                # comparison, UI smoke
.venv/bin/ruff check dashboard                  # Python lint
clang-format --dry-run -Werror $(git ls-files '*.c' '*.h' '*.cpp' '*.hpp')
cmake --preset tidy && cmake --build --preset tidy   # static analysis
```

Conventions:

- **Determinism is a test target.** Any change that breaks bit-identical
  dual runs, HDF5 replay verification, or the timeline consistency test is a
  defect by definition (see {doc}`../safety_case/determinism`).
- **Qualitative shapes are pinned, not numbers.** Tests assert signs, ratios,
  orderings, and windows (e.g. "the 1986 scram bumps power by >2% within
  6 s") so the toy coefficients can be tuned without rewriting the suite —
  while the educational behaviors stay protected.
- **GUI tests run offscreen** (`QT_QPA_PLATFORM=offscreen`, pytest-qt) and
  only smoke-test construction and reactions; logic lives below the UI.
- The native suites must stay warning-clean under the project's strict flags;
  the tidy preset must report no findings in first-party code (documented
  exclusions in `.clang-tidy`).
