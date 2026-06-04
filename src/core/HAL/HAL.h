//
// Created by nemanja on 4/7/26.
//

#ifndef EMBEDDED_VEHICLE_STACK_HAL_H
#define EMBEDDED_VEHICLE_STACK_HAL_H
#include <thread>

#include "Pins/GpioManager.h"
#include "core/MessageBus/MessageBus.h"
#include "core/System/Subsystem.h"
#include "Devices/Device.h"
#include "Devices/LED/LED.h"
#include "Devices/Motor/Motor.h"

using hal::GpioManager;

class HAL : public Subsystem, public EventProcessor {
public:

    HAL(MessageBus* msg, std::string chip_path);
    ~HAL() override;

    bool start() override;
    bool stop() override;
    std::string_view name() const override;

    template <typename T, typename... Args>
    T* createDevice(Args&&... args) {
        static_assert(std::is_base_of_v<Device, T>, "T must derive from Device");
        try {
            auto device = std::make_unique<T>(std::forward<Args>(args)...);
            T* rawPtr = device.get();
            devices_.push_back(std::move(device));
            return rawPtr;
        } catch (std::exception& e) {
            Logger::error(std::format("Could not create device: {}", e.what()));
            return nullptr;
        }
    }

    void registerHandlers() override;

    // TODO: router logic (public & listen to events)
private:
    std::jthread thread_;
    std::atomic<bool> running_;
    std::vector<std::unique_ptr<Device>> devices_;

    bool registerDevices();
    bool initDevices();
    bool shutdownDevices();

    std::unique_ptr<GpioManager> gpioManager_;
    std::unique_ptr<I2CBus> i2cBus_;
    std::unique_ptr<PCADriver> pcaDriver_;

    LED* greenLed_ = nullptr;
    LED* redLed_ = nullptr;
    LED* yellowLed_ = nullptr;
    LED* activeDiagLed_ = nullptr;
    Motor* leftMotor_ = nullptr;

    void loop();
};



#endif //EMBEDDED_VEHICLE_STACK_HAL_H
