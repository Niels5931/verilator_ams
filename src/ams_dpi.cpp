#include "ams/ams_dpi.h"

#include <cassert>
#include <cstring>
#include <iostream>
#include <memory>

namespace ams {

static IAMSTestbench* g_active_tb = nullptr;

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
    (void)config_path;  // Config is loaded by the per-DUT shim before registration.
    auto* t = tb();
    if (!t) {
        std::cerr << "[AMS-DPI] ams_init called before testbench registration\n";
        return 1;
    }
    // argc/argv are not available from DPI; pass a dummy argv.  The generated
    // per-DUT shim already called Verilated::commandArgs from main().
    int argc = 1;
    const char* argv[] = {"ams_sim"};
    char** argv_mutable = const_cast<char**>(argv);
    if (!t->init(argc, argv_mutable, {}, false)) {
        std::cerr << "[AMS-DPI] ams_init failed\n";
        return 1;
    }
    return 0;
}

int ams_finish() {
    auto* t = tb();
    if (!t) return 1;
    t->closeTrace();
    return 0;
}

int ams_sync_d2a() {
    auto* t = tb();
    if (!t) return 1;
    t->syncD2A();
    return 0;
}

int ams_run_analog(double dt_ns) {
    auto* t = tb();
    if (!t) return 1;
    t->runAnalog(dt_ns * 1e-9);
    return 0;
}

int ams_sync_a2d() {
    auto* t = tb();
    if (!t) return 1;
    t->syncA2D();
    return 0;
}

int ams_sync(double dt_ns) {
    auto* t = tb();
    if (!t) return 1;
    t->sync(dt_ns * 1e-9);
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
