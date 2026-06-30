#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
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

constexpr int ADC_BITS = 4;
constexpr double VDD = 1.8;
constexpr int CYCLES_PER_CONVERSION = 7;

std::string trim(const std::string& s) {
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) ++start;
    size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) --end;
    return s.substr(start, end - start);
}

std::vector<std::string> loadNetlistWithIncludes(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open netlist: " + path.string());
    }

    std::filesystem::path base_dir = path.parent_path();
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file, line)) {
        std::string trimmed = trim(line);
        if (trimmed.size() > 8 && trimmed.substr(0, 8) == ".include") {
            std::string include_arg = trim(trimmed.substr(8));
            if (!include_arg.empty() && include_arg.front() == '"' && include_arg.back() == '"') {
                include_arg = include_arg.substr(1, include_arg.size() - 2);
            }

            std::filesystem::path include_path = include_arg;
            if (include_path.is_relative()) {
                include_path = std::filesystem::weakly_canonical(base_dir / include_path);
            }

            auto included = loadNetlistWithIncludes(include_path);
            lines.insert(lines.end(), included.begin(), included.end());
        } else {
            lines.push_back(line);
        }
    }
    return lines;
}

std::vector<std::string> substituteVin(const std::vector<std::string>& netlist, double vin) {
    std::ostringstream ss;
    ss << vin;
    std::string vin_str = ss.str();

    std::vector<std::string> result;
    for (const auto& line : netlist) {
        std::string modified = line;
        size_t pos = 0;
        while ((pos = modified.find("{VIN}", pos)) != std::string::npos) {
            modified.replace(pos, 5, vin_str);
            pos += vin_str.size();
        }
        result.push_back(modified);
    }
    return result;
}

int expectedCode(double vin, int bits, double vdd) {
    double lsb = vdd / (1 << bits);
    int code = static_cast<int>(std::floor(vin / lsb));
    int max_code = (1 << bits) - 1;
    return std::max(0, std::min(code, max_code));
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <config.yaml>" << std::endl;
        return 1;
    }

    ams::AMSConfig base_config;
    try {
        base_config = ams::loadConfig(argv[1]);
    } catch (const std::exception& e) {
        std::cerr << "Failed to load config: " << e.what() << std::endl;
        return 1;
    }

    std::vector<std::string> base_netlist;
    try {
        base_netlist = loadNetlistWithIncludes(base_config.spice_netlist_path);
    } catch (const std::exception& e) {
        std::cerr << "Failed to load netlist: " << e.what() << std::endl;
        return 1;
    }

    const std::vector<double> test_voltages = {0.0, 0.45, 0.9, 1.35, 1.8};

    std::cout << "4-bit SAR ADC with ideal R-2R ladder DAC co-simulation" << std::endl;
    std::cout << "========================================================" << std::endl;

    bool all_pass = true;

    for (double vin : test_voltages) {
        auto netlist = substituteVin(base_netlist, vin);

        ams::AMSTestbench<Vams_top> tb(base_config);

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

        tb.input_writers_["cmp_in"] = [](Vams_top* dut, uint32_t val) {
            dut->cmp_in = val & 1;
        };

        if (!tb.init(argc, argv, netlist)) {
            std::cerr << "AMS init failed for Vin=" << vin << std::endl;
            return 1;
        }

        for (int i = 0; i < CYCLES_PER_CONVERSION; i++) {
            tb.step();
        }

        int code = static_cast<int>(tb.dut()->adc_code);
        int expected = expectedCode(vin, ADC_BITS, VDD);
        bool pass = (code == expected);
        all_pass = all_pass && pass;

        std::cout << "Vin=" << vin << "V"
                  << " | adc_code=" << code
                  << " | expected=" << expected
                  << " | " << (pass ? "PASS" : "FAIL")
                  << std::endl;
    }

    std::cout << "========================================================" << std::endl;
    std::cout << (all_pass ? "All tests passed." : "Some tests failed.") << std::endl;
    return all_pass ? 0 : 1;
}
