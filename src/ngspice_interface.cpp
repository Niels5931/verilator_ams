#include "ams/ngspice_interface.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <cstring>
#include <chrono>
#include <thread>

extern "C" {
#include <ngspice/sharedspice.h>
}

namespace ams {

static constexpr double TIME_EPSILON = 1e-12;

static int ams_send_char(char* str, int ident, void* userdata) {
    if (str) std::cerr << "[AMS-NG] " << str << std::endl;
    return 0;
}
static int ams_send_stat(char* status, int ident, void* userdata) { return 0; }

static int ams_controlled_exit(int exit_status, NG_BOOL immediate, NG_BOOL quit_on, int ident, void* userdata) {
    auto* self = static_cast<NgSpiceInterface*>(userdata);
    std::cerr << "[AMS] ngspice exit: status=" << exit_status
              << " immediate=" << immediate << " quit=" << quit_on << std::endl;
    if (!quit_on) {
        std::lock_guard<std::mutex> lock(self->mtx());
        self->set_sim_running(false);
        self->set_step_complete(true);
        self->set_bg_halted(true);
        self->notify_step_complete();
        self->notify_bg_halted();
    }
    return 0;
}

static int ams_send_data(pvecvaluesall data, int num_structs, int ident, void* userdata) {
    auto* self = static_cast<NgSpiceInterface*>(userdata);
    return self->onSendData(data, num_structs);
}

static int ams_send_init_data(pvecinfoall data, int ident, void* userdata) {
    return 0;
}

static int ams_bg_thread_running(NG_BOOL not_running, int ident, void* userdata) {
    auto* self = static_cast<NgSpiceInterface*>(userdata);
    self->onBGThreadRunning(not_running != 0);
    return 0;
}

static int ams_get_vsrc_data(double* voltage, double current_time, char* node_name, int ident, void* userdata) {
    auto* self = static_cast<NgSpiceInterface*>(userdata);
    return self->onGetVSRCData(voltage, current_time, node_name);
}

static int ams_get_isrc_data(double* current, double current_time, char* node_name, int ident, void* userdata) {
    return 0;
}

static int ams_get_sync_data(double actual_time, double* delta_time, double old_delta_time, int redo_step, int ident, int location, void* userdata) {
    auto* self = static_cast<NgSpiceInterface*>(userdata);
    return self->onGetSyncData(actual_time, delta_time, old_delta_time, redo_step, location);
}

NgSpiceInterface::NgSpiceInterface(const AMSConfig& config)
    : config_(config)
    , current_time_(0.0)
    , target_time_(0.0)
    , step_complete_(false)
    , bg_halted_(false)
    , sim_running_(false)
    , initialized_(false)
    , first_step_(true)
{}

NgSpiceInterface::~NgSpiceInterface() {
    close();
}

bool NgSpiceInterface::init() {
    if (initialized_) return true;

    int ret = ngSpice_Init(
        ams_send_char,
        ams_send_stat,
        ams_controlled_exit,
        ams_send_data,
        ams_send_init_data,
        ams_bg_thread_running,
        static_cast<void*>(this)
    );

    if (ret != 0) {
        std::cerr << "[AMS] ngSpice_Init failed: " << ret << std::endl;
        return false;
    }

    ret = ngSpice_Init_Sync(
        ams_get_vsrc_data,
        ams_get_isrc_data,
        ams_get_sync_data,
        nullptr,
        static_cast<void*>(this)
    );

    if (ret != 0) {
        std::cerr << "[AMS] ngSpice_Init_Sync failed: " << ret << std::endl;
        return false;
    }

    initialized_ = true;
    current_time_ = 0.0;
    target_time_ = 0.0;
    first_step_ = true;
    return true;
}

bool NgSpiceInterface::loadCircuit(const std::vector<std::string>& netlist_lines) {
    if (!initialized_) {
        std::cerr << "[AMS] ngspice not initialized" << std::endl;
        return false;
    }

    netlist_storage_ = netlist_lines;

    std::vector<char*> cstr_vec;
    for (auto& line : netlist_storage_) {
        cstr_vec.push_back(line.data());
    }
    cstr_vec.push_back(nullptr);

    int ret = ngSpice_Circ(cstr_vec.data());
    if (ret != 0) {
        std::cerr << "[AMS] ngSpice_Circ failed: " << ret << std::endl;
        return false;
    }

    return true;
}

bool NgSpiceInterface::loadCircuitFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[AMS] Cannot open file: " << path << std::endl;
        return false;
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file, line)) {
        size_t end = line.find_last_not_of(" \t\r\n");
        if (end != std::string::npos) {
            line = line.substr(0, end + 1);
        }
        if (!line.empty()) {
            lines.push_back(line);
        }
    }

    return loadCircuit(lines);
}

