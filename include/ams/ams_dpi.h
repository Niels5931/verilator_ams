#pragma once

#include <string>
#include <vector>

#include "ams/ams_types.h"

namespace ams {

// Abstract interface to an AMS testbench.  Concrete instances are created
// from a generated per-DUT shim and registered with the DPI bridge before
// the simulation starts.
class IAMSTestbench {
public:
    virtual ~IAMSTestbench() = default;

    virtual bool init(int argc, char** argv,
                      const std::vector<std::string>& netlist = {},
                      bool reset_dut = true) = 0;

    // Fine-grained AMS synchronization.
    virtual void runAnalog(double dt) = 0;

    // Direct analog access.
    virtual double getVoltage(const std::string& node) const = 0;
    virtual void setVoltage(const std::string& source, double voltage) = 0;

    // VCD trace control.
    virtual void openTrace(const std::string& filename, int depth = 99) = 0;
    virtual void closeTrace() = 0;

    // Configuration queries.
    virtual double vdd() const = 0;
    virtual double clockPeriod() const = 0;
    virtual double simTime() const = 0;
};

// Called by the generated per-DUT shim to make the testbench reachable from
// the DPI-C functions.  The pointer must remain valid for the simulation.
void ams_register_testbench(IAMSTestbench* tb);

// Returns the currently registered testbench, or nullptr if none.
IAMSTestbench* ams_active_testbench();

} // namespace ams

// DPI-C functions imported by ams_dpi_pkg.sv.
extern "C" {

int ams_init(const char* config_path);
int ams_finish();

int ams_run_analog(double dt_ns);

double ams_get_voltage(const char* node_name);
int ams_set_voltage(const char* source_name, double voltage);

int ams_open_trace(const char* filename);
int ams_close_trace();

double ams_get_vdd();
double ams_get_clock_period_ns();
double ams_get_sim_time_ns();

} // extern "C"
