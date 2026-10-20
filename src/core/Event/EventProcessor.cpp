//
// Created by nemanja on 4/15/26.
//

#include "EventProcessor.h"
#include "core/Event/EventTrace.h"
#include <format>

#include "core/MessageBus/MessageBus.h"
#include "utils/logger/Logger.h"
#include "core/System/Subsystem.h"
#include <exception>

EventProcessor::EventProcessor(MessageBus* bus)
    : bus_(bus)
{

}

EventProcessor::~EventProcessor() {
   stopEventLoop();
}

void EventProcessor::subscribe(const EventType event_type, std::function<void(const Event &)> handler) {
    handlers_[event_type].push_back(std::move(handler));
}

bool EventProcessor::startEventLoop() {
    {
        std::lock_guard lock(mutex_);
        terminate_.store(false);
        acceptingEvents_ = true;
    }

    try {
        thread_ = std::jthread(&EventProcessor::loop, this);
    }
    catch (...) {
        std::lock_guard lock(mutex_);
        acceptingEvents_ = false;
        terminate_.store(true);
        Logger::error(std::format("{} EventProcessor::startEventLoop() failed", processorName()));
        return false;
    }

    return true;
}

void EventProcessor::stopEventLoop() {
    {
        std::lock_guard lock(mutex_);
        acceptingEvents_ = false;
        terminate_.store(true);
    }

    cv_.notify_one();

    if (thread_.joinable()) {
        thread_.join();
    }
}

void EventProcessor::emitEvent(const EventPtr& event) {
    const auto receiver = processorName();

    if (!event) {
        Logger::error(std::format(
            "[EVENT_TRACE] {} received a null event",
            receiver
        ));
        return;
    }

    std::size_t queueSize;
    {
        std::lock_guard lock(mutex_);

        if (!acceptingEvents_) {
            return;
        }

        queue_.push(event);
        queueSize = queue_.size();
    }

    if (EventTrace::isEnabled()) {
        Logger::info(std::format(
        "[EVENT_TRACE] stage=RECEIVED "
            "event_id={} event_type={} source={} "
            "receiver={} priority={} queue_size={}",
            event->id,
            EventTypeToString(event->type),
            event->source,
            receiver,
            event->priority,
            queueSize
        ));
    }

    cv_.notify_one();
}

void EventProcessor::loop() {
    while (true) {
        std::unique_lock lock(mutex_);

        cv_.wait(lock, [this]() {
            return !queue_.empty() || terminate_.load();
        });

        if (terminate_.load() && queue_.empty()) {
            break;
        }

        while (!queue_.empty()) {
            const auto event = queue_.top();
            queue_.pop();

            const auto handlersIt = handlers_.find(event->type);

            if (handlersIt == handlers_.end()) {
                lock.unlock();

                Logger::error(std::format(
                    "[EVENT_TRACE] stage=FAILED "
                    "event_id={} event_type={} source={} "
                    "receiver={} reason=no_handler",
                    event->id,
                    EventTypeToString(event->type),
                    event->source,
                    processorName()
                ));

                lock.lock();
                continue;
            }

            const auto& handlers = handlersIt->second;

            lock.unlock();

            bool allHandlersSucceeded = true;

            for (std::size_t index = 0; index < handlers.size(); ++index) {
                try {
                    handlers[index](*event);
                }
                catch (const std::exception& exception) {
                    allHandlersSucceeded = false;

                    Logger::error(std::format(
                        "[EVENT_TRACE] stage=FAILED "
                        "event_id={} event_type={} source={} "
                        "receiver={} handler_index={} reason=\"{}\"",
                        event->id,
                        EventTypeToString(event->type),
                        event->source,
                        processorName(),
                        index + 1,
                        exception.what()
                    ));
                }
            }

            if (allHandlersSucceeded && EventTrace::isEnabled()) {
                Logger::info(std::format(
                    "[EVENT_TRACE] stage=HANDLED "
                    "event_id={} event_type={} source={} "
                    "receiver={} handler_count={}",
                    event->id,
                    EventTypeToString(event->type),
                    event->source,
                    processorName(),
                    handlers.size()
                ));
            }

            lock.lock();
        }
    }
}

// publish
void EventProcessor::publish(Event event) const {
    event.source = std::string(processorName());
    const auto eventPtr = std::make_shared<const Event>(std::move(event));
    bus_->publish(eventPtr);
}

std::string_view EventProcessor::processorName() const {
    if (const auto* subsystem = dynamic_cast<const Subsystem*>(this)) {
        return subsystem->name();
    }

    // currently, Logger is the only one EventProcessor that is not Subsystem, too.
    return "Logger";
}