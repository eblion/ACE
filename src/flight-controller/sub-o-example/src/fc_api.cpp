// Copyright 2026 Tyler Wiggins
// SPDX-License-Identifier: Apache-2.0

#include "flight-controller/sub-o-example/inc/fc_sim_connector.hpp"
#include "sim/inc/rocket.hpp"
#include <cstdlib>
#include <filesystem>
#include <iostream>
#ifndef _WIN32
#include <dlfcn.h>
#endif

namespace {
    Rocket* g_rocket = nullptr;
}

namespace fc_sim_connector {

void begin(Rocket* r) {
    g_rocket = r;
}

void end() {
    g_rocket = nullptr;
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

} // extern "C"
