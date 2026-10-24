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

PwmPin::PwmPin(PCADriver& pcaDriver, PwmPinConfig config)
    : pcaDriver_(pcaDriver), config_(std::move(config))
{
}

bool PwmPin::init() {
    if (initialized_) {
        Logger::warning(std::format(
            "PwmPin {} already initialized",
                config_.name
        ));
        return true;
    }

    if (!pcaDriver_.isInitialized()) {
        Logger::error(std::format(
            "Failed to initialize PwmPin {} BECAUSE PCA driver is not initialized",
            config_.name
        ));
        return false;
    }

    const float clampedDutyCycle = std::clamp(config_.initialDutyCycle, 0.0f, 1.0f);
    if (clampedDutyCycle == 0.0f) {
        if (!pcaDriver_.forceLow(config_.channel)) {
            Logger::error(std::format(
                "Failed to force low on PwmPin {} BECAUSE PCA driver failed to force low",
                config_.name
            ));
            return false;
        }
    } else {
        if (!pcaDriver_.setDutyCycle(config_.channel, clampedDutyCycle)) {
            Logger::error(std::format(
                "Failed to set duty cycle on PwmPin {} BECAUSE PCA driver failed to set duty cycle",
                config_.name
            ));
            return false;
        }
    }

    dutyCycle_ = clampedDutyCycle;
    initialized_ = true;

    Logger::info(std::format(
    "PwmPin {}: initialized on PCA channel {} with dutyCycle {:.2f}",
    config_.name,
    config_.channel,
    dutyCycle_
    ));

    return true;
}

bool PwmPin::shutdown() {
    if (!initialized_) {
        return true;
    }

    const bool ok = pcaDriver_.forceLow(config_.channel);

    dutyCycle_ = 0.0f;
    initialized_ = false;

    if (!ok) {
        Logger::error(std::format(
            "Failed to shutdown PwmPin {} BECAUSE PCA driver failed to force low",
            config_.name
        ));
        return false;
    }

    Logger::info(std::format(
        "PwmPin {}: shutdown on PCA channel {}",
        config_.name, config_.channel
     ));

    return true;
}

bool PwmPin::setDutyCycle(float dutyCycle) {
    if (!initialized_) {
        Logger::warning(std::format(
            "PwmPin {}: setDutyCycle called before initialization",
            config_.name
        ));
        return false;
    }

    const float clampedDutyCycle = std::clamp(dutyCycle, 0.0f, 1.0f);

    if (!pcaDriver_.setDutyCycle(config_.channel, clampedDutyCycle)) {
        Logger::error(std::format(
            "PwmPin {}: failed to set dutyCycle {:.2f}",
            config_.name,
            clampedDutyCycle
        ));
        return false;
    }

    dutyCycle_ = clampedDutyCycle;
    return true;
}

bool PwmPin::forceLow() {
    if (!initialized_) {
        Logger::warning(std::format(
            "PwmPin {}: forceLow called before initialization",
            config_.name
        ));
        return false;
    }

    if (!pcaDriver_.forceLow(config_.channel)) {
        Logger::error(std::format(
            "PwmPin {}: failed to force LOW",
            config_.name
        ));
        return false;
    }

    dutyCycle_ = 0.0f;
    return true;
}

bool PwmPin::forceHigh() {
    if (!initialized_) {
        Logger::warning(std::format(
            "PwmPin {}: forceHigh called before initialization",
            config_.name
        ));
        return false;
    }

    if (!pcaDriver_.forceHigh(config_.channel)) {
        Logger::error(std::format(
            "PwmPin {}: failed to force HIGH",
            config_.name
        ));
        return false;
    }

    dutyCycle_ = 1.0f;
    return true;
}

bool PwmPin::isInitialized() const {
    return initialized_;
}

float PwmPin::dutyCycle() const {
    return dutyCycle_;
}

std::uint8_t PwmPin::channel() const {
    return config_.channel;
}

const std::string& PwmPin::name() const {
    return config_.name;
}