//
// Created by nemanja on 11/27/25.
//

# pragma once
#include <cstdint>
#include <fstream>
#include <string>


// TODO: maybe log into file

enum class LogType : uint8_t {
    INFO,
    WARNING,
    ERROR
};

namespace TextColor {
    constexpr std::string_view GREEN  = "\033[0;32m";
    constexpr std::string_view YELLOW = "\033[0;33m";
    constexpr std::string_view RED    = "\033[0;31m";
    constexpr std::string_view RESET  = "\033[0m";
}


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
        case LogType::INFO: return TextColor::GREEN;
        case LogType::WARNING: return TextColor::YELLOW;
        case LogType::ERROR: return TextColor::RED;
    }
    return TextColor::RESET;
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


