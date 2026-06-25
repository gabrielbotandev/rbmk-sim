/* Unit tests for the reactor protection system scan logic. */
#include <string.h>

#include <unity.h>

#include "rbmk/rps/rps.h"

static rps_t rps;
static rps_inputs_t in;
static rps_outputs_t out;

/* Nominal full-power inputs: everything healthy. */
static void make_nominal_inputs(rps_inputs_t* inputs) {
    inputs->power_frac = 1.0;
    inputs->period_s = 9999.0;
    inputs->coolant_flow_frac = 1.0;
    inputs->avg_void_frac = 0.25;
    inputs->detectors_valid = 0x0Fu;
    inputs->manual_az5 = 0u;
    inputs->reset_request = 0u;
    inputs->rods_full_in = 0u;
}

void setUp(void) {
    rps_init(&rps);
    make_nominal_inputs(&in);
    memset(&out, 0, sizeof(out));
}

void tearDown(void) {}

/* ------------------------------------------------------------------ */
/* Initialization and nominal behavior                                 */
/* ------------------------------------------------------------------ */

static void test_init_state_is_normal(void) {
    TEST_ASSERT_EQUAL_UINT32(1u, rps.initialized);
    TEST_ASSERT_EQUAL_UINT32((uint32_t)RPS_STATE_NORMAL, rps.state);
    TEST_ASSERT_EQUAL_UINT32(RPS_COND_NONE, rps.trip_latched);
}

static void test_nominal_inputs_stay_normal(void) {
    for (int i = 0; i < 1000; ++i) {
        rps_step(&rps, &in, &out);
    }
    TEST_ASSERT_EQUAL_UINT32((uint32_t)RPS_STATE_NORMAL, out.state);
    TEST_ASSERT_EQUAL_UINT32(0u, out.scram_command);
    TEST_ASSERT_EQUAL_UINT32(RPS_COND_NONE, out.alarms);
    TEST_ASSERT_EQUAL_UINT32(RPS_COND_NONE, out.trip_latched);
}

/* ------------------------------------------------------------------ */
/* Alarm hysteresis                                                    */
/* ------------------------------------------------------------------ */

static void test_overpower_alarm_with_hysteresis(void) {
    in.power_frac = 1.06; /* above alarm set */
    rps_step(&rps, &in, &out);
    TEST_ASSERT_EQUAL_UINT32((uint32_t)RPS_STATE_ALARM, out.state);
    TEST_ASSERT_BITS_HIGH(RPS_COND_OVERPOWER, out.alarms);
    TEST_ASSERT_EQUAL_UINT32(0u, out.scram_command);

    in.power_frac = 1.04; /* inside the hysteresis band: alarm holds */
    rps_step(&rps, &in, &out);
    TEST_ASSERT_BITS_HIGH(RPS_COND_OVERPOWER, out.alarms);
    TEST_ASSERT_EQUAL_UINT32((uint32_t)RPS_STATE_ALARM, out.state);

    in.power_frac = 1.02; /* below clear level: alarm drops, back to NORMAL */
    rps_step(&rps, &in, &out);
    TEST_ASSERT_BITS_LOW(RPS_COND_OVERPOWER, out.alarms);
    TEST_ASSERT_EQUAL_UINT32((uint32_t)RPS_STATE_NORMAL, out.state);
}

/* ------------------------------------------------------------------ */
/* Trips and latching                                                  */
/* ------------------------------------------------------------------ */

static void test_overpower_trip_latches(void) {
    in.power_frac = 1.11;
    rps_step(&rps, &in, &out);
    TEST_ASSERT_EQUAL_UINT32((uint32_t)RPS_STATE_TRIPPED, out.state);
    TEST_ASSERT_EQUAL_UINT32(1u, out.scram_command);
    TEST_ASSERT_BITS_HIGH(RPS_COND_OVERPOWER, out.trip_latched);

    /* The condition clears but the latch (and scram) must persist. */
    in.power_frac = 0.50;
    rps_step(&rps, &in, &out);
    TEST_ASSERT_EQUAL_UINT32((uint32_t)RPS_STATE_TRIPPED, out.state);
    TEST_ASSERT_EQUAL_UINT32(1u, out.scram_command);
    TEST_ASSERT_BITS_HIGH(RPS_COND_OVERPOWER, out.trip_latched);
    TEST_ASSERT_BITS_LOW(RPS_COND_OVERPOWER, out.trip_active);
}

static void test_short_period_trip_only_when_rising(void) {
    in.period_s = 8.0; /* fast rise */
    rps_step(&rps, &in, &out);
    TEST_ASSERT_BITS_HIGH(RPS_COND_SHORT_PERIOD, out.trip_latched);
    TEST_ASSERT_EQUAL_UINT32(1u, out.scram_command);

    rps_init(&rps);
    in.period_s = -8.0; /* fast fall: never a period trip */
    rps_step(&rps, &in, &out);
    TEST_ASSERT_BITS_LOW(RPS_COND_SHORT_PERIOD, out.trip_latched);
    TEST_ASSERT_EQUAL_UINT32(0u, out.scram_command);
}

