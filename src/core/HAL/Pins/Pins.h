//
// Created by nemanja on 5/14/26.
//

#ifndef EMBEDDED_VEHICLE_STACK_PINS_H
#define EMBEDDED_VEHICLE_STACK_PINS_H
#include "core/HAL/GpioManager.h"

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


#endif //EMBEDDED_VEHICLE_STACK_PINS_H
