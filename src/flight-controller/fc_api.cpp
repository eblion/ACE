// Copyright 2026 Tyler Wiggins
// SPDX-License-Identifier: Apache-2.0

#include "flight-controller/fc_sim_connector.hpp"
#include "sim/inc/rocket.hpp"
#include <cstdlib>
#include <filesystem>
#include <iostream>
#ifndef _WIN32
#include <dlfcn.h>
#endif

namespace {
    Rocket* g_rocket = nullptr;
    const std::vector<Rocket>* g_rockets = nullptr;
}

namespace fc_sim_connector {

void begin(Rocket* r) {
    g_rocket = r;
}

void end() {
    g_rocket = nullptr;
}

void set_rockets(const std::vector<Rocket>* rockets) {
    g_rockets = rockets;
}

Api load(const std::string& path) {
    Api api;
    if (path.empty()) return api;
#ifdef _WIN32
    // shortcut: no LoadLibrary path, add one if per rocket fcs are needed on windows
    std::cerr << "fc error: can't load '" << path << "', per rocket flight controllers need linux or mac\n";
    std::exit(1);
#else
    // some genuinely insane magic that claude created (thanks anthropic)
    void* lib = dlopen(std::filesystem::absolute(path).c_str(), RTLD_NOW | RTLD_LOCAL);
    if (lib) {
        api.init   = reinterpret_cast<decltype(api.init)>(dlsym(lib, "fc_init"));
        api.update = reinterpret_cast<decltype(api.update)>(dlsym(lib, "fc_update"));
        api.free   = reinterpret_cast<decltype(api.free)>(dlsym(lib, "fc_free"));
    }
    if (!lib || !api.init || !api.update || !api.free) {
        const char* err = dlerror();
        std::cerr << "fc error: can't load '" << path << "': " << (err ? err : "missing fc_init/fc_update/fc_free") << "\n";
        std::exit(1);
    }
#endif
    return api;
}

}

extern "C" {

int fc_active_stage(void) {
    return g_rocket ? g_rocket->active_stage_idx() : 0;
}

int fc_debug_get_rocket(const char* name, fc_debug_rocket* out) {
    if (!g_rockets || !name || !out) return 0;
    for (const Rocket& r : *g_rockets) {
        if (r.get_name() != name) continue;
        RocketState s = r.get_state();
        out->mass         = s.mass;
        out->fuel         = s.fuel;
        out->r            = s.r;
        out->v            = s.v;
        out->a            = s.a;
        out->w            = s.w;
        out->q            = s.q_rocket;
        out->active_stage = r.active_stage_idx();
        out->exploded     = s.detonation_active;
        return 1;
    }
    return 0;
}

} // extern "C"
