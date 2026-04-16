//
// Created by nemanja on 4/15/26.
//

#ifndef EMBEDDED_VEHICLE_STACK_EVENTPROCESSOR_H
#define EMBEDDED_VEHICLE_STACK_EVENTPROCESSOR_H

#include <condition_variable>
#include <functional>
#include <map>
#include <queue>
#include <thread>

#include "Event.h"

class MessageBus;

using EventQueue = std::priority_queue
<
    EventPtr,
    std::vector<EventPtr>,
    EventComparator
>;

class EventProcessor {
public:
    explicit EventProcessor(MessageBus* bus);
    virtual ~EventProcessor();

    // subscribe all handlers using subscription method
    // NOTE: call it in a derived class constructor before the startEventLoop()
    virtual void registerHandlers() = 0;

    // publisher will create an Event object & move it into argument
    void publish(Event event) const;
    // message bus calls whenever event occurs to route it into EventProcessor queue
    void emitEvent(const EventPtr& event);

protected:
    void loop();
    void subscribe(EventType event_type, std::function<void(const Event &)> handler);
    void startEventLoop();
    void stopEventLoop();

    MessageBus* bus_;
    std::jthread thread_;
    std::atomic<bool> terminate_ {false};
    EventQueue queue_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::map<EventType, std::vector<std::function<void(const Event&)>>> handlers_;
};



#endif //EMBEDDED_VEHICLE_STACK_EVENTPROCESSOR_H
