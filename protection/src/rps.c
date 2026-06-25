/*
 * Reactor Protection System scan logic. See rps.h for the interface contract
 * and docs/safety_case/protection_rationale.md for the design rationale.
 *
 * MISRA-inspired subset notes:
 *   - every function has a single exit point
 *   - no dynamic memory, no recursion, all loops bounded
 *   - hysteresis is implemented with explicit set/clear levels so behavior is
 *     deterministic and chatter-free at every threshold
 */
#include "rbmk/rps/rps.h"

/* ------------------------------------------------------------------ */
/* Internal helpers (all pure)                                         */
/* ------------------------------------------------------------------ */

/* Updates one condition bit for a "value too high" comparison. */
static uint32_t cond_high(uint32_t bits, uint32_t bit, double value, double set_level,
                          double clear_level) {
    uint32_t result = bits;
    if (value > set_level) {
        result |= bit;
    } else if (value < clear_level) {
        result &= ~bit;
    } else {
        /* inside the hysteresis band: hold previous decision */
    }
    return result;
}

/* Updates one condition bit for a "value too low" comparison. */
static uint32_t cond_low(uint32_t bits, uint32_t bit, double value, double set_level,
                         double clear_level) {
    uint32_t result = bits;
    if (value < set_level) {
        result |= bit;
    } else if (value > clear_level) {
        result &= ~bit;
    } else {
        /* inside the hysteresis band: hold previous decision */
    }
    return result;
}

/* Counts set bits in the low byte (bounded, branch-free loop). */
static uint32_t popcount8(uint32_t mask) {
    uint32_t count = 0u;
    const uint32_t m = mask & 0xFFu;
    for (uint32_t i = 0u; i < 8u; ++i) {
        count += (m >> i) & 1u;
    }
    return count;
}

/* Evaluates alarm-level conditions with hysteresis memory. */
static uint32_t evaluate_alarms(uint32_t previous, const rps_inputs_t* in) {
    uint32_t bits = previous;

    bits = cond_high(bits, RPS_COND_OVERPOWER, in->power_frac, RPS_OVERPOWER_ALARM_SET,
                     RPS_OVERPOWER_ALARM_CLEAR);

    /* Short period: only a positive (rising) period is a hazard. */
    if (in->period_s > 0.0) {
        bits = cond_low(bits, RPS_COND_SHORT_PERIOD, in->period_s, RPS_PERIOD_ALARM_SET_S,
                        RPS_PERIOD_ALARM_CLEAR_S);
    } else {
        bits &= ~((uint32_t)RPS_COND_SHORT_PERIOD);
    }

    if (in->power_frac > RPS_LOW_FLOW_ARM_POWER) {
        bits = cond_low(bits, RPS_COND_LOW_FLOW, in->coolant_flow_frac, RPS_FLOW_ALARM_SET,
                        RPS_FLOW_ALARM_CLEAR);
    } else {
        bits &= ~((uint32_t)RPS_COND_LOW_FLOW);
    }

    bits = cond_high(bits, RPS_COND_HIGH_VOID, in->avg_void_frac, RPS_VOID_ALARM_SET,
                     RPS_VOID_ALARM_CLEAR);

    if (popcount8(in->detectors_valid) < RPS_MIN_VALID_DETECTORS) {
        bits |= RPS_COND_SENSOR_FAULT;
    } else {
        bits &= ~((uint32_t)RPS_COND_SENSOR_FAULT);
    }

    if (in->manual_az5 != 0u) {
        bits |= RPS_COND_MANUAL_AZ5;
    } else {
        bits &= ~((uint32_t)RPS_COND_MANUAL_AZ5);
    }

    return bits;
}

/* Evaluates trip-level conditions with hysteresis memory. */
static uint32_t evaluate_trip_conditions(uint32_t previous, const rps_inputs_t* in) {
    uint32_t bits = previous;

    bits = cond_high(bits, RPS_COND_OVERPOWER, in->power_frac, RPS_OVERPOWER_TRIP_SET,
                     RPS_OVERPOWER_TRIP_CLEAR);

    if (in->period_s > 0.0) {
        bits = cond_low(bits, RPS_COND_SHORT_PERIOD, in->period_s, RPS_PERIOD_TRIP_SET_S,
                        RPS_PERIOD_TRIP_CLEAR_S);
    } else {
        bits &= ~((uint32_t)RPS_COND_SHORT_PERIOD);
    }

    if (in->power_frac > RPS_LOW_FLOW_ARM_POWER) {
        bits = cond_low(bits, RPS_COND_LOW_FLOW, in->coolant_flow_frac, RPS_FLOW_TRIP_SET,
                        RPS_FLOW_TRIP_CLEAR);
    } else {
        bits &= ~((uint32_t)RPS_COND_LOW_FLOW);
    }

    bits = cond_high(bits, RPS_COND_HIGH_VOID, in->avg_void_frac, RPS_VOID_TRIP_SET,
                     RPS_VOID_TRIP_CLEAR);

    if (popcount8(in->detectors_valid) < RPS_MIN_VALID_DETECTORS) {
        bits |= RPS_COND_SENSOR_FAULT;
    } else {
        bits &= ~((uint32_t)RPS_COND_SENSOR_FAULT);
    }

    if (in->manual_az5 != 0u) {
        bits |= RPS_COND_MANUAL_AZ5;
    } else {
        bits &= ~((uint32_t)RPS_COND_MANUAL_AZ5);
    }

    return bits;
}

