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
#include <string_view>
#include "Event.h"

// EventProcessor usage model:
// -----------------------------------------------------------------
// 1. Inherit EventProcessor in every module that should participate
//    in the event system (as publisher, listener, or both).
//
// 2. Pass MessageBus* to the EventProcessor base constructor.
//
// 3. Implement registerHandlers() in the derived class.
//    This method should only register local handlers via subscribe().
//    It is called once during object construction, before the event loop starts.
//
// 4. Start the event loop before the module begins receiving events.
//    The event loop runs in its own thread and processes incoming events
//    from the local priority queue.
//
// 5. Subscribe the module to MessageBus for every EventType it should listen to.
//
// 6. To publish an event, create a local Event object and call publish(...).
//    EventProcessor will move it into a shared EventPtr and forward it to MessageBus.
//
// 7. MessageBus routes the same immutable event instance to all subscribed
//    EventProcessor listeners. Each listener receives it through emitEvent(),
//    which pushes it into its local queue.
//
// 8. The EventProcessor worker thread waits for queued events, pops them
//    in priority order, finds matching handlers, and executes them.
//
// 9. On shutdown, always unsubscribe the module from MessageBus before destruction,
//    then stop the event loop so no dangling listener pointers remain in the bus.
//
// In short:
// - subscribe(...)   -> register local handler functions
// - publish(...)     -> send event to MessageBus
// - emitEvent(...)   -> MessageBus injects event into local queue
// - startEventLoop() -> start listener thread
// - stopEventLoop()  -> stop listener thread

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

private:
    [[ nodiscard ]] std::string_view processorName() const;
};



#endif //EMBEDDED_VEHICLE_STACK_EVENTPROCESSOR_H
