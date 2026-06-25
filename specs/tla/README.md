# TLA+ specification of the protection system

`RPS.tla` is a small, finite formal model of the scan logic in
[`protection/src/rps.c`](../../protection/src/rps.c). One `Scan` action
corresponds to one `rps_step` call; the environment presents arbitrary inputs
each scan, so TLC explores every reachable combination of conditions, alarms,
plant feedback, and operator requests.

## Mapping to the C implementation

| TLA+ | C (`rps.c` / `rps.h`) |
|------|------------------------|
| `state` | `rps->state` (`RPS_STATE_*`) |
| `latched` | `rps->trip_latched` |
| `active` | `rps->trip_conditions` (hysteresis outcome at trip level) |
| `alarms` | `rps->alarms` |
| `latched' = latched ∪ active'` | step 2: `trip_latched \|= trip_conditions` |
| `Scram` | `outputs->scram_command` |
| reset only from `SafeShutdown` with `active' = {}` | the `RPS_STATE_SAFE_SHUTDOWN` reset permissive (AZ-5 release is implied because AZ-5 is one of the conditions) |

## What is checked

Invariants (safety):

- `TypeOK` — domains of all variables
- `NormalImpliesNoLatch`, `AlarmImpliesNoLatch` — pre-trip states carry no latch
- `LatchImpliesScram` — a latched cause always commands a scram
- `ActiveImpliesScram` — a live trip-level hazard always commands a scram

Action properties:

- `LatchMonotoneWhileTripped` — latched causes are never lost while tripped
- `NoDirectResetFromTripped` — no path from TRIPPED straight to NORMAL
- `SafeShutdownOnlyAfterTrip` — SAFE SHUTDOWN is only entered from TRIPPED
- `NoResetWhileActive` — a reset is never accepted with a condition active

Liveness (under `SF_vars(PlantDelivers)`, the explicit physical assumption
that a commanded scram eventually drives the rods in and power down):

- `TrippedLeadsToSafeShutdown` — every trip reaches the safe state

## Running TLC

Requires Java 11+. Fetch the TLA+ tools once:

```sh
mkdir -p specs/tla/tools
curl -L -o specs/tla/tools/tla2tools.jar \
  https://github.com/tlaplus/tlaplus/releases/download/v1.7.4/tla2tools.jar
```

Then, from `specs/tla/`:

```sh
java -XX:+UseParallelGC -jar tools/tla2tools.jar -config RPS.cfg RPS.tla
```

Expected: TLC completes with **no errors**, all invariants and properties
hold. The model uses three representative conditions (`az5`, `overpower`,
`flow`); the logic is independent of the condition count.
