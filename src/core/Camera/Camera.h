//
// Created by nemanja on 4/23/26.
//

#ifndef EMBEDDED_VEHICLE_STACK_CAMERA_H
#define EMBEDDED_VEHICLE_STACK_CAMERA_H
#include "core/Event/EventProcessor.h"
#include "core/System/Subsystem.h"

// Uninitialized
//     |
//     v
// Initializing -----> Faulted
//     |
//     v
//    Idle ---------> Faulted
//    |  \
//    |   \
//    v    v
// Previewing   Recording   (+ idle moze u stopping)
//     |            |
//     v            v
//    Stopping ------
//       |
//       v
//      Idle
//
// Faulted -> Initializing

enum class CameraState {
    Uninitialized,                  // equivalent to stopped
    Initializing,
    Idle,                           // Initialized & doesn't do anything
    Previewing,
    Recording,
    Stopping,                       // Stoping whatever (previewing, recording)
    Faulted
};

inline std::string toString(const CameraState& state) {
    switch (state) {
        case CameraState::Uninitialized: return "Uninitialized";
        case CameraState::Initializing: return "Initializing";
        case CameraState::Idle: return "Idle";
        case CameraState::Previewing: return "Previewing";
        case CameraState::Recording: return "Recording";
        case CameraState::Stopping: return "Stopping";
        case CameraState::Faulted: return "Faulted";
        default: return "Unknown";
    }
}

inline std::ostream& operator<< (std::ostream& os, const CameraState state) {
    return os << toString(state);
}

class Camera : public Subsystem, public EventProcessor {
public:
    Camera(MessageBus* bus)
    : Subsystem(), EventProcessor(bus)
    {
        currentState_ = CameraState::Uninitialized;
        requestedState_ = CameraState::Uninitialized;
        running_ = false;
        takeSnapShot_ = false;
    }
    ~Camera() override;

    void registerHandlers() override;
    bool start() override;
    bool stop() override;
    std::string_view name() const override;

private:
    // worker thread
    std::jthread workerThread_;
    std::atomic<bool> running_;

    // protects the state variables and takeSnapShot
    // since both the worker and listener camera threads update/read them
    std::mutex cameraMutex_;
    std::condition_variable workerCv_;
    CameraState currentState_;
    CameraState requestedState_;
    bool takeSnapShot_;

    // methods
    static bool canTransition(const CameraState& from, const CameraState& to);
    void updateState(const CameraState& to);
    void workerLoop();

    // worker functions
    // important: somehow must periodically check if they should keep running
    bool takeSnapShot();
    bool startPreview();
    bool startRecording();
};



#endif //EMBEDDED_VEHICLE_STACK_CAMERA_H
