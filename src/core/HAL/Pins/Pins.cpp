//
// Created by nemanja on 5/14/26.
//

#include "Pins.h"

#include "utils/logger/Logger.h"

DigitalOutputPin::DigitalOutputPin(GpioManager &gpioManager, OutputPinConfig config)
    : gpioManager_(gpioManager), config_(std::move(config)) { }

DigitalOutputPin::~DigitalOutputPin() {
    if (!shutdown()) {
        Logger::error(std::format("Failed to shutdown pin for {}", name()));
    }
}

bool DigitalOutputPin::init() {
    if (isInitialized()) {
        return true;
    }

    if (gpioManager_.claimPin(config_)) {
        initialized_ = true;
        return true;
    }
    return false;
}

bool DigitalOutputPin::shutdown() {
    if (!isInitialized()) {
        return true;
    }

    try {
        if (!write(PinState::LOW)) {
            Logger::error("Failed to set pin to safe state before releasing.");
        }
        gpioManager_.releasePin(config_.pin);
        initialized_ = false;
    } catch (std::exception& e) {
        Logger::error(std::format("Failed to release pin: {}", e.what()));
        return false;
    }
    return true;
}

bool DigitalOutputPin::write(const PinState state) const {
    if (!isInitialized()) {
        Logger::error("Pin is not initialized, thus cannot be written to");
        return false;
    }
    return gpioManager_.write(config_.pin, state);
}

bool DigitalOutputPin::setHigh() {
    return write(PinState::HIGH);
}

bool DigitalOutputPin::setLow() {
    return write(PinState::LOW);
}

unsigned int DigitalOutputPin::pin() const {
    return config_.pin;
}

const std::string& DigitalOutputPin::name() const {
    return config_.consumer;
}

bool DigitalOutputPin::isInitialized() const {
    return initialized_;
}

DigitalInputPin::DigitalInputPin(GpioManager &gpioManager, InputPinConfig config)
    : gpioManager_(gpioManager), config_(std::move(config)) { }

DigitalInputPin::~DigitalInputPin() {
    if (!shutdown()) {
        Logger::error(std::format("Failed to shutdown pin for {}", name()));
    }
}

bool DigitalInputPin::init() {
    if (isInitialized()) {
        return true;
    }

    if (gpioManager_.claimPin(config_)) {
        initialized_ = true;
        return true;
    }
    return false;
}

bool DigitalInputPin::shutdown() {
    if (!isInitialized()) {
        return true;
    }
    try {
        gpioManager_.releasePin(config_.pin);
        initialized_ = false;
    } catch (std::exception& e) {
        Logger::error(std::format("Failed to release pin: {}", e.what()));
        return false;
    }

    return true;
}

std::optional<PinState> DigitalInputPin::read() const {
    if (!isInitialized()) {
        Logger::error("Pin is not initialized, thus cannot be read");
        return std::nullopt;
    }

    const auto res = gpioManager_.read(config_.pin);
    if (res.has_value()) {
        return res.value();
    }
    return std::nullopt;
}


bool DigitalInputPin::isHigh() const {
    const auto res = read();
    if (res.has_value())
        return res.value() == PinState::HIGH;

    Logger::error("Failed to read pin");
    return false;
}

bool DigitalInputPin::isLow() const {
    const auto res = read();
    if (res.has_value())
        return res.value() == PinState::LOW;

    Logger::error("Failed to read pin");
    return false;
}

unsigned int DigitalInputPin::pin() const {
    return config_.pin;
}

const std::string & DigitalInputPin::name() const {
    return config_.consumer;
}

bool DigitalInputPin::isInitialized() const {
    return initialized_;
}

