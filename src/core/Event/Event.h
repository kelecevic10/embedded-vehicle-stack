//
// Created by nemanja on 4/15/26.
//

#ifndef EMBEDDED_VEHICLE_STACK_EVENT_H
#define EMBEDDED_VEHICLE_STACK_EVENT_H
#include <memory>
#include <string>
#include <variant>

#include "../../utils/enums/DiagTypes.h"
#include "utils/TimeUtils.h"



enum class EventType {
    // System & Lifecycle
    SYSTEM_SHUTDOWN,

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
};

inline std::string EventTypeToString(const EventType t) {
    switch (t) {
        case EventType::SYSTEM_SHUTDOWN:
            return "SYSTEM_SHUTDOWN";
        case EventType::DIAGNOSTICS_UPDATE:
            return "DIAGNOSTICS_UPDATE";
        case EventType::DIAGNOSTICS_WARNING:
            return "DIAGNOSTICS_WARNING";
        case EventType::DIAGNOSTICS_ALERT:
            return "DIAGNOSTICS_ALERT";
        default:
            return "UNKNOWN";
    }
}

using EventPayload = std::variant<
    std::monostate,   // empty, signal only
    std::string,
    int,
    SnapShot
>;

struct Event {
    EventType type;
    int priority;
    std::string timestamp;
    EventPayload payload;

    // we move payload because it can contain very large data -> this is much more effective
    // priority politics:
    // [0-4] critical: Emergency stops, hardware failures ...
    // [5-9] system: Module startup/shutdown signals, diagnostic alerts
    // [10] normal: Default priority, standard telemetry, routine diagnostics, user commands ...
    // [11-20] background: logging to file, heartbeat signals, UI updates ...; These are processed only when the bus is idle.
    explicit Event(EventType t, EventPayload p = std::monostate{}, const int priority = 10)
        : type(t), priority(priority), payload(std::move(p))
    {
        timestamp = TimeUtils::time_stamp();
    }
};

// Use const Event because one published event is shared across
// multiple subscribers. Immutability prevents accidental
// modification of the shared event state and makes dispatch safer.
using EventPtr = std::shared_ptr<const Event>;

struct EventComparator {
    bool operator()(const EventPtr& a, const EventPtr& b) const {
        // returns true if a is less (important) than b
        return a->priority > b->priority;
    }
};


#endif //EMBEDDED_VEHICLE_STACK_EVENT_H
