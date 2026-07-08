#pragma once

#include <memory>
#include <string>
#include <vector>

#include "ams/ams_dpi.h"
#include "ams/ams_types.h"
#include "ams/ngspice_interface.h"

namespace ams {

// DUT-less AMS bridge for UVM/SystemVerilog-driven cosimulation.
// SV owns the DUT, clock/reset, and signal mapping.  This class only
// manages the ngspice session and exposes it through IAMSTestbench so
// the DPI-C functions in ams_dpi.cpp can reach it.
class AMSBridge : public IAMSTestbench {
public:
    explicit AMSBridge(const AMSConfig& config)
        : config_(config)
        , ngspice_(new NgSpiceInterface(config))
    {}

    bool init(int argc, char** argv,
              const std::vector<std::string>& netlist = {},
              bool reset_dut = true) override {
        (void)argc;
        (void)argv;
        (void)reset_dut;
        if (!ngspice_->init()) return false;
        if (netlist.empty()) {
            if (!ngspice_->loadCircuitFromFile(config_.spice_netlist_path)) return false;
        } else {
            if (!ngspice_->loadCircuit(netlist)) return false;
        }
        return true;
    }

    void runAnalog(double dt) override { ngspice_->runAnalog(dt); }

    double getVoltage(const std::string& node_name) const override {
        return ngspice_->getVoltage(node_name);
    }

    void setVoltage(const std::string& source_name, double voltage) override {
        ngspice_->setVoltage(source_name, voltage);
    }

    void openTrace(const std::string& filename, int depth = 99) override {
        (void)filename;
        (void)depth;
    }

    void closeTrace() override {}

    double vdd() const override { return config_.vdd; }
    double clockPeriod() const override { return config_.clock_period; }
    double simTime() const override { return ngspice_->currentSimTime(); }

private:
    AMSConfig config_;
    std::unique_ptr<NgSpiceInterface> ngspice_;
};

} // namespace ams