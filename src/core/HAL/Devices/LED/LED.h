//
// Created by nemanja on 4/8/26.
//

#ifndef EMBEDDED_VEHICLE_STACK_LED_H
#define EMBEDDED_VEHICLE_STACK_LED_H
#include "core/HAL/Devices/Device.h"
#include "utils/logger/Logger.h"
#include "core/HAL/Pins/Pins.h"

using hal::GpioManager; using hal::OutputPinConfig;

class LED : public Device {
public:
    LED(std::string name, GpioManager& gpioManager, OutputPinConfig config)
        : Device(std::move(name)), pin_(gpioManager, std::move(config)) { };
    ~LED() override = default;

    bool init() override;
    bool shutdown() override;

    bool turnOn();
    bool turnOff();

private:
//  Inherited:
//  std::string name_;
//  DeviceHealth health_ {DeviceHealth::OK};
//  bool initialized_ = false;
    DigitalOutputPin pin_;
};



#endif //EMBEDDED_VEHICLE_STACK_LED_H
