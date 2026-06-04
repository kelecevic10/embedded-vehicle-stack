//
// Created by nemanja on 5/21/26.
//

#ifndef EMBEDDED_VEHICLE_STACK_MOTOR_H
#define EMBEDDED_VEHICLE_STACK_MOTOR_H

#include "MotorDriver.h"
#include "core/HAL/Devices/Device.h"
#include "core/HAL/Devices/Motor/MotorTypes.h"

class Motor : public Device {
public:
    explicit Motor(
        MotorId id,
        std::string name,
        PCADriver& pcaDriver,
        GpioManager& gpioManager,
        MotorConfig config
    );
    ~Motor() override;

    bool init() override;
    bool shutdown() override;

    bool drive(MotorDirection direction, float speed);
    bool stop();

    [[nodiscard]] MotorDirection direction() const;
    [[nodiscard]] float dutyCycle() const;
    [[nodiscard]] bool isRunning() const;
    [[nodiscard]] MotorId id() const;

private:
    std::unique_ptr<MotorDriver> driver_;
    MotorState state_ {MotorState::Uninitialized};
    MotorId id_;
    // Inherited:
    // std::string name_;
    // DeviceHealth health_ {DeviceHealth::OK};
    // bool initialized_ = false;
};


#endif //EMBEDDED_VEHICLE_STACK_MOTOR_H
