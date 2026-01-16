//
// Created by nemanja on 1/16/26.
//

#ifndef EMBEDDED_VEHICLE_STACK_DISKMONITOR_H
#define EMBEDDED_VEHICLE_STACK_DISKMONITOR_H
#include "Monitor.h"


class DiskMonitor : public Monitor {
public:
    DiskMonitor();

    void update() override;
    std::string_view getName() const override;
    bool isCritical() const override;
    MonitorStatus status() override;

    double getDiskUsage() const { return usagePercent; }
private:
    struct DiskUsageInfo {
        uint64_t totalBytes;
        uint64_t freeBytes;
        double usedPercent;
    };

    DiskUsageInfo readDiskUsage();


private:
    double usagePercent;
};


#endif //EMBEDDED_VEHICLE_STACK_DISKMONITOR_H