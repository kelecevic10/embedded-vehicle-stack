//
// Created by nemanja on 11/27/25.
//
#include "Logger.h"

#include <iostream>
#include <ostream>
#include <string>
#include <chrono>
#include <format>

#include "TimeUtils.h"


void Logger::log(LogType type, std::string_view message) {

    std::string timestamp = TimeUtils::time_stamp();
    auto& out = (type == LogType::ERROR ? std::cerr : std::cout);
    const std::string_view str = logTypeToString(type);
    const std::string_view color = logTypeToColor(type);

    out << color
        << std::format("[{}] [{}] {}", str, timestamp, message)
        << "\033[0m"                // reset color
        << '\n';
}

void Logger::info(std::string_view message)
{
    log(LogType::INFO, message);
};

void Logger::warning(std::string_view message)
{
    log(LogType::WARNING, message);
}

void Logger::error(std::string_view message)
{
    log(LogType::ERROR, message);
}