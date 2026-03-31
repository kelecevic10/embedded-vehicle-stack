//
// Created by nemanja on 3/31/26.
//

#ifndef EMBEDDED_VEHICLE_STACK_MESSAGEBUS_H
#define EMBEDDED_VEHICLE_STACK_MESSAGEBUS_H
#include <condition_variable>
#include <functional>
#include <map>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <variant>

#include "core/System/Subsystem.h"
#include "utils/TimeUtils.h"


enum class EventType {
    // System & Lifecycle
    SYSTEM_SHUTDOWN,

    // Diagnostics
    DIAGNOSTICS_UPDATE,
    DIAGNOSTICS_ALERT,
};

struct DiagnosticsData {
    double cpu_usage;
    double ram_usage;
    double disk_usage;
};

using EventPayload = std::variant<
    std::monostate,   // empty, signal only
    std::string,
    int,
    DiagnosticsData
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

struct EventComparator {
    bool operator()(const Event& a, const Event& b) const {
        // returns true if a is less (important) than b
        return a.priority > b.priority;
    }
};

class MessageBus : public Subsystem {
public:
    MessageBus();
    ~MessageBus() override = default;

    // methods
    bool start() override;
    bool stop() override;
    std::string_view name() const override;

    void subscribe(EventType event, std::function<void(const Event&)> handler);
    void publish(const Event& event);


private:
    // event handler (callback functions) must have the signature below
    std::map<EventType, std::vector<std::function<void(const Event&)>>> subscribers_;
    std::priority_queue<Event, std::vector<Event>, EventComparator> messages_;
    std::jthread thread_;
    std::atomic<bool> isRunning;
    std::mutex mutex_;
    std::condition_variable cv_;

    // methods
    void loop();
};



#endif //EMBEDDED_VEHICLE_STACK_MESSAGEBUS_H
