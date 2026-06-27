#pragma once

#include <algorithm>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace ams {

inline int32_t to_fixed(double val) {
    double scaled = val * 65536.0;
    scaled = std::max(-2147483648.0, std::min(scaled, 2147483647.0));
    return static_cast<int32_t>(scaled);
}

inline double from_fixed(int32_t val) {
    return static_cast<double>(val) / 65536.0;
}

enum class SignalDirection {
    DIGITAL_TO_ANALOG,
    ANALOG_TO_DIGITAL
};

struct AMSSignal {
    std::string spice_name;
    std::string verilog_name;
    SignalDirection direction;
    int width;
    double vdd;
};

struct AMSConfig {
    double clock_period;
    double vdd;
    std::string spice_netlist_path;
    std::vector<AMSSignal> signals;

    std::vector<const AMSSignal*> digital_to_analog() const {
        std::vector<const AMSSignal*> result;
        for (auto& s : signals) {
            if (s.direction == SignalDirection::DIGITAL_TO_ANALOG) {
                result.push_back(&s);
            }
        }
        return result;
    }

    std::vector<const AMSSignal*> analog_to_digital() const {
        std::vector<const AMSSignal*> result;
        for (auto& s : signals) {
            if (s.direction == SignalDirection::ANALOG_TO_DIGITAL) {
                result.push_back(&s);
            }
        }
        return result;
    }
};

struct AMSExchange {
    double simulation_time;
    std::map<std::string, double> analog_outputs;
    std::map<std::string, double> digital_inputs;
};

} // namespace ams