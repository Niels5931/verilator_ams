#pragma once

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "ams/ams_types.h"
#include "yaml-cpp/yaml.h"

namespace ams {

namespace detail {

inline void require(const YAML::Node& node, const std::string& key) {
    if (!node[key]) {
        throw std::runtime_error("Missing required config field: " + key);
    }
}

template <typename T>
inline T get_required(const YAML::Node& node, const std::string& key) {
    require(node, key);
    try {
        return node[key].as<T>();
    } catch (const YAML::Exception& e) {
        throw std::runtime_error("Invalid value for config field '" + key + "': " + e.what());
    }
}

template <typename T>
inline T get_optional(const YAML::Node& node, const std::string& key, const T& default_val) {
    if (!node[key]) return default_val;
    try {
        return node[key].as<T>();
    } catch (const YAML::Exception& e) {
        throw std::runtime_error("Invalid value for config field '" + key + "': " + e.what());
    }
}

} // namespace detail

inline AMSConfig loadConfig(const std::string& path) {
    YAML::Node root;
    try {
        root = YAML::LoadFile(path);
    } catch (const YAML::Exception& e) {
        throw std::runtime_error("Failed to load config file '" + path + "': " + e.what());
    }

    detail::require(root, "ams_cosim");
    const YAML::Node cosim = root["ams_cosim"];

    AMSConfig config;
    config.clock_period = detail::get_required<double>(cosim, "clock_period_ns") * 1e-9;
    config.vdd = detail::get_required<double>(cosim, "vdd");
    config.spice_netlist_path = detail::get_required<std::string>(cosim, "spice_netlist");

    detail::require(cosim, "digital_to_analog");
    for (const auto& item : cosim["digital_to_analog"]) {
        AMSSignal sig;
        sig.name = detail::get_optional<std::string>(item, "name", "");
        sig.spice_name = detail::get_required<std::string>(item, "spice_source");
        sig.verilog_name = detail::get_required<std::string>(item, "verilog_port");
        sig.direction = SignalDirection::DIGITAL_TO_ANALOG;
        sig.width = detail::get_required<int>(item, "width");
        sig.vdd = detail::get_optional<double>(item, "vdd", config.vdd);
        config.signals.push_back(sig);
    }

    detail::require(cosim, "analog_to_digital");
    for (const auto& item : cosim["analog_to_digital"]) {
        AMSSignal sig;
        sig.name = detail::get_optional<std::string>(item, "name", "");
        sig.spice_name = detail::get_required<std::string>(item, "spice_node");
        sig.verilog_name = detail::get_required<std::string>(item, "verilog_port");
        sig.direction = SignalDirection::ANALOG_TO_DIGITAL;
        sig.width = detail::get_required<int>(item, "width");
        sig.vdd = detail::get_optional<double>(item, "vdd", config.vdd);
        config.signals.push_back(sig);
    }

    return config;
}

} // namespace ams