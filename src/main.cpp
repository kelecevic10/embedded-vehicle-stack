#include <iostream>

#include "utils/logger/FileSink.h"
#include "utils/logger/Logger.h"
#include "utils/logger/TerminalSink.h"

int main() {

    Logger::addSink(std::make_unique<TerminalSink>());

    auto fileSink = std::make_unique<FileSink>("log.txt");
    FileSink* fileSinkptr = fileSink.get();
    Logger::addSink(std::move(fileSink));

    Logger::info("Hello World!");
    Logger::warning("Warning!");
    Logger::error("Error!");

    Logger::removeSink(fileSinkptr);

    Logger::info("This message will not be logged to file.");
    Logger::warning("This message will not be logged to file.");
    Logger::error("This message will not be logged to file.");

    return 0;
}
