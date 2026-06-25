# Quickstart

## Build the native core

Requires `gcc`/`g++`, `cmake` (>= 3.24) and `ninja`.

```sh
cmake --preset dev
cmake --build --preset dev
ctest --preset dev
```

The `release` preset builds the shared library loaded by the Python dashboard:

```sh
cmake --preset release
cmake --build --preset release
```

## Set up Python and run the dashboard

```sh
python3 -m venv .venv
.venv/bin/pip install -r requirements.txt
.venv/bin/pip install -e dashboard
.venv/bin/python -m rbmk_dash
```

## Run the test suites

```sh
ctest --preset dev                          # native (C/C++) tests
.venv/bin/python -m pytest dashboard/tests  # Python tests
```

## Build this documentation

```sh
.venv/bin/sphinx-build -W -b html docs docs/_build/html
```

## Optional dependencies

- **Fortran numerics** — `sudo dnf install gcc-gfortran`, then reconfigure. Without it,
  the build uses the C++ fallback implementation (identical algorithm; see
  {doc}`development/fortran`).
- **TLA+ model checking** — Java 11+ plus the `tla2tools.jar` release; see
  {doc}`development/tla`.
