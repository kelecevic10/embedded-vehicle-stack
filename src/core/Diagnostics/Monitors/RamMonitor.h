//
// Created by nemanja on 1/16/26.
//

#ifndef EMBEDDED_VEHICLE_STACK_RAMMONITOR_H
#define EMBEDDED_VEHICLE_STACK_RAMMONITOR_H
#include "Monitor.h"


class RamMonitor: public Monitor {
public:
    explicit RamMonitor()
        : ramUsage(0) {
    }

    void update() override;
    [[nodiscard]] std::string_view getName() const override;
    [[nodiscard]] bool isCritical() const override;
    MonitorStatus status() override;

    double getRamUsage() const {return ramUsage;}

private:
    double ramUsage;
    std::pair<uint64_t, uint64_t> readMemoryUsage();
};


#endif //EMBEDDED_VEHICLE_STACK_RAMMONITOR_H