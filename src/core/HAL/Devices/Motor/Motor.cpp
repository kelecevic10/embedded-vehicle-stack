//
// Created by nemanja on 5/21/26.
//

#include "Motor.h"

Motor::Motor(const MotorId id, std::string name, GpioManager &gpioManager, MotorConfig config)
    : Device(std::move(name)), id_(id)
{

    driver_ = std::make_unique<MotorDriver>(
        gpioManager,
        std::move(config.rEnable),
        std::move(config.lEnable),
        std::move(config.rPwm),
        std::move(config.lPwm)
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
        health_ = DeviceHealth::ERROR;
        return false;
    }

    initialized_ = true;
    state_ = MotorState::Stopped;
    health_ = DeviceHealth::OK;

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
        health_ = DeviceHealth::ERROR;
        return false;
    }

    initialized_ = false;
    state_ = MotorState::Uninitialized;
    health_ = DeviceHealth::OK;

    return true;
}

void Motor::drive(const MotorDirection direction, const float speed) {
    if (!isInitialized()) {
        Logger::error(std::format("Motor {} is not initialized - drive ignored", name()));
        return;
    }

    if (state_ == MotorState::Faulted) {
        Logger::error(std::format("Motor {} is in faulted state - cannot be driven", name()));
        health_ = DeviceHealth::ERROR;
        return;
    }

    driver_->drive(direction, speed);

    if (driver_->dutyCycle() <= 0.0f) {
        state_ = MotorState::Stopped;
        return;
    }

    state_ =
        direction == MotorDirection::FORWARD ?
        MotorState::RunningForward :
        MotorState::RunningBackward;

    health_ = DeviceHealth::OK;
}

void Motor::stop() {
    if (!initialized_) {
        Logger::warning(std::format("Motor {} is not initialized - stop ignored", name()));
        return;
    }

    driver_->stop();

    if (state_ == MotorState::Faulted) {
        state_ = MotorState::Stopped;
        health_ = DeviceHealth::OK;
    }
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

MotorId Motor::id() const {
    return id_;
}
