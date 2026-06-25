# rbmk-dash

PySide6 engineering dashboard and tooling for the RBMK-SIM educational simulator.

- `rbmk_dash.core` — `ctypes` bindings to the native C ABI (`librbmk_sim.so`),
  the simulation session (history, command log), HDF5 recording and replay.
- `rbmk_dash.ui` — the dashboard application (operate view, timeline replay,
  design comparison, protection annunciators).
- `rbmk_dash.cli` — headless utilities (`rbmk-logtool`).

Run from the repository root after building the native core (`cmake --preset release
&& cmake --build --preset release`) and installing (`pip install -e dashboard`):

```sh
rbmk-dash            # or: python -m rbmk_dash
```

The native library is discovered automatically in `build/release` / `build/dev`,
or explicitly via the `RBMK_SIM_LIB` environment variable.

> Educational toy only — see the repository README for the safety boundary.
