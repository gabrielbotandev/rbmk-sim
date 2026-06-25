------------------------------- MODULE RPS -------------------------------
(***************************************************************************)
(* Formal model of the RBMK-SIM reactor protection system scan logic.     *)
(*                                                                         *)
(* This is a small, finite abstraction of protection/src/rps.c:           *)
(*   - one Scan action = one call to rps_step                             *)
(*   - `active`  abstracts the trip-level condition bits (hysteresis      *)
(*     outcome), `alarms` the alarm-level bits                            *)
(*   - the AZ-5 button is one of the conditions, so "no condition active" *)
(*     implies "AZ-5 released" exactly as in the C permissive check       *)
(*   - `latched` mirrors trip_latched, `state` mirrors rps->state         *)
(*                                                                         *)
(* Educational artifact for the safety case; see specs/tla/README.md.     *)
(***************************************************************************)
EXTENDS FiniteSets

CONSTANT Conditions  \* abstract trip causes, e.g. {az5, overpower, flow}

VARIABLES
    state,     \* "Normal" | "Alarm" | "Tripped" | "SafeShutdown"
    latched,   \* subset of Conditions: causes latched since last reset
    active,    \* subset of Conditions: at trip level on this scan
    alarms,    \* subset of Conditions: at alarm level on this scan
    resetReq,  \* operator reset request on this scan
    rodsIn,    \* every shutdown bank fully inserted
    powerLow   \* power below the safe-shutdown threshold

vars == <<state, latched, active, alarms, resetReq, rodsIn, powerLow>>

States == {"Normal", "Alarm", "Tripped", "SafeShutdown"}

\* The scram command is a pure function of the state, as in the C code.
Scram == state \in {"Tripped", "SafeShutdown"}

TypeOK ==
    /\ state \in States
    /\ latched \subseteq Conditions
    /\ active \subseteq Conditions
    /\ alarms \subseteq Conditions
    /\ resetReq \in BOOLEAN
    /\ rodsIn \in BOOLEAN
    /\ powerLow \in BOOLEAN

Init ==
    /\ state = "Normal"
    /\ latched = {}
    /\ active = {}
    /\ alarms = {}
    /\ resetReq = FALSE
    /\ rodsIn = FALSE
    /\ powerLow = FALSE

(***************************************************************************)
(* One protection scan: the environment presents arbitrary new inputs and *)
(* the protection system reacts exactly as rps_step does:                 *)
(*   1. every active condition is latched                                 *)
(*   2. the state machine advances                                        *)
(*   3. reset is honoured only from SafeShutdown with no active condition *)
(***************************************************************************)
Scan ==
    \E newActive \in SUBSET Conditions :
    \E newAlarms \in SUBSET Conditions :
    \E newReset \in BOOLEAN :
    \E newRodsIn \in BOOLEAN :
    \E newPowerLow \in BOOLEAN :
        /\ active' = newActive
        /\ alarms' = newAlarms
        /\ resetReq' = newReset
        /\ rodsIn' = newRodsIn
        /\ powerLow' = newPowerLow
        /\ LET latchedAfter == latched \cup newActive IN
           IF state = "Normal" \/ state = "Alarm"
           THEN /\ latched' = latchedAfter
                /\ state' = IF latchedAfter /= {} THEN "Tripped"
                            ELSE IF newAlarms /= {} THEN "Alarm"
                            ELSE "Normal"
           ELSE IF state = "Tripped"
           THEN /\ latched' = latchedAfter
                /\ state' = IF newRodsIn /\ newPowerLow
                            THEN "SafeShutdown"
                            ELSE "Tripped"
           ELSE \* state = "SafeShutdown"
                IF newReset /\ newActive = {}
                THEN /\ latched' = {}        \* permissives met: clear and
                     /\ state' = "Normal"    \* return to NORMAL
                ELSE /\ latched' = latchedAfter
                     /\ state' = "SafeShutdown"

Next == Scan

\* A scan on which the plant has reached the shutdown conditions; strong
\* fairness on it states the physical assumption "a commanded scram
\* eventually drives the rods in and power down".
PlantDelivers == Scan /\ rodsIn' /\ powerLow'

Spec == Init /\ [][Next]_vars /\ WF_vars(Next) /\ SF_vars(PlantDelivers)

(***************************************************************************)
(* Safety invariants                                                      *)
(***************************************************************************)

\* In NORMAL nothing is latched (a latch forces TRIPPED until reset).
NormalImpliesNoLatch == (state = "Normal") => (latched = {})

\* Any latched cause means the scram command is active.
LatchImpliesScram == (latched /= {}) => Scram

\* Whenever any condition is at trip level, the scram command is active:
\* the protection never ignores a live hazard.
ActiveImpliesScram == (active /= {}) => Scram

\* ALARM never carries a latched trip.
AlarmImpliesNoLatch == (state = "Alarm") => (latched = {})

(***************************************************************************)
(* Action properties (checked over every transition)                      *)
(***************************************************************************)

\* While tripped, latched causes are never lost (until an accepted reset,
\* which can only happen from SafeShutdown).
LatchMonotoneWhileTripped == [][ (state = "Tripped") => (latched \subseteq latched') ]_vars

\* There is no direct return from TRIPPED to NORMAL: the plant must pass
\* through SAFE SHUTDOWN and an explicit reset.
NoDirectResetFromTripped == [][ (state = "Tripped") => (state' /= "Normal") ]_vars

\* SAFE SHUTDOWN is reachable only from TRIPPED (or itself).
SafeShutdownOnlyAfterTrip ==
    [][ (state' = "SafeShutdown") => (state \in {"Tripped", "SafeShutdown"}) ]_vars

\* A reset is never accepted while any condition is still active.
NoResetWhileActive ==
    [][ (state = "SafeShutdown" /\ state' = "Normal") => (active' = {}) ]_vars

(***************************************************************************)
(* Liveness (under the plant-response fairness assumption)                *)
(***************************************************************************)

\* Every trip eventually reaches the safe shutdown state.
TrippedLeadsToSafeShutdown == (state = "Tripped") ~> (state = "SafeShutdown")

=============================================================================
