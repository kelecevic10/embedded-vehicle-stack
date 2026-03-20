//
// Created by nemanja on 12/17/25.
//

#ifndef EMBEDDED_VEHICLE_STACK_ENUMS_H
#define EMBEDDED_VEHICLE_STACK_ENUMS_H
#include <cstdint>

struct SnapShot {
    double cpu_usage;
    double ram_usage;
    double disk_usage;
    // double cpu_temp;
    // double network_rtt;

    uint64_t timestamp;
};

#endif //EMBEDDED_VEHICLE_STACK_ENUMS_H