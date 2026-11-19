// Copyright 2026 Tyler Wiggins
// SPDX-License-Identifier: Apache-2.0

#pragma once

// connects FC with rocket class

#include "flight-controller/fc_api.h"
#include <memory>
#include <string>

class Rocket;

namespace fc_sim_connector {

/**
 * points fc_active_stage at a rocket for the duration of an fc_update call
 * @param r target rocket to answer queries from
 */
void begin(Rocket* r);

// kill
void end();

// a flight controller's entry point
struct Api {
    fc_state* (*init)(const fc_vehicle*) = fc_init;
    void (*update)(fc_state*, const fc_sensors*, fc_commands*) = fc_update;
    void (*free)(fc_state*) = fc_free;
};

/**
 * loads a flight controller built with `make fc-lib`
 * @param path shared library, relative to the working directory. empty gives the built in fc
 */
Api load(const std::string& path);

struct StateDeleter {
    void (*free)(fc_state*) = fc_free;
    void operator()(fc_state* s) const { free(s); }
};

// owns whatever fc_init handed back
using State = std::unique_ptr<fc_state, StateDeleter>;

}
