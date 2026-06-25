/*
 * RBMK-SIM public C ABI.
 *
 * This header is the single integration boundary between the native core
 * (C++ kernel + C protection system, coupled by the orchestrator) and every
 * frontend (the Python dashboard uses it via ctypes).
 *
 * ABI rules:
 *   - flat, fixed-size, padding-free structs of doubles / uint32 / uint64
 *   - no callbacks, no exceptions, no ownership transfer except create/destroy
 *   - every function tolerates NULL handles (no-op) - fail-safe by default
 *   - bump RBMK_ABI_VERSION on any layout or semantic change
 */
#ifndef RBMK_CAPI_H
#define RBMK_CAPI_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RBMK_ABI_VERSION 1u

#define RBMK_MAX_CHANNELS 64u
#define RBMK_NUM_BANKS 4u
#define RBMK_NUM_DETECTORS 4u

/* Rod bank indices */
#define RBMK_BANK_MANUAL_A 0u
#define RBMK_BANK_MANUAL_B 1u
#define RBMK_BANK_AUTOMATIC 2u
#define RBMK_BANK_EMERGENCY 3u

/* Rod designs */
#define RBMK_ROD_DESIGN_1986 0u
#define RBMK_ROD_DESIGN_MODIFIED 1u

/* RPS states (mirror of rps_state_id_t) */
#define RBMK_RPS_NORMAL 0u
#define RBMK_RPS_ALARM 1u
#define RBMK_RPS_TRIPPED 2u
#define RBMK_RPS_SAFE_SHUTDOWN 3u

/* RPS condition bits (mirror of RPS_COND_*) */
#define RBMK_COND_MANUAL_AZ5 0x01u
#define RBMK_COND_OVERPOWER 0x02u
#define RBMK_COND_SHORT_PERIOD 0x04u
#define RBMK_COND_LOW_FLOW 0x08u
#define RBMK_COND_HIGH_VOID 0x10u
#define RBMK_COND_SENSOR_FAULT 0x20u

typedef struct rbmk_sim rbmk_sim; /* opaque simulator handle */

/* Simulation configuration. Initialize with rbmk_config_default() and then
 * override fields; unknown/out-of-range values are clamped by the core. */
typedef struct rbmk_config {
    uint32_t struct_size;                /* sizeof(rbmk_config), for ABI sanity checks */
    uint32_t num_channels;               /* 4..64                                      */
    double dt_s;                         /* fixed step, 0.001..0.5 s                   */
    uint32_t rod_design;                 /* RBMK_ROD_DESIGN_*                          */
    uint32_t start_at_xenon_equilibrium; /* 0/1                               */
    double initial_power_frac;           /* 1e-6..1.2                                  */
    double initial_manual_rod_insertion; /* 0..1                              */
    uint32_t ar_enabled;                 /* 0/1: automatic regulator at start          */
    uint32_t detector_noise;             /* 0/1                                        */
    uint64_t noise_seed;
} rbmk_config;

/* Flat observation of the coupled system (kernel + protection).
 * Layout: 8-byte fields first, then a 12-entry uint32 block (no padding). */
typedef struct rbmk_observation {
    double time_s;
    uint64_t step_count;

    double power_frac;
    double power_mw;
    double period_s;

    double rho_total;
    double rho_rods;
    double rho_void;
    double rho_doppler;
    double rho_xenon;
    double rho_base;

    double iodine;
    double xenon;
    double xenon_rel;

    double avg_void_frac;
    double avg_fuel_temp_c;
    double coolant_temp_c;
    double flow_frac;
    double flow_command_frac;

    double inserted_rod_equivalent;
    double power_setpoint_frac;
    double rod_position[RBMK_NUM_BANKS];
    double rod_target[RBMK_NUM_BANKS];

    double channel_power[RBMK_MAX_CHANNELS];
    double channel_void[RBMK_MAX_CHANNELS];
    double detector_power_frac[RBMK_NUM_DETECTORS];

    uint32_t scram_latched;
    uint32_t ar_enabled;
    uint32_t num_channels;
    uint32_t validity_exceeded;
    uint32_t rps_state;
    uint32_t rps_scram_command;
    uint32_t rps_alarms;
    uint32_t rps_trip_active;
    uint32_t rps_trip_latched;
    uint32_t rps_reset_denied;
    uint32_t detectors_valid_mask;
    uint32_t abi_version;
} rbmk_observation;

/* --- metadata ---------------------------------------------------------- */
uint32_t rbmk_abi_version(void);
const char* rbmk_model_version(void); /* static string, never NULL */

/* --- lifecycle --------------------------------------------------------- */
void rbmk_config_default(rbmk_config* out);
rbmk_sim* rbmk_create(const rbmk_config* config); /* NULL config => defaults */
void rbmk_destroy(rbmk_sim* sim);

/* --- time -------------------------------------------------------------- */
void rbmk_step(rbmk_sim* sim, uint32_t n_steps);

/* --- observation ------------------------------------------------------- */
void rbmk_observe(const rbmk_sim* sim, rbmk_observation* out);

/* --- operator inputs (clamped; ignored on NULL handle) ------------------ */
void rbmk_set_rod_target(rbmk_sim* sim, uint32_t bank, double fraction);
void rbmk_set_pump_flow(rbmk_sim* sim, double fraction);
void rbmk_set_power_setpoint(rbmk_sim* sim, double fraction);
void rbmk_set_ar_enabled(rbmk_sim* sim, uint32_t enabled);
void rbmk_press_az5(rbmk_sim* sim);     /* one-scan pulse; trip latches anyway */
void rbmk_request_reset(rbmk_sim* sim); /* one-scan pulse                      */

/* --- fault injection (educational) ------------------------------------- */
void rbmk_set_detectors_valid(rbmk_sim* sim, uint32_t mask);

#ifdef __cplusplus
}
#endif

#endif /* RBMK_CAPI_H */