static void test_period_boundary_values(void) {
    in.period_s = RPS_PERIOD_TRIP_SET_S; /* exactly at the limit: no trip */
    rps_step(&rps, &in, &out);
    TEST_ASSERT_BITS_LOW(RPS_COND_SHORT_PERIOD, out.trip_active);

    in.period_s = RPS_PERIOD_TRIP_SET_S - 0.01; /* just inside: trip */
    rps_step(&rps, &in, &out);
    TEST_ASSERT_BITS_HIGH(RPS_COND_SHORT_PERIOD, out.trip_active);
}

static void test_low_flow_trip_is_armed_by_power(void) {
    in.power_frac = 0.05; /* below arming level */
    in.coolant_flow_frac = 0.50;
    rps_step(&rps, &in, &out);
    TEST_ASSERT_EQUAL_UINT32(0u, out.scram_command);
    TEST_ASSERT_BITS_LOW(RPS_COND_LOW_FLOW, out.trip_latched);

    rps_init(&rps);
    in.power_frac = 0.50; /* armed */
    rps_step(&rps, &in, &out);
    TEST_ASSERT_EQUAL_UINT32(1u, out.scram_command);
    TEST_ASSERT_BITS_HIGH(RPS_COND_LOW_FLOW, out.trip_latched);
}

static void test_high_void_trip(void) {
    in.avg_void_frac = 0.75;
    rps_step(&rps, &in, &out);
    TEST_ASSERT_BITS_HIGH(RPS_COND_HIGH_VOID, out.trip_latched);
    TEST_ASSERT_EQUAL_UINT32(1u, out.scram_command);
}

static void test_sensor_fault_trip_on_too_few_detectors(void) {
    in.detectors_valid = 0x01u; /* one healthy detector: below the minimum */
    rps_step(&rps, &in, &out);
    TEST_ASSERT_BITS_HIGH(RPS_COND_SENSOR_FAULT, out.trip_latched);
    TEST_ASSERT_EQUAL_UINT32(1u, out.scram_command);

    rps_init(&rps);
    in.detectors_valid = 0x05u; /* two healthy detectors: acceptable */
    rps_step(&rps, &in, &out);
    TEST_ASSERT_EQUAL_UINT32(0u, out.scram_command);
}

static void test_manual_az5_trips_immediately(void) {
    in.manual_az5 = 1u;
    rps_step(&rps, &in, &out);
    TEST_ASSERT_EQUAL_UINT32((uint32_t)RPS_STATE_TRIPPED, out.state);
    TEST_ASSERT_EQUAL_UINT32(1u, out.scram_command);
    TEST_ASSERT_BITS_HIGH(RPS_COND_MANUAL_AZ5, out.trip_latched);
}

/* ------------------------------------------------------------------ */
/* Safe shutdown and reset permissives                                 */
/* ------------------------------------------------------------------ */

static void trip_and_reach_safe_shutdown(void) {
    in.manual_az5 = 1u;
    rps_step(&rps, &in, &out);
    in.manual_az5 = 0u;

    /* Rods drive in; power decays. */
    in.rods_full_in = 1u;
    in.power_frac = 0.005;
    rps_step(&rps, &in, &out);
    TEST_ASSERT_EQUAL_UINT32((uint32_t)RPS_STATE_SAFE_SHUTDOWN, out.state);
    TEST_ASSERT_EQUAL_UINT32(1u, out.scram_command); /* rods held in */
}

static void test_safe_shutdown_entry(void) {
    trip_and_reach_safe_shutdown();
}

static void test_reset_denied_while_tripped(void) {
    in.power_frac = 1.11;
    rps_step(&rps, &in, &out);
    TEST_ASSERT_EQUAL_UINT32((uint32_t)RPS_STATE_TRIPPED, out.state);

    in.power_frac = 0.50;  /* condition cleared          */
    in.reset_request = 1u; /* but rods are not in yet    */
    rps_step(&rps, &in, &out);
    TEST_ASSERT_EQUAL_UINT32((uint32_t)RPS_STATE_TRIPPED, out.state);
    TEST_ASSERT_EQUAL_UINT32(1u, out.reset_denied);
    TEST_ASSERT_EQUAL_UINT32(1u, out.scram_command);
}

static void test_reset_allowed_from_safe_shutdown(void) {
    trip_and_reach_safe_shutdown();

    in.reset_request = 1u;
    rps_step(&rps, &in, &out);
    TEST_ASSERT_EQUAL_UINT32((uint32_t)RPS_STATE_NORMAL, out.state);
    TEST_ASSERT_EQUAL_UINT32(0u, out.scram_command);
    TEST_ASSERT_EQUAL_UINT32(RPS_COND_NONE, out.trip_latched);
    TEST_ASSERT_EQUAL_UINT32(0u, out.reset_denied);
}

