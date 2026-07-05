#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

#include "Vams_top.h"
#include "verilated.h"

#include "ams/ams_dpi.h"
#include "ams/ams_testbench.h"
#include "ams/ams_types.h"
#include "ams/config_parser.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <config.yaml>" << std::endl;
        return 1;
    }

    ams::AMSConfig config;
    try {
        config = ams::loadConfig(argv[1]);
    } catch (const std::exception& e) {
        std::cerr << "Failed to load config: " << e.what() << std::endl;
        return 1;
    }

    // The Verilated top module owns the UVM processes and the SystemVerilog
    // clock/reset generation.  The C++ side only registers the port mapping
    // between the DUT and ngspice.
    auto tb = std::make_unique<ams::AMSTestbench<Vams_top>>(config);

    tb->output_accessors_["b0"] = [](const Vams_top* dut) -> uint32_t {
        return dut->b0;
    };
    tb->output_accessors_["b1"] = [](const Vams_top* dut) -> uint32_t {
        return dut->b1;
    };
    tb->output_accessors_["b2"] = [](const Vams_top* dut) -> uint32_t {
        return dut->b2;
    };
    tb->output_accessors_["b3"] = [](const Vams_top* dut) -> uint32_t {
        return dut->b3;
    };

    tb->input_writers_["vout"] = [](Vams_top* dut, uint32_t val) {
        dut->vout = val;
    };

    // Register with the DPI bridge so SystemVerilog/UVM can call ams_* functions.
    ams::ams_register_testbench(tb.get());

    // Initialize Verilator command args.  The actual AMS init is triggered from
    // SystemVerilog via ams_dpi_pkg::ams_init().
    Verilated::commandArgs(argc, argv);

    // Standard Verilator event loop.  UVM drives everything from SV.
    while (!Verilated::gotFinish()) {
        tb->dut()->eval();
    }

    return 0;
}
