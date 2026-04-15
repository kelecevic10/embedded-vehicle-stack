//
// Created by nemanja on 4/7/26.
//

#include "HAL.h"

#include <csignal>

#include "Devices/LED.h"

bool HAL::registerDevices() {
    using namespace hal;
    try {
        devices_.push_back(std::make_unique<LED>(17,
            "LED",
            gpioManager_.get(),
            OutputPinConfig{17, "LED", false, PinState::LOW}
            ));
    } catch (std::system_error& e) {
        Logger::error(std::format("Could not register Device, error: {}", e.what()));
        return false;
    };

    return true;
}

HAL::HAL(MessageBus* msg, std::string chip_path)
    : messageBus_(msg)
{
    try {
        gpioManager_ = std::make_unique<GpioManager>(std::move(chip_path));
    } catch (std::system_error& e) {
        Logger::error(std::format("Could not open the system chip: {}", e.what()));
        throw;
    };
}

HAL::~HAL() {
}


bool HAL::start() {
    if (thread_.joinable()) {
        Logger::error("HAL already started!");
        return false;
    }

    if (devices_.empty()) {
        if (!registerDevices()) {
            Logger::error("Could not register devices!");
            return false;
        }
    }

    thread_ = std::jthread(&HAL::loop, this);
    return true;
}

bool HAL::stop() {
    if (thread_.joinable()) {
        thread_.join();
        return true;
    }
    Logger::info("HAL already stopped!");
    return true;
}

std::string_view HAL::name() const {
    return "HAL";
}

void HAL::loop() {
    // listen to sensors for events to publish
    // listen to bus to route controls
}