static void test_reset_denied_while_az5_held(void) {
    trip_and_reach_safe_shutdown();

    in.manual_az5 = 1u; /* button still held */
    in.reset_request = 1u;
    rps_step(&rps, &in, &out);
    TEST_ASSERT_EQUAL_UINT32((uint32_t)RPS_STATE_SAFE_SHUTDOWN, out.state);
    TEST_ASSERT_EQUAL_UINT32(1u, out.reset_denied);
    TEST_ASSERT_EQUAL_UINT32(1u, out.scram_command);
}

static void test_retrip_after_reset(void) {
    trip_and_reach_safe_shutdown();
    in.reset_request = 1u;
    rps_step(&rps, &in, &out);
    TEST_ASSERT_EQUAL_UINT32((uint32_t)RPS_STATE_NORMAL, out.state);

    /* A fresh hazard must trip again immediately. */
    in.reset_request = 0u;
    in.rods_full_in = 0u;
    in.power_frac = 1.2;
    rps_step(&rps, &in, &out);
    TEST_ASSERT_EQUAL_UINT32((uint32_t)RPS_STATE_TRIPPED, out.state);
    TEST_ASSERT_EQUAL_UINT32(1u, out.scram_command);
}

/* ------------------------------------------------------------------ */
/* Determinism and defensive behavior                                  */
/* ------------------------------------------------------------------ */

static void test_deterministic_replay_of_input_trace(void) {
    /* A synthetic but eventful input trace, applied to two instances. */
    rps_t rps_a;
    rps_t rps_b;
    rps_outputs_t out_a;
    rps_outputs_t out_b;
    rps_inputs_t trace;

    rps_init(&rps_a);
    rps_init(&rps_b);
    make_nominal_inputs(&trace);

    for (int i = 0; i < 5000; ++i) {
        /* Deterministic synthetic excursion profile. */
        trace.power_frac = 1.0 + 0.15 * (double)((i / 100) % 3) * ((i % 2 == 0) ? 1.0 : 0.9);
        trace.period_s = (i > 2000) ? 12.0 : 9999.0;
        trace.coolant_flow_frac = (i > 3000) ? 0.65 : 1.0;
        trace.manual_az5 = (i == 4000) ? 1u : 0u;
        trace.rods_full_in = (i > 4500) ? 1u : 0u;

        rps_step(&rps_a, &trace, &out_a);
        rps_step(&rps_b, &trace, &out_b);
        TEST_ASSERT_EQUAL_MEMORY(&out_a, &out_b, sizeof(rps_outputs_t));
    }
    TEST_ASSERT_EQUAL_MEMORY(&rps_a, &rps_b, sizeof(rps_t));
}

static void test_null_and_uninitialized_fail_safe(void) {
    rps_t cold;
    memset(&cold, 0, sizeof(cold)); /* never initialized */

    rps_step((rps_t*)0, &in, &out);
    TEST_ASSERT_EQUAL_UINT32(1u, out.scram_command);

    memset(&out, 0, sizeof(out));
    rps_step(&cold, &in, &out);
    TEST_ASSERT_EQUAL_UINT32(1u, out.scram_command);

    memset(&out, 0, sizeof(out));
    rps_step(&rps, (const rps_inputs_t*)0, &out);
    TEST_ASSERT_EQUAL_UINT32(1u, out.scram_command);

    /* NULL outputs must not crash. */
    rps_step(&rps, &in, (rps_outputs_t*)0);
    TEST_PASS();
}

/* ------------------------------------------------------------------ */

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_init_state_is_normal);
    RUN_TEST(test_nominal_inputs_stay_normal);
    RUN_TEST(test_overpower_alarm_with_hysteresis);
    RUN_TEST(test_overpower_trip_latches);
    RUN_TEST(test_short_period_trip_only_when_rising);
    RUN_TEST(test_period_boundary_values);
    RUN_TEST(test_low_flow_trip_is_armed_by_power);
    RUN_TEST(test_high_void_trip);
    RUN_TEST(test_sensor_fault_trip_on_too_few_detectors);
    RUN_TEST(test_manual_az5_trips_immediately);
    RUN_TEST(test_safe_shutdown_entry);
    RUN_TEST(test_reset_denied_while_tripped);
    RUN_TEST(test_reset_allowed_from_safe_shutdown);
    RUN_TEST(test_reset_denied_while_az5_held);
    RUN_TEST(test_retrip_after_reset);
    RUN_TEST(test_deterministic_replay_of_input_trace);
    RUN_TEST(test_null_and_uninitialized_fail_safe);
    return UNITY_END();
}
