//
// Created by nemanja on 11/29/25.
//

#include "TimeUtils.h"
#include <chrono>
#include <format>

namespace TimeUtils {
    std::string time_stamp() {
        // get the current time point from a system clock
        auto now = std::chrono::system_clock::now();
        return std::format("{:%d-%m-%Y %H:%M}", now);
    }
}