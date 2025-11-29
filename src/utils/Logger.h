//
// Created by nemanja on 11/27/25.
//

# pragma once
#include <cstdint>
#include <string>


// TODO: maybe log into file

enum class LogType : uint8_t {
    INFO,
    WARNING,
    ERROR
};

constexpr std::string_view logTypeToString(LogType type) {
    switch (type) {
        case LogType::INFO: return "INFO";
        case LogType::WARNING: return "WARNING";
        case LogType::ERROR: return "ERROR";
    }
    return "UNKNOWN";
}

inline std::string_view logTypeToColor(LogType type) {
    switch (type) {
        case LogType::INFO: return "\033[0;32m";
        case LogType::WARNING: return "\033[0;33m";
        case LogType::ERROR: return "\033[0;31m";
    }
    return "\033[0m";
}

class Logger {
public:
    Logger() = delete;
    ~Logger() = delete;

    static void info(std::string_view);
    static void warning(std::string_view);
    static void error(std::string_view);

private:
    static void log(LogType, std::string_view);
};


