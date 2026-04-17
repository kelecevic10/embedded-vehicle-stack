//
// Created by nemanja on 11/27/25.
//

# pragma once
#include <memory>
#include <vector>
#include "Sink.h"
#include "core/Event/EventProcessor.h"
#include "core/MessageBus/MessageBus.h"


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

class Logger : public EventProcessor {
public:
    Logger(MessageBus* bus_)
        : EventProcessor(bus_)
    {

        Logger::registerHandlers();

        startEventLoop();

        bus_->subscribe(EventType::DIAGNOSTICS_UPDATE, this);
        bus_->subscribe(EventType::DIAGNOSTICS_WARNING, this);
        bus_->subscribe(EventType::DIAGNOSTICS_ALERT, this);
        // bus_->subscribe(EventType::SYSTEM_SHUTDOWN, this);
    };

    ~Logger() override
    {
        bus_->unsubscribe(EventType::DIAGNOSTICS_UPDATE, this);
        bus_->unsubscribe(EventType::DIAGNOSTICS_WARNING, this);
        bus_->unsubscribe(EventType::DIAGNOSTICS_ALERT, this);
        // bus_->unsubscribe(EventType::SYSTEM_SHUTDOWN, this);

        // the base class EventProcessor destructor calls stopEventLoop()
        // stopEventLoop();
    };

    void registerHandlers() override {
        subscribe(EventType::DIAGNOSTICS_UPDATE, [](const Event& e) {
            const auto snapshot = std::get<SnapShot>(e.payload);
            info(std::format("Diagnostics update\n{}", SnapShotToString(snapshot)));
        });

        subscribe(EventType::DIAGNOSTICS_WARNING, [](const Event& e) {
            const auto message = std::get<std::string>(e.payload);
            warning(message);
        });

        subscribe(EventType::DIAGNOSTICS_ALERT, [](const Event& e) {
            const auto message = std::get<std::string>(e.payload);
            error(message);
        });
    };

    static void info(std::string_view);
    static void warning(std::string_view);
    static void error(std::string_view);

    static void addSink(std::unique_ptr<Sink>);
    static void removeSink(const Sink*);

    static void setDefaultLogFolder(const std::string&);
    static const std::string& getDefaultLogFolder();


private:
    static std::string formatLog(LogType, std::string_view);
    static std::vector<std::unique_ptr<Sink>> sinks;

    static inline std::string defaultLogFolder = "../logs/";
};


