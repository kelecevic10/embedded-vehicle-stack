//
// Created by nemanja on 12/17/25.
//

#ifndef EMBEDDED_VEHICLE_STACK_ENUMS_H
#define EMBEDDED_VEHICLE_STACK_ENUMS_H
#include <cstdint>
#include <iostream>
#include <string>
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



#endif //EMBEDDED_VEHICLE_STACK_ENUMS_H