void NgSpiceInterface::close() {
    if (!initialized_) return;

    bool was_running;
    {
        std::lock_guard<std::mutex> lock(mtx_);
        was_running = sim_running_;
    }

    if (was_running) {
        ngSpice_Command(const_cast<char*>("bg_halt"));
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        std::unique_lock<std::mutex> lock(mtx_);
        cv_bg_halted_.wait(lock, [this] { return bg_halted_; });
    }

    ngSpice_Command(const_cast<char*>("quit"));

    {
        std::lock_guard<std::mutex> lock(mtx_);
        initialized_ = false;
        sim_running_ = false;
    }
}

AMSExchange NgSpiceInterface::step(const std::map<std::string, double>& digital_outputs) {
    std::unique_lock<std::mutex> lock(mtx_);

    digital_inputs_ = digital_outputs;
    target_time_ = current_time_ + config_.clock_period;
    step_complete_ = false;
    bg_halted_ = false;

    ngSpice_SetBkpt(target_time_);

    lock.unlock();
    if (first_step_) {
        ngSpice_Command(const_cast<char*>("bg_run"));
        first_step_ = false;
    } else {
        ngSpice_Command(const_cast<char*>("bg_resume"));
    }
    lock.lock();

    sim_running_ = true;

    cv_step_complete_.wait(lock, [this] { return step_complete_; });

    lock.unlock();
    ngSpice_Command(const_cast<char*>("bg_halt"));
    lock.lock();

    cv_bg_halted_.wait(lock, [this] { return bg_halted_; });
    bg_halted_ = false;

    AMSExchange result;
    result.simulation_time = current_time_;
    result.analog_outputs = analog_outputs_;
    result.digital_inputs = digital_inputs_;

    return result;
}

double NgSpiceInterface::currentSimTime() const {
    return current_time_;
}

bool NgSpiceInterface::isRunning() const {
    return sim_running_;
}

int NgSpiceInterface::onSendData(pvecvaluesall data, int num_structs) {
    if (!data || data->veccount == 0) return 0;

    double sim_time = -1.0;
    for (int i = 0; i < data->veccount; i++) {
        if (data->vecsa[i]->is_scale) {
            sim_time = data->vecsa[i]->creal;
            break;
        }
    }

    if (sim_time < 0.0) return 0;

    std::lock_guard<std::mutex> lock(mtx_);

    double TOLERANCE = config_.clock_period * 0.01;

    if (!step_complete_ && sim_time >= target_time_ - TOLERANCE) {
        analog_outputs_.clear();
        for (const auto* sig : config_.analog_to_digital()) {
            std::string target_lower;
            for (char c : sig->spice_name) target_lower += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            std::string vec_name_paren = "v(" + target_lower + ")";
            for (int i = 0; i < data->veccount; i++) {
                std::string actual_name(data->vecsa[i]->name);
                std::string actual_lower;
                for (char c : actual_name) actual_lower += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                if (actual_lower == target_lower || actual_lower == vec_name_paren) {
                    analog_outputs_[sig->spice_name] = data->vecsa[i]->creal;
                    break;
                }
            }
        }

        current_time_ = sim_time;
        step_complete_ = true;
        cv_step_complete_.notify_one();
    }

    return 0;
}

int NgSpiceInterface::onGetVSRCData(double* voltage, double current_time, const char* node_name) {
    if (!voltage || !node_name) return 0;

    std::lock_guard<std::mutex> lock(mtx_);

    for (const auto* sig : config_.digital_to_analog()) {
        if (sig->spice_name == node_name) {
            auto it = digital_inputs_.find(sig->spice_name);
            if (it != digital_inputs_.end()) {
                *voltage = it->second;
                return 0;
            }
        }
    }

    *voltage = 0.0;
    return 0;
}

int NgSpiceInterface::onGetSyncData(double actual_time, double* delta_time, double old_delta_time, int redo_step, int location) {
    std::lock_guard<std::mutex> lock(mtx_);

    if (location == 0) {
        if (actual_time + *delta_time > target_time_ + TIME_EPSILON) {
            *delta_time = target_time_ - actual_time;
            if (*delta_time < TIME_EPSILON) {
                *delta_time = TIME_EPSILON;
            }
            return 1;
        }
    }

    return 0;
}

void NgSpiceInterface::onBGThreadRunning(bool not_running) {
    std::lock_guard<std::mutex> lock(mtx_);
    sim_running_ = !not_running;
    if (not_running) {
        bg_halted_ = true;
        cv_bg_halted_.notify_one();
    }
}

} // namespace ams