//
// Created by nemanja on 5/21/26.
//

#ifndef EMBEDDED_VEHICLE_STACK_MOTORTYPES_H
#define EMBEDDED_VEHICLE_STACK_MOTORTYPES_H
#include "core/HAL/Pins/Pins.h"


enum class MotorDirection { FORWARD, BACKWARD };

inline std::string toString(const MotorDirection direction) {
    switch (direction) {
        case MotorDirection::FORWARD: return "FORWARD";
        case MotorDirection::BACKWARD: return "BACKWARD";
    }
    return "UNKNOWN";
}

enum class MotorState { Uninitialized, Stopped, RunningForward, RunningBackward, Faulted };

inline std::string toString(const MotorState state) {
    switch (state) {
        case MotorState::Uninitialized: return "UNINITIALIZED";
        case MotorState::Stopped: return "STOPPED";
        case MotorState::RunningForward: return "RUNNING_FORWARD";
        case MotorState::RunningBackward: return "RUNNING_BACKWARD";
        default: return "UNKNOWN";
    }
}

struct MotorConfig {
    OutputPinConfig rEnable;
    OutputPinConfig lEnable;
    PwmPinConfig rPwm;
    PwmPinConfig lPwm;
};


#endif //EMBEDDED_VEHICLE_STACK_MOTORTYPES_H