/* Writes the fail-safe output pattern (used on contract violations). */
static void write_fail_safe(rps_outputs_t* outputs) {
    if (outputs != (rps_outputs_t*)0) {
        outputs->state = (uint32_t)RPS_STATE_TRIPPED;
        outputs->scram_command = 1u;
        outputs->alarms = RPS_COND_SENSOR_FAULT;
        outputs->trip_active = RPS_COND_SENSOR_FAULT;
        outputs->trip_latched = RPS_COND_SENSOR_FAULT;
        outputs->reset_denied = 0u;
    }
}

/* ------------------------------------------------------------------ */
/* API                                                                 */
/* ------------------------------------------------------------------ */

void rps_init(rps_t* rps) {
    if (rps != (rps_t*)0) {
        rps->initialized = 1u;
        rps->state = (uint32_t)RPS_STATE_NORMAL;
        rps->alarms = RPS_COND_NONE;
        rps->trip_conditions = RPS_COND_NONE;
        rps->trip_latched = RPS_COND_NONE;
        rps->reserved = 0u;
        rps->cycle_count = 0u;
    }
}

void rps_step(rps_t* rps, const rps_inputs_t* inputs, rps_outputs_t* outputs) {
    if ((rps == (rps_t*)0) || (inputs == (const rps_inputs_t*)0) || (rps->initialized != 1u)) {
        /* Contract violation: fail toward the safe state. */
        write_fail_safe(outputs);
    } else {
        uint32_t reset_denied = 0u;

        /* 1. Condition evaluation with hysteresis. */
        rps->alarms = evaluate_alarms(rps->alarms, inputs);
        rps->trip_conditions = evaluate_trip_conditions(rps->trip_conditions, inputs);

        /* 2. Latch every condition that reaches trip level. */
        rps->trip_latched |= rps->trip_conditions;

        /* 3. State transitions. */
        switch (rps->state) {
        case (uint32_t)RPS_STATE_NORMAL:
            if (rps->trip_latched != RPS_COND_NONE) {
                rps->state = (uint32_t)RPS_STATE_TRIPPED;
            } else if (rps->alarms != RPS_COND_NONE) {
                rps->state = (uint32_t)RPS_STATE_ALARM;
            } else {
                /* remain NORMAL */
            }
            break;

        case (uint32_t)RPS_STATE_ALARM:
            if (rps->trip_latched != RPS_COND_NONE) {
                rps->state = (uint32_t)RPS_STATE_TRIPPED;
            } else if (rps->alarms == RPS_COND_NONE) {
                rps->state = (uint32_t)RPS_STATE_NORMAL;
            } else {
                /* remain ALARM */
            }
            break;

        case (uint32_t)RPS_STATE_TRIPPED:
            if ((inputs->rods_full_in == 1u) && (inputs->power_frac < RPS_SHUTDOWN_POWER_FRAC)) {
                rps->state = (uint32_t)RPS_STATE_SAFE_SHUTDOWN;
            }
            if (inputs->reset_request == 1u) {
                /* Reset from TRIPPED is never allowed: the plant must reach
                 * the safe shutdown state first. */
                reset_denied = 1u;
            }
            break;

        case (uint32_t)RPS_STATE_SAFE_SHUTDOWN:
            if (inputs->reset_request == 1u) {
                if ((rps->trip_conditions == RPS_COND_NONE) && (inputs->manual_az5 == 0u)) {
                    /* Permissives met: clear latches, return to NORMAL.
                     * Rod withdrawal remains a separate operator action. */
                    rps->trip_latched = RPS_COND_NONE;
                    rps->state = (uint32_t)RPS_STATE_NORMAL;
                } else {
                    reset_denied = 1u;
                }
            }
            break;

        default:
            /* Unreachable by construction; recover to the safe state. */
            rps->state = (uint32_t)RPS_STATE_TRIPPED;
            rps->trip_latched |= RPS_COND_SENSOR_FAULT;
            break;
        }

        rps->cycle_count += 1u;

        /* 4. Outputs. */
        if (outputs != (rps_outputs_t*)0) {
            outputs->state = rps->state;
            outputs->scram_command = ((rps->state == (uint32_t)RPS_STATE_TRIPPED) ||
                                      (rps->state == (uint32_t)RPS_STATE_SAFE_SHUTDOWN))
                                         ? 1u
                                         : 0u;
            outputs->alarms = rps->alarms;
            outputs->trip_active = rps->trip_conditions;
            outputs->trip_latched = rps->trip_latched;
            outputs->reset_denied = reset_denied;
        }
    }
}
