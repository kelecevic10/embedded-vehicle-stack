//
// Created by nemanja on 3/31/26.
//

#include "MessageBus.h"
#include <format>
#include <thread>
#include "utils/logger/Logger.h"
#include "core/Event/EventProcessor.h"

MessageBus::MessageBus() : isRunning(false)
{

}

bool MessageBus::start()
{
    if (isRunning.load()) {
        Logger::warning("MessageBus already running!");
        return false;
    }

    isRunning = true;
    try {
        thread_ = std::jthread(&MessageBus::loop, this);
    } catch (std::system_error& e) {
        isRunning = false;
        Logger::error(std::format("Failed to create message bus thread: {}", e.what()));
        return false;
    }
    return true;
}

bool MessageBus::stop()
{
    isRunning = false;
    // if the thread waits on cv, isRunning = false will not force it to quit the while loop and shut down
    cv_.notify_one();
    if (thread_.joinable()) {
        thread_.join();
    }
    return true;
}

std::string_view MessageBus::name() const
{
    return "MessageBus";
}


void MessageBus::loop()
{
    while (true) {
        std::unique_lock lock(mutex_);

        cv_.wait(lock, [this]() {
            return !messages_.empty() || !isRunning.load();
        });

        // graceful shutdown
        if (!isRunning.load() && messages_.empty()) {
            break;
        }

        EventPtr event = messages_.top();
        messages_.pop();

        std::vector<EventProcessor*> subscribersCopy;
        auto it = subscribers_.find(event->type);
        if (it != subscribers_.end()) {
            subscribersCopy = it->second;
        }

        lock.unlock();

        for (auto* subscriber : subscribersCopy) {
            subscriber->emitEvent(event);
        }
    }
}

void MessageBus::subscribe(const EventType event, EventProcessor* subscriber)
{
    std::lock_guard lock(mutex_);
    if (!subscribers_.contains(event)) {
        subscribers_[event].push_back(subscriber);
    }
}

void MessageBus::unsubscribe(EventType event, EventProcessor *subscriber) {
    std::lock_guard lock(mutex_);

    if (subscribers_.contains(event)) {
        std::erase(subscribers_[event], subscriber);
    }

    if (subscribers_[event].empty()) {
        subscribers_.erase(event);
    }
}


void MessageBus::publish(const EventPtr& event)
{
    std::lock_guard lock(mutex_);

    if (!isRunning.load()) {
        Logger::warning("MessageBus is not running - cannot publish an Event!");
        return;
    }

    messages_.push(event);
    cv_.notify_one();
}
