#include "ams/ams_dpi.h"

#include <cassert>
#include <cstring>
#include <iostream>
#include <memory>

#include "ams/ams_bridge.h"
#include "ams/ams_types.h"
#include "ams/config_parser.h"

namespace ams {

static IAMSTestbench* g_active_tb = nullptr;
static std::unique_ptr<AMSBridge> g_bridge;

void ams_register_testbench(IAMSTestbench* tb) {
    g_active_tb = tb;
}

IAMSTestbench* ams_active_testbench() {
    return g_active_tb;
}

} // namespace ams

namespace {

ams::IAMSTestbench* tb() {
    return ams::ams_active_testbench();
}

} // namespace

extern "C" {

int ams_init(const char* config_path) {
    // If a testbench was already registered (e.g., by a hand-written main.cpp),
    // just initialize it.  Otherwise create an AMSBridge from the YAML config
    // path passed from SystemVerilog.
    auto* existing = tb();
    if (existing) {
        int argc = 1;
        const char* argv[] = {"ams_sim"};
        char** argv_mutable = const_cast<char**>(argv);
        if (!existing->init(argc, argv_mutable, {}, false)) {
            std::cerr << "[AMS-DPI] ams_init failed\n";
            return 1;
        }
        return 0;
    }

    if (!config_path) {
        std::cerr << "[AMS-DPI] ams_init called without a config path\n";
        return 1;
    }

    ams::AMSConfig config;
    try {
        config = ams::loadConfig(config_path);
    } catch (const std::exception& e) {
        std::cerr << "[AMS-DPI] Failed to load config '" << config_path
                  << "': " << e.what() << "\n";
        return 1;
    }

    ams::g_bridge = std::make_unique<ams::AMSBridge>(config);
    ams::ams_register_testbench(ams::g_bridge.get());

    int argc = 1;
    const char* argv[] = {"ams_sim"};
    char** argv_mutable = const_cast<char**>(argv);
    if (!ams::g_bridge->init(argc, argv_mutable, {}, false)) {
        std::cerr << "[AMS-DPI] ams_init failed\n";
        ams::g_bridge.reset();
        ams::ams_register_testbench(nullptr);
        return 1;
    }
    return 0;
}

int ams_finish() {
    auto* t = tb();
    if (!t) return 1;
    t->closeTrace();
    // Destroy the bridge (and the ngspice session) if we created it.
    ams::g_bridge.reset();
    ams::ams_register_testbench(nullptr);
    return 0;
}

int ams_run_analog(double dt_ns) {
    auto* t = tb();
    if (!t) return 1;
    t->runAnalog(dt_ns * 1e-9);
    return 0;
}

double ams_get_voltage(const char* node_name) {
    auto* t = tb();
    if (!t || !node_name) return 0.0;
    return t->getVoltage(node_name);
}

int ams_set_voltage(const char* source_name, double voltage) {
    auto* t = tb();
    if (!t || !source_name) return 1;
    t->setVoltage(source_name, voltage);
    return 0;
}

int ams_open_trace(const char* filename) {
    auto* t = tb();
    if (!t || !filename) return 1;
    t->openTrace(filename);
    return 0;
}

int ams_close_trace() {
    auto* t = tb();
    if (!t) return 1;
    t->closeTrace();
    return 0;
}

double ams_get_vdd() {
    auto* t = tb();
    if (!t) return 0.0;
    return t->vdd();
}

double ams_get_clock_period_ns() {
    auto* t = tb();
    if (!t) return 0.0;
    return t->clockPeriod() * 1e9;
}

double ams_get_sim_time_ns() {
    auto* t = tb();
    if (!t) return 0.0;
    return t->simTime() * 1e9;
}

} // extern "C"
