/*
 * Simplified Reactor Protection System (RPS) for the RBMK-SIM educational
 * simulator.
 *
 * Style: MISRA-inspired C11 subset (see docs/development/coding_standard.md).
 *   - fixed-width types only, no dynamic memory, no recursion, bounded loops
 *   - pure deterministic scan function: outputs depend only on (state, inputs)
 *   - defensive NULL handling that always fails toward the safe state
 *
 * The protection logic itself is intentionally simple and qualitative. It is
 * an educational artifact, NOT an operational protection design.
 */
#ifndef RBMK_RPS_H
#define RBMK_RPS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RPS_API_VERSION 1u

/* ------------------------------------------------------------------ */
/* States                                                              */
/* ------------------------------------------------------------------ */
typedef enum {
    RPS_STATE_NORMAL = 0,       /* all parameters inside alarm limits        */
    RPS_STATE_ALARM = 1,        /* at least one alarm level exceeded         */
    RPS_STATE_TRIPPED = 2,      /* a trip latched: scram command active      */
    RPS_STATE_SAFE_SHUTDOWN = 3 /* tripped, rods fully in, power below floor */
} rps_state_id_t;

/* ------------------------------------------------------------------ */
/* Condition bits (used for alarms, active trip conditions, latches)  */
/* ------------------------------------------------------------------ */
#define RPS_COND_NONE 0x00u
#define RPS_COND_MANUAL_AZ5 0x01u
#define RPS_COND_OVERPOWER 0x02u
#define RPS_COND_SHORT_PERIOD 0x04u
#define RPS_COND_LOW_FLOW 0x08u
#define RPS_COND_HIGH_VOID 0x10u
#define RPS_COND_SENSOR_FAULT 0x20u
#define RPS_COND_ALL 0x3Fu

/* ------------------------------------------------------------------ */
/* Fixed setpoints (normalized units; educational toy values)         */
/* Set/clear pairs implement deterministic hysteresis.                 */
/* ------------------------------------------------------------------ */
#define RPS_OVERPOWER_ALARM_SET 1.05
#define RPS_OVERPOWER_ALARM_CLEAR 1.03
#define RPS_OVERPOWER_TRIP_SET 1.10
#define RPS_OVERPOWER_TRIP_CLEAR 1.08

/* Reactor period limits apply to positive (rising) periods only.      */
#define RPS_PERIOD_ALARM_SET_S 30.0
#define RPS_PERIOD_ALARM_CLEAR_S 36.0
#define RPS_PERIOD_TRIP_SET_S 15.0
#define RPS_PERIOD_TRIP_CLEAR_S 18.0

/* Low-flow protection is armed only above this power level.           */
#define RPS_LOW_FLOW_ARM_POWER 0.10
#define RPS_FLOW_ALARM_SET 0.80
#define RPS_FLOW_ALARM_CLEAR 0.82
#define RPS_FLOW_TRIP_SET 0.70
#define RPS_FLOW_TRIP_CLEAR 0.72

#define RPS_VOID_ALARM_SET 0.30
#define RPS_VOID_ALARM_CLEAR 0.28
#define RPS_VOID_TRIP_SET 0.38
#define RPS_VOID_TRIP_CLEAR 0.36

/* Fewer valid power detectors than this is a protection fault.        */
#define RPS_MIN_VALID_DETECTORS 2u

/* Safe-shutdown entry conditions (while tripped).                     */
#define RPS_SHUTDOWN_POWER_FRAC 0.01

/* ------------------------------------------------------------------ */
/* Interface types                                                     */
/* ------------------------------------------------------------------ */
typedef struct {
    double power_frac;        /* conservative (highest valid) power reading  */
    double period_s;          /* signed reactor period, seconds              */
    double coolant_flow_frac; /* normalized loop flow                        */
    double avg_void_frac;     /* core-average void fraction                  */
    uint32_t detectors_valid; /* bitmask of valid power detectors (bits 0-3) */
    uint32_t manual_az5;      /* 1 while the AZ-5 button is pressed          */
    uint32_t reset_request;   /* 1 while the operator requests reset         */
    uint32_t rods_full_in;    /* 1 when every shutdown bank is fully inserted*/
} rps_inputs_t;

typedef struct {
    uint32_t state;         /* rps_state_id_t                              */
    uint32_t scram_command; /* 1 = drive all rods to full insertion        */
    uint32_t alarms;        /* RPS_COND_* bits at alarm level              */
    uint32_t trip_active;   /* RPS_COND_* bits currently at trip level     */
    uint32_t trip_latched;  /* RPS_COND_* bits latched since last reset    */
    uint32_t reset_denied;  /* 1 when a reset request was refused          */
} rps_outputs_t;

typedef struct {
    uint32_t initialized;     /* sentinel, set by rps_init                   */
    uint32_t state;           /* rps_state_id_t                              */
    uint32_t alarms;          /* hysteresis memory, alarm level              */
    uint32_t trip_conditions; /* hysteresis memory, trip level               */
    uint32_t trip_latched;    /* latched causes                              */
    uint32_t reserved;        /* keeps the struct padding-free               */
    uint64_t cycle_count;
} rps_t;

/* ------------------------------------------------------------------ */
/* API                                                                 */
/* ------------------------------------------------------------------ */

/* Places the protection system in NORMAL with no latches. */
void rps_init(rps_t* rps);

/*
 * One protection scan. Pure with respect to (rps, inputs): no other state is
 * read or written. NULL rps/inputs or an uninitialized rps produce the
 * fail-safe output (scram commanded) without dereferencing the bad pointer.
 * A NULL outputs pointer makes the call a no-op on outputs but the internal
 * state still advances deterministically.
 */
void rps_step(rps_t* rps, const rps_inputs_t* inputs, rps_outputs_t* outputs);

#ifdef __cplusplus
}
#endif

#endif /* RBMK_RPS_H */
