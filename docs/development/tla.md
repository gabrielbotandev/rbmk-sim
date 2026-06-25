# TLA+ workflow

The protection-system state machine is specified in `specs/tla/RPS.tla` — a
finite model of `protection/src/rps.c` where one `Scan` action equals one
`rps_step` call. The full mapping table, checked invariants/properties, and
rationale live in `specs/tla/README.md` (in the repository).

## Running the model checker

Java 11+ required. Fetch the tools jar once (kept out of git):

```sh
mkdir -p specs/tla/tools
curl -L -o specs/tla/tools/tla2tools.jar \
  https://github.com/tlaplus/tlaplus/releases/download/v1.7.4/tla2tools.jar
```

Run TLC from `specs/tla/`:

```sh
java -XX:+UseParallelGC -jar tools/tla2tools.jar -config RPS.cfg RPS.tla
```

Expected output ends with `Model checking completed. No error has been found.`
(~2,950 distinct states, about 1-2 minutes).

## What the spec guarantees (and what it does not)

TLC exhaustively verifies the *protection state logic*: trip latching, the
impossibility of resetting from TRIPPED, safe-state reachability under the
explicit plant-response fairness assumption, and that live hazards always
command a scram. It says nothing about the physics model, thresholds, or
timing — those are covered by the native test suites.
