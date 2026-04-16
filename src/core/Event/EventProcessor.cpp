//
// Created by nemanja on 4/15/26.
//

#include "EventProcessor.h"

#include <format>

#include "core/MessageBus/MessageBus.h"
#include "utils/logger/Logger.h"

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

void EventProcessor::startEventLoop() {
    terminate_.store(false);
    thread_ = std::jthread(&EventProcessor::loop, this);
}

void EventProcessor::stopEventLoop() {
    terminate_.store(true);
    cv_.notify_one();
    if (thread_.joinable()) {
        thread_.join();
    }
}

void EventProcessor::emitEvent(const EventPtr& event) {
    std::lock_guard lock(mutex_);
    queue_.push(event);
    cv_.notify_one();
}

void EventProcessor::loop() {
    while (!terminate_.load()) {
        std::unique_lock lock(mutex_);
        cv_.wait(lock, [this](){return !queue_.empty() || terminate_.load();});
        if (terminate_.load() && queue_.empty())
            break;

        while (!queue_.empty()) {
            const auto event = queue_.top();
            queue_.pop();
            if (handlers_.contains(event->type)) {
                const auto& handlers = handlers_[event->type];
                lock.unlock();
                for (auto& handler : handlers) {
                    handler(*event);
                }
                lock.lock();
            } else {
                Logger::error(std::format("No handler for event type: {}", EventTypeToString(event->type)));
            }
        }
    }
}

// publish
void EventProcessor::publish(Event event) const {
    const auto eventPtr = std::make_shared<const Event>(std::move(event));
    bus_->publish(eventPtr);
}