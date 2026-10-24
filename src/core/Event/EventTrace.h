//
// Created by nemanja on 10/20/26.
//

#ifndef EMBEDDED_VEHICLE_STACK_EVENTTRACE_H
#define EMBEDDED_VEHICLE_STACK_EVENTTRACE_H


#include <atomic>

class EventTrace {
public:
    static void setEnabled(bool enabled) {
        enabled_.store(enabled, std::memory_order_relaxed);
    }

    static bool isEnabled() {
        return enabled_.load(std::memory_order_relaxed);
    }

private:
    inline static std::atomic<bool> enabled_{true};
};

#endif //EMBEDDED_VEHICLE_STACK_EVENTTRACE_H
