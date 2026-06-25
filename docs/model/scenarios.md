# Scenario and timeline format

Scenarios are versioned JSON files in the repository's `scenarios/` directory
(currently `chernobyl_timeline.json`). They are **data, not code**: an
initial configuration plus a sorted list of timed events.

## Schema (version 1)

```json
{
  "schema_version": 1,
  "name": "machine-name",
  "title": "Human title",
  "description": "...",
  "sources": ["public references"],
  "disclaimer": "...",
  "config": {
    "rod_design": "1986 | modified",
    "num_channels": 12,
    "dt_s": 0.05,
    "initial_power_frac": 1.0,
    "initial_manual_rod_insertion": 0.35,
    "ar_enabled": true,
    "detector_noise": false
  },
  "events": [
    {
      "id": "unique-id",
      "offset_s": 0,
      "label": "timestamped headline",
      "historical": true,
      "narrative": "what is documented to have happened",
      "model_note": "what the toy model is scripted to do, and why",
      "actions": [ { "command": "set_power_setpoint", "args": [0.5] } ]
    }
  ]
}
```

Rules enforced by the loader (`rbmk_dash.core.scenario`):

- `schema_version` must match; events must be sorted by `offset_s`.
- every `actions[].command` must be a whitelisted session command
  (`set_rod_target`, `set_pump_flow`, `set_power_setpoint`, `set_ar_enabled`,
  `press_az5`, `request_reset`, `set_detectors_valid`).
- `historical` marks the narrative as documented history (rendered with the
  HISTORICAL badge); `false` marks model illustrations. `model_note` always
  describes the scripted model behavior. The UI never blends the two.

## Replay semantics

`TimelineRun` applies each event's actions at the exact step
`round(offset_s / dt)`, fast-forwarding deterministically between events
(simulated time is never compressed — xenon needs its real hours; the kernel
just runs as fast as the host allows). Every event is also recorded into the
HDF5 log's `/events` group, so a saved timeline run carries its own
annotations and verifies bit-exactly like any other run.
