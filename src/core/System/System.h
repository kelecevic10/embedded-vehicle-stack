//
// Created by nemanja on 11/26/25.
//

#ifndef ECU_CAR_CONTROLSYSTEM_H
#define ECU_CAR_CONTROLSYSTEM_H

#include <memory>
#include <vector>
#include <string>
#include <iostream>
#include "Subsystem.h"

enum class SystemState {
    Initializing,       // initializing the system
    Starting,           // starting the whole system
    Running,            // successfully running
    Stopping,           // shutting down
    Stopped,            // successfully shut down
    Faulted             // unsuccessfully shut down
};

inline std::string toString(const SystemState state) {
    switch (state) {
        case SystemState::Initializing:
            return "Initializing";
        case SystemState::Running:
            return "Running";
        case SystemState::Stopping:
            return "Stopping";
        case SystemState::Faulted:
            return "Faulted";
        case SystemState::Stopped:
            return "Stopped";
        case SystemState::Starting:
            return "Starting";
        default:
            return "Unknown";
    }
}

inline std::ostream& operator<< (std::ostream& os, const SystemState state) {
    os << toString(state);
    return os;
}

class System {
public:
    System(): state_(SystemState::Stopped), subsystems_() {
        init();
    };
    ~System();

    // methods
    bool start();
    bool stop();

    // getters
    SystemState getState() const { return state_; }
private:

    SystemState state_;
    std::vector<std::unique_ptr<Subsystem>> subsystems_;


    // methods
    void init();
};


#endif //ECU_CAR_CONTROLSYSTEM_H