//
// Created by nemanja on 4/8/26.
//

#include "LED.h"

#include "utils/logger/Logger.h"

using hal::PinState;
bool LED::turnOn() const {
    if (write(PinState::HIGH)) {
        return true;
    }
    Logger::error("Failed to turn on LED");
    return false;
}

bool LED::turnOff() const {
    if (write(PinState::LOW)) {
        return true;
    }
    Logger::error("Failed to turn off LED");
    return false;
}
