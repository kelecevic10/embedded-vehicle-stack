//
// Created by nemanja on 4/8/26.
//

#include "LED.h"

#include "utils/logger/Logger.h"

bool LED::init() {
    if (isInitialized()) {
        return true;
    }

    if (!pin_.init()) {
        Logger::error(std::format("Failed to initialize {} pin", name()));
        return false;
    }

    initialized_ = true;
    return true;
}

bool LED::shutdown() {
    if (!isInitialized()) {
        return true;
    }

    if (!turnOff()) {
        Logger::error(std::format("Failed to turn off {} while shutting down the Device", name()));
        return false;
    }

    if (!pin_.shutdown()) {
        Logger::error(std::format("Failed to shutdown {} LED pin", name()));
        return false;
    }
    initialized_ = false;
    return true;
}

bool LED::turnOn() {
    if (!isInitialized()) {
        Logger::error(std::format("{} is not initialized - cannot be turned on", name()));
        return false;
    }

    if (!pin_.setHigh()) {
        Logger::error(std::format("Failed to turn on {} LED", name()));
        return false;
    }
    return true;
}

bool LED::turnOff() {
    if (!isInitialized()) {
        Logger::error(std::format("{} is not initialized - cannot be turned off", name()));
        return false;
    }

    if (!pin_.setLow()) {
        Logger::error(std::format("Failed to turn off {}", name()));
        return false;
    }
    return true;
}
