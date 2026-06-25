# Protection logic rationale

The protection system (RPS) is the safety-critical *style* showcase of this
project: small, allocation-free, single-exit C11 with explicit hysteresis and
a latched state machine, mirrored by a TLC-checked TLA+ model.

## State machine

```{mermaid}
stateDiagram-v2
    [*] --> NORMAL
    NORMAL --> ALARM : any alarm level reached
    ALARM --> NORMAL : all alarms cleared (hysteresis)
    NORMAL --> TRIPPED : any trip latched
    ALARM --> TRIPPED : any trip latched
    TRIPPED --> SAFE_SHUTDOWN : rods fully in AND power < 1%
    SAFE_SHUTDOWN --> NORMAL : operator reset AND no active condition
    note right of TRIPPED
        scram command active
        latches never cleared here
        reset always denied here
    end note
    note right of SAFE_SHUTDOWN
        scram command stays active
        (rods held in)
    end note
```

## Design decisions and their reasons

**Latched trips.** A trip cause is remembered (`trip_latched`) until an
explicit, permissive-gated reset — a transient hazard must not "un-trip" the
plant by clearing itself. Verified at three levels: Unity unit tests, the
end-to-end coupling tests, and the TLA+ action property
`LatchMonotoneWhileTripped`.

**Hysteresis on every analog threshold.** Each condition has a set level and
a clear level (e.g. overpower trips at 110% and clears at 108%). This makes
the scan deterministic and chatter-free at the boundary — the same input
trace always produces the same output trace, bit for bit.

**Rising-period protection only.** A short *positive* period (fast power
rise) trips; falling power never does. The arming of low-flow protection by a
power threshold shows conditional protection: at 5% power, low flow is not a
hazard in this toy model, and spurious trips are themselves a safety cost.

**Conservative sensing.** The orchestrator feeds the RPS the *highest*
reading among valid power detectors, and fewer than two valid detectors is
itself a trip (`SENSOR_FAULT`) — degraded instrumentation fails toward
shutdown, not toward blindness.

**Fail-safe contract violations.** A NULL pointer or uninitialized state
produces the fail-safe output (scram commanded). The defensive checks cost a
few instructions per scan and remove an entire class of integration error.

**Reset permissives.** Reset is honoured only from SAFE_SHUTDOWN, only with
no condition active and AZ-5 released. There is deliberately *no* path from
TRIPPED back to NORMAL: the plant must first reach the safe state. The TLA+
properties `NoDirectResetFromTripped`, `SafeShutdownOnlyAfterTrip`, and
`NoResetWhileActive` verify exactly this over every reachable input sequence.

**Scram authority.** The protection command always wins over operator rod
targets (the kernel scram latch overrides targets until released). The
orchestrator applies it in the same fixed position of every step, so there is
no scan/actuation race.

## Setpoints

| Condition | Alarm set/clear | Trip set/clear | Notes |
|-----------|-----------------|----------------|-------|
| Overpower | 1.05 / 1.03 | 1.10 / 1.08 | × nominal power |
| Short period | 30 / 36 s | 15 / 18 s | rising periods only |
| Low flow | 0.80 / 0.82 | 0.70 / 0.72 | armed above 10% power |
| High void | 0.30 / 0.28 | 0.38 / 0.36 | core-average void fraction |
| Sensor fault | — | < 2 valid detectors | no hysteresis |
| Manual AZ-5 | — | while pressed | latches like any trip |

All values are toy setpoints chosen to interact sensibly with the toy
physics; they are *not* derived from any plant.
