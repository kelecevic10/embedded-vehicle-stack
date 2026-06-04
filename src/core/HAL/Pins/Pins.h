//
// Created by nemanja on 5/14/26.
//

#ifndef EMBEDDED_VEHICLE_STACK_PINS_H
#define EMBEDDED_VEHICLE_STACK_PINS_H
#include "GpioManager.h"
#include "core/HAL/PWM/PCADriver.h"

using GpioManager = hal::GpioManager;
using OutputPinConfig = hal::OutputPinConfig;
using InputPinConfig = hal::InputPinConfig;
using PinState = hal::PinState;

class DigitalOutputPin {
public:
    DigitalOutputPin(GpioManager& gpioManager, OutputPinConfig config);
    ~DigitalOutputPin();

    DigitalOutputPin(const DigitalOutputPin&) = delete;
    DigitalOutputPin& operator=(const DigitalOutputPin&) = delete;
    DigitalOutputPin(DigitalOutputPin&&) = delete;
    DigitalOutputPin& operator=(DigitalOutputPin&&) = delete;

    bool init();
    bool shutdown();

    bool setHigh();
    bool setLow();

    unsigned int pin() const;
    const std::string& name() const;
    bool isInitialized() const;

private:
    GpioManager& gpioManager_;
    OutputPinConfig config_;
    bool initialized_ = false;

    // methods
    bool write(PinState state) const;
};

class DigitalInputPin {
public:
    DigitalInputPin(GpioManager& gpioManager, InputPinConfig config);
    ~DigitalInputPin();

    DigitalInputPin(const DigitalInputPin&) = delete;
    DigitalInputPin& operator=(const DigitalInputPin&) = delete;
    DigitalInputPin(DigitalInputPin&&) = delete;
    DigitalInputPin& operator=(DigitalInputPin&&) = delete;

    bool init();
    bool shutdown();

    bool isHigh() const;
    bool isLow() const;

    unsigned int pin() const;
    const std::string& name() const;
    bool isInitialized() const;

private:
    GpioManager& gpioManager_;
    InputPinConfig config_;
    bool initialized_ = false;

    // methods
    std::optional<PinState> read() const;
};

struct PwmPinConfig {
    std::string name;
    std::uint8_t channel;
    float initialDutyCycle {0.0f};
};

class PwmPin {
public:
    PwmPin(PCADriver& pcaDriver, PwmPinConfig config);

    bool init();
    bool shutdown();

    bool setDutyCycle(float dutyCycle);
    bool forceLow();
    bool forceHigh();

    [[nodiscard]] bool isInitialized() const;
    [[nodiscard]] float dutyCycle() const;
    [[nodiscard]] std::uint8_t channel() const;
    [[nodiscard]] const std::string& name() const;

private:
    PCADriver& pcaDriver_;
    PwmPinConfig config_;
    bool initialized_ {false};
    float dutyCycle_ {0.0f};
};

#endif //EMBEDDED_VEHICLE_STACK_PINS_H
