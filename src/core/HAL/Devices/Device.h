//
// Created by nemanja on 4/7/26.
//

#ifndef EMBEDDED_VEHICLE_STACK_DEVICE_H
#define EMBEDDED_VEHICLE_STACK_DEVICE_H
#include <string>

#include "core/MessageBus/MessageBus.h"
#include "core/HAL/GpioManager.h"
#include "utils/logger/Logger.h"

enum class DeviceHealth {
    OK,
    WARNING,
    ERROR
};

using hal::GpioManager;
using hal::InputPinConfig;
using hal::OutputPinConfig;
using hal::PinState;

class Device {
public:
     Device(
         const unsigned int pin,
         std::string name,
         GpioManager* gpioManager
         )
        : pin_(pin), name_(std::move(name)), gpioManager_(gpioManager) { };

    virtual ~Device() = default;

    unsigned int pin() const {return pin_;}
    std::string name() const {return name_;};
    DeviceHealth health() const {return health_;};

protected:
    const unsigned int pin_;
    std::string name_;
    DeviceHealth health_ {DeviceHealth::OK};
    GpioManager* gpioManager_;
};

class Actuator : public Device {
public:
    Actuator(
        const unsigned int pin,
        std::string name,
        GpioManager* gpioManager,
        OutputPinConfig config
        )
        : Device(pin, std::move(name), gpioManager)
        , config_(std::move(config))
    {
        if (!claimPin()) {
            throw std::runtime_error(
                "Could not claim pin for Actuator: " + this->name() +
                ", pin: " + std::to_string(this->pin())
            );
        }
    };

    ~Actuator() override
    {
        if (gpioManager_ != nullptr)
            gpioManager_->releasePin(pin_);
    }

    bool claimPin() {
        if (gpioManager_ != nullptr)
            return gpioManager_->claimPin(config_);
        return false;
    };

    bool write(const PinState value_) const {
        return gpioManager_->write(pin_, value_);
    };
private:
    OutputPinConfig config_;
};

class Sensor : public Device {
public:
    Sensor(
        const unsigned int pin,
        std::string name,
        GpioManager* gpioManager,
        InputPinConfig config
        )
        : Device(pin, std::move(name), gpioManager)
        , config_(std::move(config))
    {

    };
    ~Sensor() override = default;

    // make a wrapper for future gpioManager interface that works with events
    // gpio function is not implemented yet
    // TODO:
    // In the future, this method should return a dedicated sensor-reading/result type
    // instead of a system Event. The idea is for the sensor layer to aggregate/filter
    // low-level library events into one meaningful reading, while HAL will translate
    // that reading into the project's Event model when needed.
    virtual const Event read() = 0;
private:
    InputPinConfig config_;
};









#endif //EMBEDDED_VEHICLE_STACK_DEVICE_H
