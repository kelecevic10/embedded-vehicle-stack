//
// Created by nemanja on 4/7/26.
//

#include "HAL.h"

#include <csignal>

#include "Devices/LED/LED.h"

bool HAL::registerDevices() {
    using namespace hal;

    i2cBus_ = std::make_unique<I2CBus>("/dev/i2c-1");
    if (!i2cBus_->openBus()) {
        Logger::error("HAL: Could not open I2C bus!");
        return false;
    }

    OutputPinConfig config_led_ok {
        .pin = 4,
        .consumer = "LED_OK",
        .activeLow = false,
        .initialValue = PinState::LOW
    };

    greenLed_ = createDevice<LED>(
        config_led_ok.consumer,
        *gpioManager_,
        std::move(config_led_ok)
    );

    if (!greenLed_) return false;

    OutputPinConfig config_led_error {
        .pin = 5,
        .consumer = "LED_ERROR",
        .activeLow = false,
        .initialValue = PinState::LOW
    };

    redLed_ = createDevice<LED>(
        config_led_error.consumer,
        *gpioManager_,
        std::move(config_led_error)
    );

    if (!redLed_) return false;

    OutputPinConfig config_led_warning {
        .pin = 6,
        .consumer = "LED_WARNING",
        .activeLow = false,
        .initialValue = PinState::LOW
    };

    yellowLed_ = createDevice<LED>(
        config_led_warning.consumer,
        *gpioManager_,
        std::move(config_led_warning)
    );

    if (!yellowLed_) return false;

    I2CDevice pcaDevice(*i2cBus_, 0x40);
    pcaDriver_ = std::make_unique<PCADriver>(std::move(pcaDevice));

    if (!pcaDriver_->init()) {
        Logger::error("HAL: Could not initialize PCA driver!");
        return false;
    }

    if (!pcaDriver_->setPwmFrequency(100.0f)) {
        Logger::error("HAL: Could not set PCA PWM frequency!");
        return false;
    }

    OutputPinConfig rEnableConf {
        .pin = 17,
        .consumer = "MOTOR_R_EN",
        .activeLow = false,
        .initialValue = PinState::LOW
    };

    OutputPinConfig lEnableConf {
        .pin = 27,
        .consumer = "MOTOR_L_EN",
        .activeLow = false,
        .initialValue = PinState::LOW
    };

    PwmPinConfig rPwmConf {
        .name = "LEFT_RPWM",
        .channel = 0,
        .initialDutyCycle = 0.0f
    };

    PwmPinConfig lPwmConf {
        .name = "LEFT_LPWM",
        .channel = 1,
        .initialDutyCycle = 0.0f
    };

    auto leftMotorConf = MotorConfig {
        .rEnable = std::move(rEnableConf),
        .lEnable = std::move(lEnableConf),
        .rPwm = std::move(rPwmConf),
        .lPwm = std::move(lPwmConf)
    };

    leftMotor_ = createDevice<Motor>(
        MotorId::LEFT,
        "L_MOTOR",
        *pcaDriver_,
        *gpioManager_,
        std::move(leftMotorConf)
    );

    if (!leftMotor_) return false;

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
    bus_->subscribe(EventType::MOTOR_DRIVE, this);
    bus_->subscribe(EventType::MOTOR_STOP, this);

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
    bus_->unsubscribe(EventType::MOTOR_DRIVE, this);
    bus_->unsubscribe(EventType::MOTOR_STOP, this);

    stopEventLoop();

    if (!shutdownDevices()) {
        Logger::error("HAL: Could not shutdown devices!");
        return false;
    }

    if (pcaDriver_) {
        if (!pcaDriver_->shutdown()) {
            Logger::error("HAL: Could not shutdown PCA driver!");
            return false;
        }
    }

    if (i2cBus_) {
        if (!i2cBus_->closeBus()) {
            Logger::error("HAL: Could not close I2C bus!");
            return false;
        }
    }

    if (activeDiagLed_) {
        activeDiagLed_->turnOff();
        activeDiagLed_ = nullptr;
    }

    return true;
}

HAL::~HAL() {
    if (running_.load()) {
        HAL::stop();
    }
    if (activeDiagLed_ != nullptr)
        activeDiagLed_->turnOff();
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
                if (activeDiagLed_ != greenLed_) {
                    if (activeDiagLed_)
                        activeDiagLed_->turnOff();
                    if (greenLed_->turnOn())
                        activeDiagLed_ = greenLed_;
                }
                break;
            case DiagHealth::WARNING:
                if (activeDiagLed_ != yellowLed_) {
                    if (activeDiagLed_)
                        activeDiagLed_->turnOff();
                    if (yellowLed_->turnOn())
                        activeDiagLed_ = yellowLed_;
                }
                break;
            case DiagHealth::ERROR:
                if (activeDiagLed_ != redLed_) {
                    if (activeDiagLed_)
                        activeDiagLed_->turnOff();
                    if (redLed_->turnOn())
                        activeDiagLed_ = redLed_;
                }
                break;
            default:
                break;
        }
    });

    subscribe(EventType::MOTOR_DRIVE, [this](const Event& e) {
        const auto* payload = std::get_if<MotorCommandPayload>(&e.payload);
        if (!payload) {
            Logger::error("Could not get motor command payload!");
            return;
        }

        switch (payload->motorId) {
            case MotorId::LEFT:
                if (leftMotor_) leftMotor_->drive(payload->direction, payload->dutyCycle);
                break;
            case MotorId::RIGHT:
                // if (rightMotor_) rightMotor_->drive(payload->direction, payload->dutyCycle);
                break;
            case MotorId::BOTH:
                if (leftMotor_) leftMotor_->drive(payload->direction, payload->dutyCycle);
                // if (rightMotor_) rightMotor_->drive(payload->direction, payload->dutyCycle);
                break;
        }
    });

    subscribe(EventType::MOTOR_STOP, [this](const Event& e) {
        const auto* payload = std::get_if<MotorId>(&e.payload);
        if (!payload) {
            Logger::error("HAL: MOTOR_STOP event has invalid MotorId payload");
            return;
        }

        switch (*payload) {
            case MotorId::LEFT:
                if (leftMotor_) leftMotor_->stop();
                break;
            case MotorId::RIGHT:
                // if (rightMotor_) rightMotor_->stop();
                break;
            case MotorId::BOTH:
                if (leftMotor_) leftMotor_->stop();
                // if (rightMotor_) rightMotor_->stop();
                break;
            default:
                Logger::error("HAL: unknown MotorId in MOTOR_STOP event");
                break;
        }
    });
}