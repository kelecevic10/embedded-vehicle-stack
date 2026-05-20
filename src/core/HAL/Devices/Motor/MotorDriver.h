//
// Created by nemanja on 5/20/26.
//

#ifndef EMBEDDED_VEHICLE_STACK_MOTORDRIVER_H
#define EMBEDDED_VEHICLE_STACK_MOTORDRIVER_H

#include "core/HAL/Pins/Pins.h"

enum class MotorDirection {
    FORWARD,
    BACKWARD
};

class MotorDriver {
public:
    MotorDriver(
        GpioManager& gpioManager,
        OutputPinConfig rEnableConf,
        OutputPinConfig lEnableConf,
        OutputPinConfig rPwmConf,
        OutputPinConfig lPwmConf
    );

    ~MotorDriver();

    bool init();
    bool shutdown();

    void setDirection(MotorDirection direction);
    void setDutyCycle(float dutyCycle); // 0.0 - 1.0

    void drive(MotorDirection direction, float dutyCycle);
    void drive(); // applies currently stored direction_ and dutyCycle_

    void stop();

    [[nodiscard]] bool isInitialized() const;
    [[nodiscard]] MotorDirection direction() const;
    [[nodiscard]] float dutyCycle() const;

private:
    bool forcePinsLow();
    bool applyDigitalDrive();

private:
    DigitalOutputPin rEnablePin_;
    DigitalOutputPin lEnablePin_;
    DigitalOutputPin rPwmPin_;
    DigitalOutputPin lPwmPin_;


    MotorDirection direction_ {MotorDirection::FORWARD};
    float dutyCycle_ {0.0f};
    bool initialized_ {false};
};

#endif // EMBEDDED_VEHICLE_STACK_MOTORDRIVER_H