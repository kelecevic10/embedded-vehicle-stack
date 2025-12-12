//
// Created by nemanja on 11/29/25.
//

#pragma once

#include <cstdint>
#include <string>

namespace TimeUtils {
    std::string time_stamp();
    uint64_t get_time_in_ms();
    std::string format_time(uint64_t time_in_ms);
}

