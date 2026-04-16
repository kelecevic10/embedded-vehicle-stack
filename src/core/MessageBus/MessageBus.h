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
#include "utils/enums/DiagTypes.h"
#include "../Event/Event.h"

class EventProcessor;

class MessageBus : public Subsystem {
public:
    MessageBus();
    ~MessageBus() override = default;

    // methods
    bool start() override;
    bool stop() override;
    std::string_view name() const override;

    void subscribe(EventType event, EventProcessor* subscriber);
    void unsubscribe(EventType event, EventProcessor* subscriber);
    void publish(const EventPtr&);
private:
    // event handler (callback functions) must have the signature below
    std::map<EventType, std::vector<EventProcessor*>> subscribers_;
    std::priority_queue
    <
        EventPtr,
        std::vector<EventPtr>,
        EventComparator
    > messages_;
    std::jthread thread_;
    std::atomic<bool> isRunning;
    std::mutex mutex_;
    std::condition_variable cv_;

    // methods
    void loop();
};



#endif //EMBEDDED_VEHICLE_STACK_MESSAGEBUS_H
