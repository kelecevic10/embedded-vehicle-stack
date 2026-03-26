//
// Created by nemanja on 11/26/25.
//

#include "System.h"

#include <format>

#include "core/Diagnostics/Diagnostics.h"
#include "utils/logger/Logger.h"


void System::init() {
    subsystems_.push_back(std::make_unique<Diagnostics>());
    state_ = SystemState::Initializing;
}

System::~System() {
    if (state_ != SystemState::Stopped) {
        Logger::warning("System destructor invoked while system is not stopped, attempting graceful shut down ...");
        try {
            if (!stop()) {
                Logger::error("Could not stop system gracefully!");
            }
        } catch (const std::exception& e) {
            Logger::error(std::format("System destructor encountered an unexpected error during shutdown: {}", e.what()));
        }
    }
}

bool System::start() {
    if (state_ != SystemState::Initializing && state_ != SystemState::Stopped) {
        Logger::error(std::format("Cannot start the system from {} state", toString(state_)));
        return false;
    }

    state_ = SystemState::Starting;
    for (const auto& module : subsystems_) {
        if (!module->start()) {
            Logger::error(std::format("Could not start subsystem: ", module->name()));
            state_ = SystemState::Faulted;
            return false;
        }
        Logger::info(std::format("Subsystem {} started", module->name()));
    }

    state_ = SystemState::Running;
    Logger::info("System started!");
    return true;
}

bool System::stop() {
    if (state_ != SystemState::Running) {
        Logger::error(std::format("Cannot stop the system from {} state", toString(state_)));
        return false;
    }
    state_ = SystemState::Stopping;
    for (const auto& module : subsystems_) {
        if (!module->stop()) {
            Logger::error(std::format("Could not stop subsystem: ", module->name()));
            state_ = SystemState::Faulted;
            return false;
        }
        Logger::info(std::format("Subsystem {} stopped", module->name()));
    }
    state_ = SystemState::Stopped;
    Logger::info("System stopped!");
    return true;
}
