//
// Created by nemanja on 5/20/26.
//

#include "MotorDriver.h"

#include <algorithm>
#include <utility>

#include "utils/logger/Logger.h"

MotorDriver::MotorDriver(
    GpioManager& gpioManager,
    PCADriver& pcaDriver,
    OutputPinConfig rEnableConf,
    OutputPinConfig lEnableConf,
    PwmPinConfig rPwmConf,
    PwmPinConfig lPwmConf
) :
    rEnablePin_(gpioManager, std::move(rEnableConf)),
    lEnablePin_(gpioManager, std::move(lEnableConf)),
    rPwmPin_(pcaDriver, std::move(rPwmConf)),
    lPwmPin_(pcaDriver, std::move(lPwmConf))
{ }

MotorDriver::~MotorDriver() {
    shutdown();
}

bool MotorDriver::init() {
    if (initialized_) {
        Logger::warning("MotorDriver: already initialized");
        return true;
    }

    if (!rEnablePin_.init()) {
        Logger::error("MotorDriver: could not initialize motor r_enable pin");
        shutdown();
        return false;
    }

    if (!lEnablePin_.init()) {
        Logger::error("MotorDriver: could not initialize motor l_enable pin");
        shutdown();
        return false;
    }

    if (!rPwmPin_.init()) {
        Logger::error("MotorDriver: could not initialize motor r_pwm pin");
        shutdown();
        return false;
    }

    if (!lPwmPin_.init()) {
        Logger::error("MotorDriver: could not initialize motor l_pwm pin");
        shutdown();
        return false;
    }

    if (!setPinsLow()) {
        Logger::error("MotorDriver: could not set motor pins to safe LOW state during initialization");
        shutdown();
        return false;
    }

    direction_ = MotorDirection::FORWARD;
    dutyCycle_ = 0.0f;
    initialized_ = true;

    return true;
}

bool MotorDriver::shutdown() {
    // Important: shutdown must also work after partial init failure.
    // Therefore, it must not simply return when initialized_ == false.

    bool safeLow = setPinsLow();

    bool validPinShutDown = true;

    if (rEnablePin_.isInitialized()) {
        validPinShutDown &= rEnablePin_.shutdown();
    }

    if (lEnablePin_.isInitialized()) {
        validPinShutDown &= lEnablePin_.shutdown();
    }

    if (rPwmPin_.isInitialized()) {
        validPinShutDown &= rPwmPin_.shutdown();
    }

    if (lPwmPin_.isInitialized()) {
        validPinShutDown &= lPwmPin_.shutdown();
    }

    initialized_ = false;
    dutyCycle_ = 0.0f;

    if (!validPinShutDown) {
        Logger::error("MotorDriver: one or more motor pins failed to shutdown");
    }

    if (!safeLow) {
        Logger::error("MotorDriver: failed to force pins LOW during shutdown");
    }

    return validPinShutDown && safeLow;
}

void MotorDriver::setDirection(const MotorDirection direction) {
    direction_ = direction;
}

void MotorDriver::setDutyCycle(const float dutyCycle) {
    const float clampedDutyCycle = std::clamp(dutyCycle, 0.0f, 1.0f);

    if (clampedDutyCycle != dutyCycle) {
        Logger::warning("MotorDriver: duty cycle out of range, value clamped");
    }

    dutyCycle_ = clampedDutyCycle;
}

bool MotorDriver::drive(const MotorDirection direction, const float dutyCycle) {
    setDirection(direction);
    setDutyCycle(dutyCycle);
    return drive();
}

bool MotorDriver::drive() {
    if (!initialized_) {
        Logger::warning("MotorDriver: drive() called before initialization");
        return false;
    }

    if (dutyCycle_ <= 0.0f) {
        return stop();
    }

    if (!applyDrive()) {
        Logger::error("MotorDriver: could not apply drive command");

        if (!stop()) {
            Logger::error("MotorDriver: failed to restore safe state after drive failure.");
        }

        return false;
    }

    return true;
}

bool MotorDriver::stop() {
    if (!initialized_) {
        Logger::warning("MotorDriver: stop() called before initialization");
        return true;
    }

    if (!setPinsLow()) {
        Logger::error("MotorDriver: could not stop motor safely");
        return false;
    }

    setDutyCycle(0.0f);
    return true;
}

bool MotorDriver::isInitialized() const {
    return initialized_;
}

MotorDirection MotorDriver::direction() const {
    return direction_;
}

float MotorDriver::dutyCycle() const {
    return dutyCycle_;
}

bool MotorDriver::setPinsLow() {
    bool valid = true;

    // Safer order: first remove drive/PWM signals, then disable the driver.
    if (rPwmPin_.isInitialized()) {
        valid &= rPwmPin_.forceLow();
    }

    if (lPwmPin_.isInitialized()) {
        valid &= lPwmPin_.forceLow();
    }

    if (rEnablePin_.isInitialized()) {
        valid &= rEnablePin_.setLow();
    }

    if (lEnablePin_.isInitialized()) {
        valid &= lEnablePin_.setLow();
    }

    return valid;
}

bool MotorDriver::applyDrive() {

    // Safe transition: first remove both direction/PWM signals.
    if (!rPwmPin_.forceLow() || !lPwmPin_.forceLow()) {
        Logger::error("MotorDriver: could not clear PWM pins before direction change");
        return false;
    }

    if (!rEnablePin_.setHigh() || !lEnablePin_.setHigh()) {
        Logger::error("MotorDriver: could not enable BTS motor driver");
        return false;
    }

    if (direction_ == MotorDirection::FORWARD) {
        return rPwmPin_.setDutyCycle(dutyCycle_) && lPwmPin_.forceLow();
    }

    return rPwmPin_.forceLow() && lPwmPin_.setDutyCycle(dutyCycle_);
}