//
// Created by nemanja on 11/27/25.
//
#include "Logger.h"

#include <iostream>
#include <ostream>
#include <string>
#include <chrono>
#include <format>

#include "../TimeUtils.h"

// define the static vector
std::vector<std::unique_ptr<Sink>> Logger::sinks = {};

std::string Logger::formatLog(LogType type, std::string_view message) {

    std::string timestamp = TimeUtils::time_stamp();
    const std::string_view logType = logTypeToString(type);
    const std::string_view color = logTypeToColor(type);

    std::string result;
    constexpr size_t EXTRA_CAPACITY = 20;
    result.reserve(message.size() + timestamp.size() + logType.size() + EXTRA_CAPACITY);
    result += color;
    result += std::format("[{}] [{}] {}", timestamp, logType, message);
    result += TextColor::RESET;

    return result;
}

// the creation (allocation) of a sink is made directly when calling this func
// via addSink(std::make_unique<FileSink>()) <- example
void Logger::addSink(std::unique_ptr<Sink> sink) {
    sinks.push_back(std::move(sink));
}

void Logger::removeSink(const Sink* toRemove) {
    sinks.erase(
        std::remove_if(
            sinks.begin(),
            sinks.end(),
            [&](const std::unique_ptr<Sink>& sink) {
                return sink.get() == toRemove;
            }
        ),
        sinks.end()
    );
}

void Logger::setDefaultLogFolder(const std::string& folder) {
    defaultLogFolder = ".." + folder;
}

const std::string & Logger::getDefaultLogFolder() {
    return defaultLogFolder;
}


void Logger::info(std::string_view message)
{
    auto formatted_message = formatLog(LogType::INFO, message);
    for (auto& sink: sinks) {
        sink->writeLog(formatted_message);
    }
};

void Logger::warning(std::string_view message)
{
    auto formatted_message = formatLog(LogType::WARNING, message);
    for (auto& sink: sinks) {
        sink->writeLog(formatted_message);
    }
}

void Logger::error(std::string_view message)
{
    auto formatted_message = formatLog(LogType::ERROR, message);
    for (auto& sink: sinks) {
        sink->writeLog(formatted_message);
    }
}