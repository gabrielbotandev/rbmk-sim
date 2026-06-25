# Components and data flow

For the component table and responsibility matrix see
{doc}`../safety_case/architecture`. This page covers the runtime flows.

## Interactive operation

```{mermaid}
sequenceDiagram
    participant UI as Dashboard (QTimer tick)
    participant S as Session (Python)
    participant O as Orchestrator (C++)
    participant R as RPS (C)
    participant K as Kernel (C++)

    UI->>S: apply("set_rod_target", bank, x)
    S->>S: append CommandRecord(step, name, args)
    S->>O: rbmk_set_rod_target(...)
    UI->>S: advance(n)
    loop n steps
        O->>K: observe (sense)
        O->>R: rps_step(inputs)
        R-->>O: scram_command, state, latches
        O->>K: command/release scram (actuate)
        O->>K: step() (physics)
    end
    S->>O: rbmk_observe()
    S-->>UI: observation + history
```

The Python layer is presentation and recording only: nothing in the coupling
loop depends on it, so a headless run (`TimelineRun`, tests, `rbmk-logtool
verify`) behaves identically to the dashboard.

## Timeline replay

`TimelineRun` walks the scenario's events, advancing the session to each
event's exact step, applying its actions through the same command-log path as
the UI, and tagging the run log's `/events` group. Fast-forward is just
"advance many steps quickly" — simulated time is never compressed.

## Recording and replay

```{mermaid}
flowchart LR
    live["Live session<br/>(config + commands + steps)"] -->|"record (stride)"| h5[("HDF5 log")]
    h5 -->|load_run| data["RunData"]
    data -->|replay_run| clone["Re-executed session"]
    clone -->|"compare every signal"| verdict{"max |Δ| == 0 ?"}
    verdict -->|yes| pass["bit-exact PASS"]
    verdict -->|no| fail["FAIL (binary mismatch or tamper)"]
```
