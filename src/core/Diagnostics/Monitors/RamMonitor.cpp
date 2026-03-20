//
// Created by nemanja on 1/16/26.
//

#include "RamMonitor.h"

#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>

#include "utils/logger/Logger.h"

void RamMonitor::update() {
    auto [total, available] = readMemoryUsage();

    if (total == 0) {
        monitorStatus = MonitorStatus::ERROR;
        ramUsage = 0;
        return;
    }

    double used = total - available;
    ramUsage = used / static_cast<double>(total) * 100;
}

std::string_view RamMonitor::getName() const {
    return "RAM Monitor";
}

bool RamMonitor::isCritical() const {
    return true;
}

MonitorStatus RamMonitor::status() {
    if (ramUsage > 90)
        return MonitorStatus::ERROR;

    if (ramUsage > 70)
        return MonitorStatus::WARNING;

    return MonitorStatus::OK;
}

std::pair<uint64_t, uint64_t> RamMonitor::readMemoryUsage() {
    std::ifstream file("/proc/meminfo");
    if (!file.is_open()) {
        Logger::error("Could not open /proc/meminfo");
        monitorStatus = MonitorStatus::WARNING;
        return {0, 0};
    }

    std::string line;
    std::unordered_map<std::string, uint64_t> memoryUsage;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string key;

        uint64_t value;
        iss >> key >> value;
        key = key.substr(0, key.find(':'));

        memoryUsage[key] = value;

        if (memoryUsage.contains("MemTotal")
            && memoryUsage.contains("MemAvailable")) {
            break;
        }
    }

    uint64_t total = memoryUsage.at("MemTotal");
    uint64_t available = memoryUsage.at("MemAvailable");
    return {total, available};
}

