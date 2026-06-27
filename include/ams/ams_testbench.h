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

#include "ams/ams_types.h"
#include "ams/ngspice_interface.h"

namespace ams {

template <typename VTop>
class AMSTestbench {
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

    ~AMSTestbench() {
        if (tracing_) closeTrace();
        dut_->final();
        ngspice_->close();
    }

    AMSTestbench(const AMSTestbench&) = delete;
    AMSTestbench& operator=(const AMSTestbench&) = delete;

    bool init(int argc, char** argv) {
        Verilated::commandArgs(argc, argv);

        if (!ngspice_->init()) return false;
        if (!ngspice_->loadCircuitFromFile(config_.spice_netlist_path)) return false;

        dut_->rst_n = 0;
        for (int i = 0; i < 5; i++) {
            tickDut();
        }
        dut_->rst_n = 1;
        dut_->eval();

        return true;
    }

    void run(uint64_t num_cycles) {
        for (uint64_t i = 0; i < num_cycles && !Verilated::gotFinish(); i++) {
            step();
        }
    }

    void step() {
        auto digital_outputs = collectDigitalOutputs();
        AMSExchange exchange = ngspice_->step(digital_outputs);
        applyAnalogInputs(exchange);
        tickDut();
        cycle_++;
    }

    VTop* dut() { return dut_.get(); }
    NgSpiceInterface& ngspice() { return *ngspice_; }
    uint64_t cycle() const { return cycle_; }
    double simTime() const { return cycle_ * config_.clock_period; }

    void openTrace(const std::string& filename, int depth = 99) {
        Verilated::traceEverOn(true);
        trace_ = new VerilatedVcdC;
        dut_->trace(trace_, depth);
        trace_->open(filename.c_str());
        tracing_ = true;
    }

    void closeTrace() {
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

    void applyAnalogInputs(const AMSExchange& exchange) {
        for (auto* sig : config_.analog_to_digital()) {
            auto it = input_writers_.find(sig->verilog_name);
            if (it == input_writers_.end()) continue;
            auto vit = exchange.analog_outputs.find(sig->spice_name);
            if (vit == exchange.analog_outputs.end()) continue;
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