//
// Created by nemanja on 4/8/26.
//

#include "LED.h"

#include "utils/logger/Logger.h"

bool LED::turnOn() const {
    if (this->write(hal::PinState::HIGH)) {
        return true;
    }
    Logger::error("Failed to turn on LED");
    return false;
}

bool LED::turnOff() const {
    if (this->write(hal::PinState::LOW)) {
        return true;
    }
    Logger::error("Failed to turn off LED");
    return false;
}
