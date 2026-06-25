# Building

## Requirements

| Component | Needs |
|-----------|-------|
| Native core | gcc/g++ (C11/C++17), CMake ≥ 3.25, ninja |
| Fortran numerics (optional) | gfortran (`sudo dnf install gcc-gfortran`) |
| Sanitizer preset (optional) | `sudo dnf install libasan libubsan` |
| Python layer | Python ≥ 3.12, `python3 -m venv` |
| Docs | the project venv (Sphinx installed from `requirements.txt`) |
| TLA+ checking (optional) | Java 11+ and `tla2tools.jar` (see {doc}`tla`) |

## Presets

| Preset | Purpose |
|--------|---------|
| `dev` | Debug build + all tests |
| `dev-asan` | Debug + ASan/UBSan (requires the runtime libraries) |
| `release` | RelWithDebInfo; the shared library the dashboard loads |
| `tidy` | clang-tidy on every translation unit |

```sh
cmake --preset dev && cmake --build --preset dev && ctest --preset dev
cmake --preset release && cmake --build --preset release   # for the dashboard
```

At configure time the superbuild reports whether Fortran numerics are enabled
(`RBMK: Fortran numerics enabled (...)`) or the C++ fallback is in use.
Test frameworks (doctest, Unity) are fetched once at pinned tags into the
build tree — network is needed for the first testing configure only.

## Python environment

```sh
python3 -m venv .venv
.venv/bin/pip install -r requirements.txt
.venv/bin/pip install -e dashboard
.venv/bin/python -m rbmk_dash        # launch (needs the release/dev library)
```

## Documentation

```sh
.venv/bin/sphinx-build -W -b html docs docs/_build/html
```
