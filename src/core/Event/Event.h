//
// Created by nemanja on 4/15/26.
//

#ifndef EMBEDDED_VEHICLE_STACK_EVENT_H
#define EMBEDDED_VEHICLE_STACK_EVENT_H
#include <memory>
#include <string>
#include <variant>
#include <atomic>
#include <cstdint>
#include <string_view>

#include "../../utils/enums/DiagTypes.h"
#include "core/HAL/Devices/Motor/MotorDriver.h"
#include "utils/TimeUtils.h"



enum class EventType {
    // System & Lifecycle
    SYSTEM_SHUTDOWN,
    SUBSYSTEM_STATE_CHANGED,
    // Diagnostics
    DIAGNOSTICS_UPDATE,
    DIAGNOSTICS_WARNING,
    DIAGNOSTICS_ALERT,
    // Camera
    CAMERA_TAKE_SNAPSHOT,
    CAMERA_START_RECORDING,
    CAMERA_STOP_RECORDING,
    CAMERA_START_PREVIEW,
    CAMERA_STOP_PREVIEW,
    // Motor
    MOTOR_DRIVE,
    MOTOR_STOP,
};

inline std::string EventTypeToString(const EventType t) {
    switch (t) {
        case EventType::SYSTEM_SHUTDOWN:
            return "SYSTEM_SHUTDOWN";
        case EventType::SUBSYSTEM_STATE_CHANGED:
            return "SUBSYSTEM_STATE_CHANGED";
        case EventType::DIAGNOSTICS_UPDATE:
            return "DIAGNOSTICS_UPDATE";
        case EventType::DIAGNOSTICS_WARNING:
            return "DIAGNOSTICS_WARNING";
        case EventType::DIAGNOSTICS_ALERT:
            return "DIAGNOSTICS_ALERT";
        case EventType::CAMERA_TAKE_SNAPSHOT:
            return "CAMERA_TAKE_SNAPSHOT";
        case EventType::CAMERA_START_RECORDING:
            return "CAMERA_START_RECORDING";
        case EventType::CAMERA_STOP_RECORDING:
            return "CAMERA_STOP_RECORDING";
        case EventType::CAMERA_START_PREVIEW:
            return "CAMERA_START_PREVIEW";
        case EventType::CAMERA_STOP_PREVIEW:
            return "CAMERA_STOP_PREVIEW";
        case EventType::MOTOR_DRIVE:
            return "MOTOR_DRIVE";
        case EventType::MOTOR_STOP:
            return "MOTOR_STOP";
        default:
            return "UNKNOWN";
    }
}

enum class MotorId {
    LEFT,
    RIGHT,
    BOTH
};

struct MotorCommandPayload {
    MotorId motorId;
    MotorDirection direction;
    float dutyCycle;
};

using EventPayload = std::variant<
    std::monostate,   // empty, signal only
    std::string,
    int,
    SnapShot,
    MotorCommandPayload,
    MotorId
>;

struct Event {
    std::uint64_t id;
    EventType type;
    EventPayload payload;
    int priority;
    std::uint64_t createdAtMs;
    std::string source;

    // we move payload because it can contain very large data -> this is much more effective
    // priority politics:
    // [0-4] critical: Emergency stops, hardware failures ...
    // [5-9] system: Module startup/shutdown signals, diagnostic alerts
    // [10] normal: Default priority, standard telemetry, routine diagnostics, user commands ...
    // [11-20] background: logging to file, heartbeat signals, UI updates ...; These are processed only when the bus is idle.
    explicit Event(
        EventType type,
        EventPayload payload = std::monostate {},
        int priority = 10,
        std::string source = "UNKNOWN"
    )
        : id(nextId_.fetch_add(1, std::memory_order_relaxed)),
          type(type),
          payload(std::move(payload)),
          priority(priority),
          createdAtMs(TimeUtils::get_time_in_ms()),
          source(std::move(source))
    {
    }

private:
    inline static std::atomic<std::uint64_t> nextId_ {1};
};

// Use const Event because one published event is shared across
// multiple subscribers. Immutability prevents accidental
// modification of the shared event state and makes dispatch safer.
using EventPtr = std::shared_ptr<const Event>;
struct EventComparator {
    bool operator()(const EventPtr& a, const EventPtr& b) const {
        // returns true if a is less (important) than b
        if (a->priority != b->priority)
            return a->priority > b->priority;
        return a->id > b->id;
    }
};


#endif //EMBEDDED_VEHICLE_STACK_EVENT_H
