//
// Created by nemanja on 11/26/25.
//

#include "System.h"

#include <format>

#include "core/Camera/Camera.h"
#include "core/Diagnostics/Diagnostics.h"
#include "utils/logger/Logger.h"
#include "core/HAL/HAL.h"

void System::init() {
    state_ = SystemState::Initializing;

    bus_ = std::make_unique<MessageBus>();
    try {
        subsystems_.push_back(std::make_unique<HAL>(bus_.get(), "/dev/gpiochip0"));
        subsystems_.push_back(std::make_unique<Diagnostics>(bus_.get()));
        subsystems_.push_back(std::make_unique<Camera>(bus_.get()));
    } catch (const std::exception& e) {
        Logger::error(std::format("Could not create HAL subsystem: {}", e.what()));
        state_ = SystemState::Faulted;
        return;
    }
    // check if the creation of all subsystems is correct; in opposite, go to the faulted state
    // currently, there is no subsystem that can fail in creation stage
    // note: creation of HAL can throw exception -> catch it here
    state_ = SystemState::Initialized;
    Logger::info("System initialized!");
}

System::~System() {
    try {
        if (state_ != SystemState::Stopped)
            stop();
    } catch (std::exception& e) {
        Logger::error(std::format("Critical: System destructor failed to shut down gracefully.\n Error: {}", e.what()));
    }
}

bool System::start() {
    if (state_ != SystemState::Initialized && state_ != SystemState::Stopped) {
        Logger::error(std::format("Cannot start the system from {} state", toString(state_)));
        return false;
    }

    state_ = SystemState::Starting;
    Logger::info("Starting system ...");

    if (!bus_->start()) {
        Logger::error("Could not start MessageBus. System startup aborted");

        // start() can leave the bus turned off or, in case of inconsistency, leave it turned on
        // stop() will, in both cases, give it a proper state
        bus_->stop();

        state_ = SystemState::Stopped;
        return false;
    }

    std::vector<Subsystem*> started_modules;
    for (const auto& module : subsystems_) {
        if (!module->start()) {
            Logger::error(std::format("Could not start subsystem: {}, starting rollback procedure ...", module->name()));
            state_ = SystemState::Stopping;
            for (auto it = started_modules.rbegin(); it != started_modules.rend(); ++it) {
                if (!(*it)->stop()) {
                    Logger::error(std::format("Could not stop subsystem: {}", (*it)->name()));
                    state_ = SystemState::Faulted;
                    return false;
                }
            }
            Logger::info("System rollback successfully done!");
            state_ = SystemState::Stopped;
            bus_->stop();
            return false;  // system hasn't started successfully, but stayed in consistent state via rollback mechanism
        }

        Logger::info(std::format("Subsystem {} successfully started", module->name()));
        started_modules.push_back(module.get());
    }

    state_ = SystemState::Running;
    Logger::info("System started!");
    return true;
}

bool System::stop() {
    if (state_ != SystemState::Running && state_ != SystemState::Stopped && state_ != SystemState::Faulted) {
        Logger::error(std::format("Cannot stop system from {} state", toString(state_)));
        return false;
    }

    if (state_ == SystemState::Stopped) {
        Logger::warning("System stop() called but system is already stopped");
        return true;
    }

    state_ = SystemState::Stopping;
    for (auto it = subsystems_.rbegin(); it != subsystems_.rend(); ++it) {
        const auto& module = (*it);
        if (!module->stop()) {
            Logger::error(std::format("Could not stop subsystem: {}", module->name()));
            state_ = SystemState::Faulted;
            return false;
        }
        Logger::info(std::format("Subsystem {} stopped", module->name()));
    }

    bus_->stop();
    Logger::info("Message bus stopped!");

    state_ = SystemState::Stopped;
    Logger::info("System stopped!");
    return true;
}
