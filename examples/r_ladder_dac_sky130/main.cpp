#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "Vams_top.h"
#include "verilated.h"

#include "ams/ams_testbench.h"
#include "ams/ams_types.h"
#include "ams/config_parser.h"
#include "ams/ngspice_interface.h"

namespace {

std::vector<std::string> loadNetlist(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open netlist: " + path);
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file, line)) {
        lines.push_back(line);
    }
    return lines;
}

std::vector<std::string> substitutePdkRoot(const std::vector<std::string>& netlist,
                                           const std::string& pdk_root) {
    std::vector<std::string> result;
    for (const auto& line : netlist) {
        std::string modified = line;
        size_t pos = 0;
        while ((pos = modified.find("{PDK_ROOT}", pos)) != std::string::npos) {
            modified.replace(pos, 10, pdk_root);
            pos += pdk_root.size();
        }
        result.push_back(modified);
    }
    return result;
}

} // namespace

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

    const char* pdk_root_env = std::getenv("PDK_ROOT");
    if (!pdk_root_env || std::string(pdk_root_env).empty()) {
        std::cerr << "PDK_ROOT environment variable is not set." << std::endl;
        return 1;
    }
    std::string pdk_root(pdk_root_env);

    std::vector<std::string> base_netlist;
    try {
        base_netlist = loadNetlist(config.spice_netlist_path);
    } catch (const std::exception& e) {
        std::cerr << "Failed to load netlist: " << e.what() << std::endl;
        return 1;
    }

    auto netlist = substitutePdkRoot(base_netlist, pdk_root);

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

    if (!tb.init(argc, argv, netlist)) {
        std::cerr << "AMS init failed" << std::endl;
        return 1;
    }

    tb.openTrace("ams_waveforms.vcd");

    std::cout << "Sky130 4-bit R-2R ladder DAC co-simulation\n"
              << "==========================================\n"
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

    std::cout << "==========================================\n"
              << "Simulation complete." << std::endl;
    return 0;
}
