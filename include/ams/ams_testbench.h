#pragma once

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <map>

#include "verilated.h"
#include "verilated_vcd_c.h"

#include "ams/ams_dpi.h"
#include "ams/ams_types.h"
#include "ams/ngspice_interface.h"

namespace ams {

template <typename VTop>
class AMSTestbench : public IAMSTestbench {
public:
    using PortAccessor = std::function<uint32_t(const VTop*)>;
    using PortWriter = std::function<void(VTop*, uint32_t)>;

    std::map<std::string, PortAccessor> output_accessors_;
    std::map<std::string, PortWriter> input_writers_;

    explicit AMSTestbench(const AMSConfig& config)
        : config_(config)
        , dut_(new VTop("top"))
        , ngspice_(new NgSpiceInterface(config))
        , cycle_(0)
        , sim_time_(0)
        , trace_(nullptr)
        , tracing_(false)
    {}

    virtual ~AMSTestbench() {
        if (tracing_) closeTrace();
        dut_->final();
        ngspice_->close();
    }

    AMSTestbench(const AMSTestbench&) = delete;
    AMSTestbench& operator=(const AMSTestbench&) = delete;

    bool init(int argc, char** argv, const std::vector<std::string>& netlist_lines = {},
              bool reset_dut = true) override {
        Verilated::commandArgs(argc, argv);

        if (!ngspice_->init()) return false;
        if (netlist_lines.empty()) {
            if (!ngspice_->loadCircuitFromFile(config_.spice_netlist_path)) return false;
        } else {
            if (!ngspice_->loadCircuit(netlist_lines)) return false;
        }

        if (reset_dut) {
            dut_->rst_n = 0;
            for (int i = 0; i < 5; i++) {
                tickDut();
            }
            dut_->rst_n = 1;
            dut_->eval();
        }

        return true;
    }

    void run(uint64_t num_cycles) {
        for (uint64_t i = 0; i < num_cycles && !Verilated::gotFinish(); i++) {
            step();
        }
    }

    // Full clock-cycle step with C++ clock ownership.
    // Kept for backward compatibility with existing C++ testbenches.
    void step() {
        syncD2A();
        runAnalog(config_.clock_period);
        syncA2D();
        tickDut();
        cycle_++;
    }

    // Fine-grained AMS synchronization for UVM-driven cosimulation.
    // The caller (UVM/SV) owns the clock and reset.
    void syncD2A() override {
        ngspice_->setDigitalInputs(collectDigitalOutputs());
    }

    void runAnalog(double dt) override {
        ngspice_->runAnalog(dt);
    }

    void syncA2D() override {
        applyAnalogInputs(ngspice_->getAnalogOutputs());
    }

    void applyAnalogInputs(const std::map<std::string, double>& analog_outputs) {
        for (auto* sig : config_.analog_to_digital()) {
            auto it = input_writers_.find(sig->verilog_name);
            if (it == input_writers_.end()) continue;
            auto vit = analog_outputs.find(sig->spice_name);
            if (vit == analog_outputs.end()) continue;
            double voltage = vit->second;
            if (sig->width == 1) {
                it->second(dut_.get(), voltage >= sig->vdd / 2.0 ? 1 : 0);
            } else if (sig->width == 32) {
                it->second(dut_.get(), static_cast<uint32_t>(to_fixed(voltage)));
            } else {
                it->second(dut_.get(), static_cast<uint32_t>(voltage));
            }
        }
    }

    void sync(double dt) override {
        syncD2A();
        runAnalog(dt);
        syncA2D();
    }

    VTop* dut() { return dut_.get(); }
    NgSpiceInterface& ngspice() { return *ngspice_; }
    uint64_t cycle() const { return cycle_; }
    double simTime() const override { return ngspice_->currentSimTime(); }

    double vdd() const override { return config_.vdd; }
    double clockPeriod() const override { return config_.clock_period; }

    double getVoltage(const std::string& node_name) const override {
        return ngspice_->getVoltage(node_name);
    }

    void setVoltage(const std::string& source_name, double voltage) override {
        ngspice_->setVoltage(source_name, voltage);
    }

    void openTrace(const std::string& filename, int depth = 99) override {
        Verilated::traceEverOn(true);
        trace_ = new VerilatedVcdC;
        dut_->trace(trace_, depth);
        trace_->open(filename.c_str());
        tracing_ = true;
    }

    void closeTrace() override {
        if (trace_) {
            trace_->close();
            trace_ = nullptr;
        }
        tracing_ = false;
    }

private:
    AMSConfig config_;
    std::unique_ptr<VTop> dut_;
    std::unique_ptr<NgSpiceInterface> ngspice_;

    uint64_t cycle_;
    vluint64_t sim_time_;

    VerilatedVcdC* trace_;
    bool tracing_;

    std::map<std::string, double> collectDigitalOutputs() {
        std::map<std::string, double> outputs;
        for (auto* sig : config_.digital_to_analog()) {
            auto it = output_accessors_.find(sig->verilog_name);
            if (it == output_accessors_.end()) continue;
            uint32_t raw = it->second(dut_.get());
            if (sig->width == 1) {
                outputs[sig->spice_name] = (raw & 1) ? sig->vdd : 0.0;
            } else if (sig->width == 32) {
                outputs[sig->spice_name] = from_fixed(static_cast<int32_t>(raw));
            } else {
                outputs[sig->spice_name] = static_cast<double>(raw);
            }
        }
        return outputs;
    }

    void tickDut() {
        dut_->clk = 1;
        dut_->eval();
        if (tracing_) trace_->dump(sim_time_++);
        dut_->clk = 0;
        dut_->eval();
        if (tracing_) trace_->dump(sim_time_++);
    }
};

} // namespace ams