#pragma once

#include <condition_variable>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <vector>

#include "ams/ams_types.h"

extern "C" {
struct vecvaluesall;
struct vecinfoall;
}

namespace ams {

class NgSpiceInterface {
public:
    explicit NgSpiceInterface(const AMSConfig& config);
    ~NgSpiceInterface();

    NgSpiceInterface(const NgSpiceInterface&) = delete;
    NgSpiceInterface& operator=(const NgSpiceInterface&) = delete;

    bool init();
    bool loadCircuit(const std::vector<std::string>& netlist_lines);
    bool loadCircuitFromFile(const std::string& path);
    void close();

    // Full step: set inputs, run one clock period, return outputs.
    // Kept for backward compatibility with existing C++ testbenches.
    AMSExchange step(const std::map<std::string, double>& digital_outputs);

    // Fine-grained phases for UVM-driven cosimulation.
    bool setDigitalInputs(const std::map<std::string, double>& outputs);
    bool runAnalog(double dt);
    std::map<std::string, double> getAnalogOutputs() const;

    // Direct raw access to arbitrary ngspice nodes/sources.
    double getVoltage(const std::string& node_name) const;
    bool setVoltage(const std::string& source_name, double voltage);

    double currentSimTime() const;
    bool isRunning() const;

    std::mutex& mtx() { return mtx_; }
    void set_sim_running(bool v) { sim_running_ = v; }
    void set_step_complete(bool v) { step_complete_ = v; }
    void set_bg_halted(bool v) { bg_halted_ = v; }
    void notify_step_complete() { cv_step_complete_.notify_one(); }
    void notify_bg_halted() { cv_bg_halted_.notify_one(); }

    int onSendData(vecvaluesall* data, int num_structs);
    int onGetVSRCData(double* voltage, double current_time, const char* node_name);
    int onGetSyncData(double actual_time, double* delta_time, double old_delta_time, int redo_step, int location);
    void onBGThreadRunning(bool not_running);

private:
    AMSConfig config_;

    std::mutex mtx_;
    std::condition_variable cv_step_complete_;
    std::condition_variable cv_bg_halted_;

    std::map<std::string, double> digital_inputs_;
    std::map<std::string, double> analog_outputs_;
    double current_time_;
    double target_time_;
    bool step_complete_;
    bool bg_halted_;
    bool sim_running_;
    bool initialized_;
    bool first_step_;

    std::vector<std::string> netlist_storage_;
};

} // namespace ams