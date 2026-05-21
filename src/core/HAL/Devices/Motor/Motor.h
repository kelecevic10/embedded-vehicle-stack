//
// Created by nemanja on 5/21/26.
//

#ifndef EMBEDDED_VEHICLE_STACK_MOTOR_H
#define EMBEDDED_VEHICLE_STACK_MOTOR_H

#include "MotorDriver.h"
#include "core/HAL/Devices/Device.h"

struct MotorConfig {
    OutputPinConfig rEnable;
    OutputPinConfig lEnable;
    OutputPinConfig rPwm;
    OutputPinConfig lPwm;
};

enum class MotorState {
    Uninitialized,
    Stopped,
    RunningForward,
    RunningBackward,
    Faulted
};

class Motor : public Device {
public:
    explicit Motor(std::string name, GpioManager& gpioManager, MotorConfig config);
    ~Motor() override;

    bool init() override;
    bool shutdown() override;

    void drive(MotorDirection direction, float dutyCycle);
    void stop();

    [[nodiscard]] MotorDirection direction() const;
    [[nodiscard]] float dutyCycle() const;
    [[nodiscard]] bool isRunning() const;

private:
    std::unique_ptr<MotorDriver> driver_;
    MotorState state_ {MotorState::Uninitialized};

    // TODO: Update health_ wherever is needed
    // std::string name_;
    // DeviceHealth health_ {DeviceHealth::OK};
    // bool initialized_ = false;
};


#endif //EMBEDDED_VEHICLE_STACK_MOTOR_H
