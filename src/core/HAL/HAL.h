//
// Created by nemanja on 4/7/26.
//

#ifndef EMBEDDED_VEHICLE_STACK_HAL_H
#define EMBEDDED_VEHICLE_STACK_HAL_H
#include <thread>

#include "GpioManager.h"
#include "core/MessageBus/MessageBus.h"
#include "core/System/Subsystem.h"
#include "Devices/Device.h"

using hal::GpioManager;

class HAL : Subsystem {
public:

    HAL(MessageBus* msg, std::string chip_path);
    ~HAL() override;

    bool start() override;
    bool stop() override;
    std::string_view name() const override;

    // TODO: router logic (public & listen to events)
private:
    std::jthread thread_;
    MessageBus* messageBus_;
    std::unique_ptr<GpioManager> gpioManager_;
    std::vector<std::unique_ptr<Device>> devices_;

    bool registerDevices();
    void loop();
};



#endif //EMBEDDED_VEHICLE_STACK_HAL_H
