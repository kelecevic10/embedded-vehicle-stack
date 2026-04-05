//
// Created by nemanja on 1/16/26.
//

#ifndef EMBEDDED_VEHICLE_STACK_RAMMONITOR_H
#define EMBEDDED_VEHICLE_STACK_RAMMONITOR_H
#include "Monitor.h"
#include <cstdint>

class RamMonitor: public Monitor {
public:
    explicit RamMonitor()
        : ramUsage_(0) {
    }

    void update() override;
    std::string_view getName() const override;
    bool isCritical() const override;
    MonitorStatus status() override;
    double getUsage() const override {return ramUsage_;}

private:
    double ramUsage_;
    std::pair<uint64_t, uint64_t> readMemoryUsage();
};


#endif //EMBEDDED_VEHICLE_STACK_RAMMONITOR_H