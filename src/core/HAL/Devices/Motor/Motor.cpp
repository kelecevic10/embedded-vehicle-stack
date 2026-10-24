//
// Created by nemanja on 5/21/26.
//

#include "Motor.h"

Motor::Motor(
    const MotorId id,
    std::string name,
    PCADriver& pcaDriver,
    GpioManager& gpioManager,
    MotorConfig config
    )
    : Device(std::move(name)), id_(id)
{

    driver_ = std::make_unique<MotorDriver>(
        gpioManager,
        pcaDriver,
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

    Logger::info(std::format("Motor {}: initializing", name()));
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

    Logger::info(std::format("Motor {}: initialized successfully", name()));

    return true;
}

bool Motor::shutdown() {
    if (!initialized_ && state_ == MotorState::Uninitialized) {
        return true;
    }

    Logger::info(std::format("Motor {}: shutting down", name()));

    const bool stopped = driver_->stop();
    const bool shutdownOk = driver_->shutdown();

    if (!stopped || !shutdownOk) {
        Logger::error(std::format(
            "Motor {}: failed to shutdown safely",
            name()
        ));

        state_ = MotorState::Faulted;
        health_ = DeviceHealth::ERROR;
        initialized_ = false;

        return false;
    }

    initialized_ = false;
    state_ = MotorState::Uninitialized;
    health_ = DeviceHealth::OK;

    Logger::info(std::format("Motor {}: shutdown complete", name()));
    return true;
}

bool Motor::drive(MotorDirection direction, float dutyCycle) {
    if (!initialized_) {
        Logger::warning(std::format(
            "Motor {}: drive ignored because motor is not initialized",
            name()
        ));
        return false;
    }

    if (state_ == MotorState::Faulted) {
        Logger::error(std::format(
            "Motor {}: drive ignored because motor is faulted",
            name()
        ));
        health_ = DeviceHealth::ERROR;
        return false;
    }

    Logger::info(std::format(
        "Motor {}: drive requested direction={}, dutyCycle={:.2f}",
        name(),
        toString(direction),
        dutyCycle
    ));

    const auto oldState = state_;

    if (!driver_->drive(direction, dutyCycle)) {
        Logger::error(std::format(
            "Motor {}: driver failed to execute drive command",
            name()
        ));

        driver_->stop();

        state_ = MotorState::Faulted;
        health_ = DeviceHealth::ERROR;

        Logger::error(std::format(
            "Motor {}: state {} -> {}",
            name(),
            toString(oldState),
            toString(state_)
        ));

        return false;
    }

    if (driver_->dutyCycle() <= 0.0f) {
        state_ = MotorState::Stopped;
    } else {
        state_ = direction == MotorDirection::FORWARD
            ? MotorState::RunningForward
            : MotorState::RunningBackward;
    }

    health_ = DeviceHealth::OK;

    Logger::info(std::format(
        "Motor {}: state {} -> {}",
        name(),
        toString(oldState),
        toString(state_)
    ));

    return true;
}

bool Motor::stop() {
    if (!initialized_) {
        Logger::warning(std::format(
            "Motor {}: stop ignored because motor is not initialized",
            name()
        ));
        return true;
    }

    Logger::info(std::format("Motor {}: stop requested", name()));

    const auto oldState = state_;

    if (!driver_->stop()) {
        Logger::error(std::format(
            "Motor {}: driver failed to stop motor safely",
            name()
        ));

        state_ = MotorState::Faulted;
        health_ = DeviceHealth::ERROR;

        Logger::error(std::format(
            "Motor {}: state {} -> {}",
            name(),
            toString(oldState),
            toString(state_)
        ));

        return false;
    }

    if (state_ != MotorState::Faulted) {
        state_ = MotorState::Stopped;
        health_ = DeviceHealth::OK;
    }

    Logger::info(std::format(
        "Motor {}: state {} -> {}",
        name(),
        toString(oldState),
        toString(state_)
    ));

    return true;
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
