#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>

#include "Vams_top.h"
#include "verilated.h"

#include "ams/ams_testbench.h"
#include "ams/ams_types.h"
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

    tb.output_accessors_["b0"] = [](const Vams_top* dut) -> uint32_t {
        return dut->b0;
    };
    tb.output_accessors_["b1"] = [](const Vams_top* dut) -> uint32_t {
        return dut->b1;
    };
    tb.output_accessors_["b2"] = [](const Vams_top* dut) -> uint32_t {
        return dut->b2;
    };
    tb.output_accessors_["b3"] = [](const Vams_top* dut) -> uint32_t {
        return dut->b3;
    };

    tb.input_writers_["vout"] = [](Vams_top* dut, uint32_t val) {
        dut->vout = val;
    };

    if (!tb.init(argc, argv)) {
        std::cerr << "AMS init failed" << std::endl;
        return 1;
    }

    tb.openTrace("ams_waveforms.vcd");

    std::cout << "4-bit R-2R ladder DAC co-simulation\n"
              << "====================================\n"
              << std::fixed << std::setprecision(4);

    int prev_code = 0;
    constexpr int NUM_CODES = 16;

    for (int i = 0; i < NUM_CODES; ++i) {
        tb.step();

        int next_code = (tb.dut()->b0 ? 1 : 0)
                      | (tb.dut()->b1 ? 2 : 0)
                      | (tb.dut()->b2 ? 4 : 0)
                      | (tb.dut()->b3 ? 8 : 0);

        uint32_t vout_raw = tb.dut()->vout;
        double vout = ams::from_fixed(static_cast<int32_t>(vout_raw));
        double expected = (config.vdd * prev_code) / 16.0;

        std::cout << "code=" << std::setw(2) << prev_code
                  << " | vout=" << std::setw(6) << vout << "V"
                  << " | expected=" << std::setw(6) << expected << "V"
                  << std::endl;

        prev_code = next_code;
    }

    std::cout << "====================================\n"
              << "Simulation complete." << std::endl;
    return 0;
}
