//
// Created by nemanja on 4/8/26.
//

#ifndef EMBEDDED_VEHICLE_STACK_LED_H
#define EMBEDDED_VEHICLE_STACK_LED_H
#include "core/HAL/Devices/Device.h"
#include <gpiod.hpp>

#include "utils/logger/Logger.h"

class LED : public Actuator {
public:
    LED(
        const unsigned int pin,
        std::string name,
        hal::GpioManager* gpioManager,
        hal::OutputPinConfig config
        )
        : Actuator(pin, std::move(name), gpioManager, std::move(config))
    {

    };

    ~LED() override = default;

    bool turnOn() const;
    bool turnOff() const;

private:
    // it has DeviceHealth health_ inherited
};



#endif //EMBEDDED_VEHICLE_STACK_LED_H
