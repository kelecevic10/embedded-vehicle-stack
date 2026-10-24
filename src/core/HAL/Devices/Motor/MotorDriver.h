//
// Created by nemanja on 5/20/26.
//

#ifndef EMBEDDED_VEHICLE_STACK_MOTORDRIVER_H
#define EMBEDDED_VEHICLE_STACK_MOTORDRIVER_H

#include "core/HAL/Pins/Pins.h"
#include "core/HAL/Devices/Motor/MotorTypes.h"

class MotorDriver {
public:
    MotorDriver(
        GpioManager& gpioManager,
        PCADriver& pcaDriver,
        OutputPinConfig rEnableConf,
        OutputPinConfig lEnableConf,
        PwmPinConfig rPwmConf,
        PwmPinConfig lPwmConf
    );

    ~MotorDriver();

    bool init();
    bool shutdown();

    void setDirection(MotorDirection direction);
    void setDutyCycle(float dutyCycle); // 0.0 - 1.0

    bool drive(MotorDirection direction, float dutyCycle);
    bool drive(); // applies currently stored direction_ and dutyCycle_

    bool stop();

    [[nodiscard]] bool isInitialized() const;
    [[nodiscard]] MotorDirection direction() const;
    [[nodiscard]] float dutyCycle() const;

private:
    bool setPinsLow();
    bool applyDrive();

private:
    DigitalOutputPin rEnablePin_;
    DigitalOutputPin lEnablePin_;
    PwmPin rPwmPin_;
    PwmPin lPwmPin_;

    MotorDirection direction_ {MotorDirection::FORWARD};
    float dutyCycle_ {0.0f};
    bool initialized_ {false};
};

#endif // EMBEDDED_VEHICLE_STACK_MOTORDRIVER_H