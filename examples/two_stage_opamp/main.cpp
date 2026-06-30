#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

#include "Vams_top.h"
#include "verilated.h"

#include "ams/ams_types.h"
#include "ams/ams_testbench.h"
#include "ams/config_parser.h"
#include "ams/ngspice_interface.h"

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

    ams::AMSTestbench<Vams_top> tb(config);

    tb.output_accessors_["digital_out"] = [](const Vams_top* dut) -> uint32_t {
        return dut->digital_out;
    };
    tb.input_writers_["analog_in"] = [](Vams_top* dut, uint32_t val) {
        dut->analog_in = val;
    };
    tb.input_writers_["analog_in_valid"] = [](Vams_top* dut, uint32_t val) {
        dut->analog_in_valid = val;
    };

    if (!tb.init(argc, argv)) {
        std::cerr << "AMS init failed" << std::endl;
        return 1;
    }

    tb.openTrace("ams_waveforms.vcd");

    const uint64_t num_cycles = 4;
    std::cout << "Running AMS co-simulation for " << num_cycles
              << " cycles (clock=" << config.clock_period * 1e9 << "ns)..." << std::endl;

    for (uint64_t i = 0; i < num_cycles; i++) {
        tb.step();
        tb.dut()->analog_in_valid = 1;

        uint32_t analog_raw = tb.dut()->analog_in;
        double voltage = ams::from_fixed(static_cast<int32_t>(analog_raw));
        std::cout << "Cycle " << i
                  << " | digital_out=" << static_cast<int>(tb.dut()->digital_out)
                  << " | VO=" << voltage << "V"
                  << " | ngspice_time=" << tb.ngspice().currentSimTime()
                  << std::endl;
    }

    std::cout << "Simulation complete." << std::endl;
    return 0;
}