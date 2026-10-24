//
// Created by nemanja on 4/7/26.
//

#ifndef EMBEDDED_VEHICLE_STACK_DEVICE_H
#define EMBEDDED_VEHICLE_STACK_DEVICE_H
#include <string>

#include "core/MessageBus/MessageBus.h"
#include "../Pins/GpioManager.h"
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
    explicit Device(std::string name): name_(std::move(name)) { };
    virtual ~Device() = default;

    virtual bool init() = 0;
    virtual bool shutdown() = 0;

    bool isInitialized() const {return initialized_;};
    std::string name() const {return name_;};
    DeviceHealth health() const {return health_;};

protected:
    std::string name_;
    DeviceHealth health_ {DeviceHealth::OK};
    bool initialized_ = false;

};












#endif //EMBEDDED_VEHICLE_STACK_DEVICE_H
