//
// Created by nemanja on 3/20/26.
//

#ifndef EMBEDDED_VEHICLE_STACK_SUBSYSTEM_H
#define EMBEDDED_VEHICLE_STACK_SUBSYSTEM_H
#include <string_view>
#include "../Event/Event.h"

class Subsystem {
public:
    virtual ~Subsystem() = default;

    // start should:
    // startEventLoop() (if has one)
    // subscribe to particular event type via bus
    virtual bool start() = 0;
    // stop should:
    // unsubscribe from subscribed event types
    // stopEventLoop() (if has one)
    virtual bool stop() = 0;
    virtual std::string_view name() const = 0;

};

#endif //EMBEDDED_VEHICLE_STACK_SUBSYSTEM_H

