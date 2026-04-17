//
// Created by nemanja on 12/17/25.
//

#ifndef EMBEDDED_VEHICLE_STACK_ENUMS_H
#define EMBEDDED_VEHICLE_STACK_ENUMS_H
#include <cstdint>
#include <format>
#include <iostream>
#include <string>

#include "utils/TimeUtils.h"

enum struct DiagHealth {
    OK,
    WARNING,
    ERROR
};


inline std::string_view toString(const DiagHealth health) {
    switch (health) {
        case DiagHealth::OK: return "OK";
        case DiagHealth::WARNING: return "WARNING";
        case DiagHealth::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
};

inline std::ostream &operator<<(std::ostream &os, const DiagHealth health) {
    return os << toString(health);
}

struct SnapShot {
    double cpu_usage;
    double ram_usage;
    double disk_usage;
    // double cpu_temp;
    // double network_rtt;

    DiagHealth diag_health;
    uint64_t timestamp;
};


inline std::string SnapShotToString(const SnapShot& snap) {
    std::string time_str = TimeUtils::format_time(snap.timestamp);
    return std::format(
        "CPU: {:.2f}\nRAM: {:.2f}\nDISK: {:.2f}\nDIAG HEALTH: {}\nTime: {}\n",
        snap.cpu_usage, snap.ram_usage, snap.disk_usage, toString(snap.diag_health), time_str
        );
}

#endif //EMBEDDED_VEHICLE_STACK_ENUMS_H