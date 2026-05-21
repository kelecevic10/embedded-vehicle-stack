//
// Created by nemanja on 5/21/26.
//

#include "Motor.h"

Motor::Motor(std::string name, GpioManager &gpioManager, MotorConfig config)
    : Device(std::move(name))
{

    auto rEnableConf = config.rEnable;
    auto lEnableConf = config.lEnable;
    auto rPwmConf = config.rPwm;
    auto lPwmConf = config.lPwm;

    driver_ = std::make_unique<MotorDriver>(
        gpioManager,
        rEnableConf,
        lEnableConf,
        rPwmConf,
        lPwmConf
    );
}

Motor::~Motor() {
    Motor::shutdown();
}

bool Motor::init() {
    if (initialized_) {
        Logger::warning(std::format("Motor {} already initialized", name()));
        return true;
    }

    if (!driver_->init()) {
        Logger::error(std::format("Failed to initialize Motor {}", name()));
        state_ = MotorState::Faulted;
        return false;
    }

    initialized_ = true;
    state_ = MotorState::Stopped;

    return true;
}

bool Motor::shutdown() {
    if (!initialized_) {
        Logger::warning(std::format("Motor {} already shutdown", name()));
        return true;
    }

    if (!driver_->shutdown()) {
        Logger::error(std::format("Failed to shutdown Motor {}", name()));
        state_ = MotorState::Faulted;
        return false;
    }

    initialized_ = false;
    state_ = MotorState::Uninitialized;

    return true;
}

void Motor::drive(MotorDirection direction, float dutyCycle) {
    if (!isInitialized()) {
        Logger::error(std::format("Motor {} is not initialized - drive ignored", name()));
        return;
    }

    if (state_ == MotorState::Uninitialized || state_ == MotorState::Faulted) {
        Logger::error(std::format("Motor {} is uninitialized or  in faulted state - cannot be driven", name()));
        state_ = MotorState::Faulted;
        return;
    }

    driver_->drive(direction, dutyCycle);

    if (dutyCycle <= 0.0f) {
        state_ = MotorState::Stopped;
        return;
    }

    state_ =
        direction == MotorDirection::FORWARD ?
        MotorState::RunningForward :
        MotorState::RunningBackward;
}

void Motor::stop() {
    if (!initialized_) {
        Logger::warning(std::format("Motor {} is not initialized - stop ignored", name()));
        return;
    }
    driver_->stop();
    state_ = MotorState::Stopped;
}

MotorDirection Motor::direction() const {
    return driver_->direction();
}

float Motor::dutyCycle() const {
    return driver_->dutyCycle();
}

bool Motor::isRunning() const {
    return state_ == MotorState::RunningForward || state_ == MotorState::RunningBackward;
}
