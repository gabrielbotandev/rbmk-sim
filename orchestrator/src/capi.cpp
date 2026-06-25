// C ABI implementation. Every function is NULL-tolerant and never lets an
// exception cross the boundary (allocation failure returns NULL from create).
#include "rbmk/capi/rbmk_capi.h"

#include <new>

#include "rbmk/orchestrator/simulator.hpp"

#ifndef RBMK_MODEL_VERSION
#define RBMK_MODEL_VERSION "0.0.0-unversioned"
#endif

using rbmk::orchestrator::Simulator;

struct rbmk_sim {
    Simulator impl;

    explicit rbmk_sim(const rbmk::kernel::Config& cfg) : impl(cfg) {}
};

namespace {

rbmk::kernel::Config to_kernel_config(const rbmk_config* config) {
    rbmk::kernel::Config out;  // defaults
    if (config != nullptr && config->struct_size == sizeof(rbmk_config)) {
        out.num_channels = config->num_channels;
        out.dt_s = config->dt_s;
        out.rod_design = (config->rod_design == RBMK_ROD_DESIGN_MODIFIED)
                             ? rbmk::kernel::RodDesign::kModified
                             : rbmk::kernel::RodDesign::kOriginal1986;
        out.start_at_xenon_equilibrium = (config->start_at_xenon_equilibrium != 0U);
        out.initial_power_frac = config->initial_power_frac;
        out.initial_manual_rod_insertion = config->initial_manual_rod_insertion;
        out.ar_enabled = (config->ar_enabled != 0U);
        out.detector_noise = (config->detector_noise != 0U);
        out.noise_seed = config->noise_seed;
    }
    return out;
}

}  // namespace

extern "C" {

uint32_t rbmk_abi_version(void) {
    return RBMK_ABI_VERSION;
}

const char* rbmk_model_version(void) {
    return RBMK_MODEL_VERSION;
}

void rbmk_config_default(rbmk_config* out) {
    if (out != nullptr) {
        const rbmk::kernel::Config defaults;
        out->struct_size = sizeof(rbmk_config);
        out->num_channels = defaults.num_channels;
        out->dt_s = defaults.dt_s;
        out->rod_design = RBMK_ROD_DESIGN_1986;
        out->start_at_xenon_equilibrium = defaults.start_at_xenon_equilibrium ? 1U : 0U;
        out->initial_power_frac = defaults.initial_power_frac;
        out->initial_manual_rod_insertion = defaults.initial_manual_rod_insertion;
        out->ar_enabled = defaults.ar_enabled ? 1U : 0U;
        out->detector_noise = defaults.detector_noise ? 1U : 0U;
        out->noise_seed = defaults.noise_seed;
    }
}

rbmk_sim* rbmk_create(const rbmk_config* config) {
    // NOTE(misra-dev): the only dynamic allocation in the native core; the
    // handle is created once per session, never in the per-step path.
    return new (std::nothrow) rbmk_sim(to_kernel_config(config));
}

void rbmk_destroy(rbmk_sim* sim) {
    delete sim;  // delete on NULL is well-defined
}

void rbmk_step(rbmk_sim* sim, uint32_t n_steps) {
    if (sim != nullptr) {
        sim->impl.step(n_steps);
    }
}

void rbmk_observe(const rbmk_sim* sim, rbmk_observation* out) {
    if (sim != nullptr && out != nullptr) {
        sim->impl.observe(*out);
    }
}

void rbmk_set_rod_target(rbmk_sim* sim, uint32_t bank, double fraction) {
    if (sim != nullptr) {
        sim->impl.set_rod_target(bank, fraction);
    }
}

void rbmk_set_pump_flow(rbmk_sim* sim, double fraction) {
    if (sim != nullptr) {
        sim->impl.set_pump_flow(fraction);
    }
}

void rbmk_set_power_setpoint(rbmk_sim* sim, double fraction) {
    if (sim != nullptr) {
        sim->impl.set_power_setpoint(fraction);
    }
}

void rbmk_set_ar_enabled(rbmk_sim* sim, uint32_t enabled) {
    if (sim != nullptr) {
        sim->impl.set_ar_enabled(enabled != 0U);
    }
}

void rbmk_press_az5(rbmk_sim* sim) {
    if (sim != nullptr) {
        sim->impl.press_az5();
    }
}

void rbmk_request_reset(rbmk_sim* sim) {
    if (sim != nullptr) {
        sim->impl.request_reset();
    }
}

void rbmk_set_detectors_valid(rbmk_sim* sim, uint32_t mask) {
    if (sim != nullptr) {
        sim->impl.set_detectors_valid(mask);
    }
}

}  // extern "C"
