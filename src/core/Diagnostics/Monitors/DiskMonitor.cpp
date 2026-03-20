//
// Created by nemanja on 1/16/26.
//

#include "DiskMonitor.h"
#include <sys/statvfs.h>
#include <cstdint>
#include <stdexcept>


DiskMonitor::DiskMonitor() : usagePercent(0.0)
{

}

void DiskMonitor::update() {
    DiskUsageInfo usage = readDiskUsage();
    usagePercent = usage.usedPercent;

    if (usagePercent > 90) {
        monitorStatus = MonitorStatus::ERROR;
    } else if (usagePercent > 75) {
        monitorStatus = MonitorStatus::WARNING;
    } else {
        monitorStatus = MonitorStatus::OK;
    }
}

std::string_view DiskMonitor::getName() const {
    return "Disk Monitor";
}

bool DiskMonitor::isCritical() const {
    // think about it again
    return true;
}

MonitorStatus DiskMonitor::status() {
    return monitorStatus;
}

DiskMonitor::DiskUsageInfo DiskMonitor::readDiskUsage() {
    struct statvfs fs {};

    if (statvfs("/", &fs) != 0) {
        throw std::runtime_error("Could not read disk usage");
    }

    uint64_t total = static_cast<uint64_t>(fs.f_blocks)  * fs.f_frsize;
    uint64_t free  = static_cast<uint64_t>(fs.f_bavail)  * fs.f_frsize;

    if (total == 0) {
        throw std::runtime_error("Could not read disk usage");
    }

    double usedPercent = (1.0 - (static_cast<double>(free) / static_cast<double>(total))) * 100.0;

    return DiskUsageInfo {
        .totalBytes = total,
        .freeBytes = free,
        .usedPercent = usedPercent
    };
}
