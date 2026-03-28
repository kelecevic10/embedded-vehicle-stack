//
// Created by nemanja on 3/20/26.
//

#ifndef EMBEDDED_VEHICLE_STACK_SUBSYSTEM_H
#define EMBEDDED_VEHICLE_STACK_SUBSYSTEM_H
#include <string_view>

class Subsystem {
public:
    virtual ~Subsystem() = default;

    // virtual void initialize() = 0;
    virtual bool start() = 0;
    virtual bool stop() = 0;
    virtual std::string_view name() const = 0;
};

#endif //EMBEDDED_VEHICLE_STACK_SUBSYSTEM_H

