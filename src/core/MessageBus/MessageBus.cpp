//
// Created by nemanja on 3/31/26.
//

#include "MessageBus.h"

#include <thread>


MessageBus::MessageBus() : isRunning(false)
{
    // todo: greska jer nit odmah izlazi iz while-a
    thread_ = std::jthread(&MessageBus::loop, this);
}

bool MessageBus::start()
{
    isRunning = true;
    return true;
}

bool MessageBus::stop()
{
    isRunning = false;
    return true;
}

std::string_view MessageBus::name() const
{
    return "MessageBus";
}


void MessageBus::loop()
{
    while (isRunning.load()) {
        std::unique_lock lock(mutex_);

        cv_.wait(lock, [this](){return !messages_.empty();});
        while (!messages_.empty())
        {
            const Event message = std::move(messages_.top());
            messages_.pop();
            EventType messageType = message.type;
            // eventually lock.unlock(mutex_) because its not a big deal if race condition occurs in subscribers_ ???
            // plus additional locking before reading a most priority message
            const auto& subscribers = subscribers_[messageType];
            for (const auto& callback : subscribers) {
                callback(message);
            }
        }
    }
}

void MessageBus::subscribe(const EventType event, std::function<void(const Event&)> handler)
{
    std::lock_guard lock(mutex_);
    subscribers_[event].push_back(std::move(handler));
}

void MessageBus::publish(const Event &event)
{
    std::lock_guard lock(mutex_);
    messages_.push(event);
    cv_.notify_one();
}

