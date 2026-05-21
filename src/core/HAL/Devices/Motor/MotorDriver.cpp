//
// Created by nemanja on 5/20/26.
//

#include "MotorDriver.h"

#include <algorithm>
#include <utility>

#include "utils/logger/Logger.h"

MotorDriver::MotorDriver(
    GpioManager& gpioManager,
    OutputPinConfig rEnableConf,
    OutputPinConfig lEnableConf,
    OutputPinConfig rPwmConf,
    OutputPinConfig lPwmConf
) :
    rEnablePin_(gpioManager, std::move(rEnableConf)),
    lEnablePin_(gpioManager, std::move(lEnableConf)),
    rPwmPin_(gpioManager, std::move(rPwmConf)),
    lPwmPin_(gpioManager, std::move(lPwmConf))
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

    if (rPwmPin_.isInitialized()) {
        rPwmPin_.setLow();
    }

    if (lPwmPin_.isInitialized()) {
        lPwmPin_.setLow();
    }

    if (rEnablePin_.isInitialized()) {
        rEnablePin_.setLow();
    }

    if (lEnablePin_.isInitialized()) {
        lEnablePin_.setLow();
    }

    bool valid = true;

    if (rEnablePin_.isInitialized()) {
        valid &= rEnablePin_.shutdown();
    }

    if (lEnablePin_.isInitialized()) {
        valid &= lEnablePin_.shutdown();
    }

    if (rPwmPin_.isInitialized()) {
        valid &= rPwmPin_.shutdown();
    }

    if (lPwmPin_.isInitialized()) {
        valid &= lPwmPin_.shutdown();
    }

    initialized_ = false;
    dutyCycle_ = 0.0f;

    if (!valid) {
        Logger::error("MotorDriver: one or more motor pins failed to shutdown");
    }

    return valid;
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

void MotorDriver::drive(const MotorDirection direction, const float dutyCycle) {
    setDirection(direction);
    setDutyCycle(dutyCycle);
    drive();
}

void MotorDriver::drive() {
    if (!initialized_) {
        Logger::warning("MotorDriver: drive() called before initialization");
        return;
    }

    if (dutyCycle_ <= 0.0f) {
        stop();
        return;
    }

    if (!applyDigitalDrive()) {
        Logger::error("MotorDriver: could not apply digital drive command");
        stop();
    }
}

void MotorDriver::stop() {
    if (!initialized_) {
        Logger::warning("MotorDriver: stop() called before initialization");
        return;
    }

    if (!setPinsLow()) {
        Logger::error("MotorDriver: could not stop motor safely");
    }

    setDutyCycle(0.0f);
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
        valid &= rPwmPin_.setLow();
    }

    if (lPwmPin_.isInitialized()) {
        valid &= lPwmPin_.setLow();
    }

    if (rEnablePin_.isInitialized()) {
        valid &= rEnablePin_.setLow();
    }

    if (lEnablePin_.isInitialized()) {
        valid &= lEnablePin_.setLow();
    }

    return valid;
}

bool MotorDriver::applyDigitalDrive() {
    // Temporary digital implementation:
    // any dutyCycle_ > 0 means full-power drive.
    // Real PWM will later replace this behavior.

    // Safe transition: first remove both direction/PWM signals.
    if (!rPwmPin_.setLow() || !lPwmPin_.setLow()) {
        Logger::error("MotorDriver: could not clear PWM pins before direction change");
        return false;
    }

    if (!rEnablePin_.setHigh() || !lEnablePin_.setHigh()) {
        Logger::error("MotorDriver: could not enable BTS motor driver");
        return false;
    }

    if (direction_ == MotorDirection::FORWARD) {
        return rPwmPin_.setHigh() && lPwmPin_.setLow();
    }

    return rPwmPin_.setLow() && lPwmPin_.setHigh();
}