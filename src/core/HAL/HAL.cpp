//
// Created by nemanja on 4/7/26.
//

#include "HAL.h"

#include <csignal>

#include "Devices/LED/LED.h"

bool HAL::registerDevices() {
    using namespace hal;

    OutputPinConfig config_led_ok {
        .pin = 17,
        .consumer = "LED_OK",
        .activeLow = false,
        .initialValue = PinState::LOW
    };

    greenLed = createDevice<LED>(
        config_led_ok.consumer,
        *gpioManager_,
        std::move(config_led_ok)
        );

    if (!greenLed) return false;

    OutputPinConfig config_led_error {
        .pin = 5,
        .consumer = "LED_ERROR",
        .activeLow = false,
        .initialValue = PinState::LOW
    };

    redLed = createDevice<LED>(
        config_led_error.consumer,
        *gpioManager_,
        std::move(config_led_error)
        );

    if (!redLed) return false;

    OutputPinConfig config_led_warning {
        .pin = 6,
        .consumer = "LED_WARNING",
        .activeLow = false,
        .initialValue = PinState::LOW
    };

    yellowLed = createDevice<LED>(
        config_led_warning.consumer,
        *gpioManager_,
        std::move(config_led_warning)
        );

    if (!yellowLed) return false;

    return true;
}

bool HAL::initDevices() {
    for (auto& device : devices_) {
        if (!device->init()) {
            Logger::error(std::format("Could not initialize device {}", device->name()));
            return false;
        }
    }
    Logger::info("All devices initialized successfully!");
    return true;
}

bool HAL::shutdownDevices() {
    for (auto& device : devices_) {
        if (!device->shutdown()) {
            Logger::error(std::format("Could not shutdown device {}", device->name()));
            return false;
        }
    }

    Logger::info("All devices shutdown successfully!");
    return true;
}

HAL::HAL(MessageBus* msg, std::string chip_path)
    : EventProcessor(msg), running_(false)
{
    try {
        gpioManager_ = std::make_unique<GpioManager>(std::move(chip_path));
    } catch (std::system_error& e) {
        Logger::error(std::format("Could not open the system chip: {}", e.what()));
        throw;
    };
}



bool HAL::start() {
    if (thread_.joinable()) {
        Logger::error("HAL already started!");
        return false;
    }

    if (devices_.empty()) {
        if (!registerDevices()) {
            Logger::error("Could not register devices!");
            return false;
        }
    }

    if (!initDevices()) {
        Logger::error("HAL: Could not initialize devices!");
        return false;
    }

    if (handlers_.empty())
        registerHandlers();

    bus_->subscribe(EventType::DIAGNOSTICS_UPDATE, this);
    startEventLoop();

    running_.store(true);
    thread_ = std::jthread(&HAL::loop, this);


    return true;
}

bool HAL::stop() {
    if (!thread_.joinable()) {
        Logger::info("HAL already stopped!");
        return true;
    }

    // stopping "main" thread that listen to sensors
    running_.store(false);
    thread_.join();

    // stopping EventListener thread that listens to system
    bus_->unsubscribe(EventType::DIAGNOSTICS_UPDATE, this);
    stopEventLoop();

    if (!shutdownDevices()) {
        Logger::error("HAL: Could not shutdown devices!");
        return false;
    }

    if (activeDiagLed) {
        activeDiagLed->turnOff();
        activeDiagLed = nullptr;
    }

    return true;
}

HAL::~HAL() {
    if (running_.load()) {
        HAL::stop();
    }
    if (activeDiagLed != nullptr)
        activeDiagLed->turnOff();
}


std::string_view HAL::name() const {
    return "HAL";
}


void HAL::loop() {
    size_t counter = 0;
    while (running_.load()) {
        // todo: implement sensor reading logic here
        std::this_thread::sleep_for(std::chrono::seconds(10));
        Logger::info(std::format("HAL loop iteration {}", counter++));
    }
}

void HAL::registerHandlers() {
    subscribe(EventType::DIAGNOSTICS_UPDATE, [this](const Event& e) {
        const SnapShot snapshot = std::get<SnapShot>(e.payload);
        switch (snapshot.diag_health) {
            case DiagHealth::OK:
                if (activeDiagLed != greenLed) {
                    if (activeDiagLed)
                        activeDiagLed->turnOff();
                    if (greenLed->turnOn())
                        activeDiagLed = greenLed;
                }
                break;
            case DiagHealth::WARNING:
                if (activeDiagLed != yellowLed) {
                    if (activeDiagLed)
                        activeDiagLed->turnOff();
                    if (yellowLed->turnOn())
                        activeDiagLed = yellowLed;
                }
                break;
            case DiagHealth::ERROR:
                if (activeDiagLed != redLed) {
                    if (activeDiagLed)
                        activeDiagLed->turnOff();
                    if (redLed->turnOn())
                        activeDiagLed = redLed;
                }
                break;
            default:
                break;
        }
    });
}