//
// Created by nemanja on 12/5/25.
//

#include "FileSink.h"

#include "Logger.h"
#include <filesystem>

FileSink::FileSink(const std::string& file_name)
{
    namespace fs = std::filesystem;
    fs::create_directories(Logger::getDefaultLogFolder());
    std::string full_path = Logger::getDefaultLogFolder() + file_name;

    file.open(full_path, std::ios::out | std::ios::app);

    // TODO: handle the exception
    if (!file.is_open()) {
        throw std::runtime_error("FileSink: Could not open log file");
    }
}

// clear formatted string from ANSI color tags
std::string clear_color(std::string_view str) {
    std::string result;
    result.reserve(str.size());

    bool skipping = false;
    for (const char c : str) {
        if (!skipping) {
            if (c == '\033') {
                skipping = true;
            } else {
                result.push_back(c);
            }
        } else {
            if (c == 'm') {
                skipping = false;
            }
        }
    }
    return result;
}

void FileSink::writeLog(std::string_view formatted_message) {
    file << clear_color(std::string(formatted_message)) << '\n';
}