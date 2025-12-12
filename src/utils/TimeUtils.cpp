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

    uint64_t get_time_in_ms() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch())
                .count();
    }

    std::string format_time(uint64_t time_in_ms) {
        constexpr std::string_view kDateTimeFormat = "{:%d-%m-%Y %H:%M:%S}";

        const auto since_epoch = std::chrono::milliseconds {time_in_ms};
        const std::chrono::sys_time tp {since_epoch};

        return std::format(kDateTimeFormat, tp);
    }
}
