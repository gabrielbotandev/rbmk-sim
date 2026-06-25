# HDF5 logging and replay

Every run — interactive, timeline, or headless — can be captured as a single
HDF5 file that fully describes it and proves its own integrity under replay.

## Schema (format_version 1)

| Path | Content |
|------|---------|
| `/meta` (attrs) | `format_version`, `model_version` (with `+fortran` when applicable), `abi_version`, `dash_version`, `scenario`, `created_utc`, `dt_s`, `sample_stride`, `total_steps`, and an educational disclaimer |
| `/config` (attrs) | every `rbmk_config` field — enough to reconstruct the simulator |
| `/timeseries/*` | equal-length 1-D datasets: `time_s`, power, period, the full reactivity breakdown, poisons, thermals, flows, rod-margin proxy (float64); protection state/flags and validity (uint64); plus 2-D `rod_position`, `detector_power_frac`, `channel_void`, `channel_power` |
| `/commands/*` | `step` (uint64), `name`, `args_json` — the operator command log |
| `/events/*` | `step`, `kind` (`historical`/`model`), `text` — timeline annotations |

The recorder samples on exact step-stride boundaries (stride 1 for
interactive runs, 20 → 1 s samples for the 24 h timeline), independent of UI
timing, so logs are identical no matter how the run was driven.

## Utilities

```sh
rbmk-logtool info   runs/example.h5            # metadata, commands, events
rbmk-logtool export runs/example.h5 --out x.csv [--fields time_s power_mw ...]
rbmk-logtool verify runs/example.h5            # bit-exact replay check
```

`verify` re-executes the run from `/config` + `/commands` on the local binary
and compares **every** recorded signal; any nonzero deviation fails (exit 1).
This catches both tampered logs and binary mismatches (see the per-binary
caveat in {doc}`../safety_case/determinism`).

## Workflows

- **Save from the dashboard** — "Save run log (HDF5)…" (Operate) or "Save
  timeline log…" (Timeline replay); files default to `runs/` (gitignored).
- **Inspect** — `rbmk-logtool info`, or open the file with any HDF5 viewer;
  the schema is self-describing.
- **Replay programmatically** — `load_run` / `replay_run` /
  `verify_run` in `rbmk_dash.core.recorder`.
