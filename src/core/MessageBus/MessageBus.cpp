//
// Created by nemanja on 3/31/26.
//

#include "MessageBus.h"

#include <format>
#include <thread>

#include "utils/logger/Logger.h"


MessageBus::MessageBus() : isRunning(false)
{

}

bool MessageBus::start()
{
    isRunning = true;
    try {
        thread_ = std::jthread(&MessageBus::loop, this);
    } catch (std::system_error& e) {
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

        cv_.wait(lock, [this](){return !messages_.empty() || !isRunning.load();});
        while (!messages_.empty())
        {
            // top() returns a const reference which is not moveable -> we cast it in non-const reference
            std::unique_ptr<Event> msg = std::move(const_cast<std::unique_ptr<Event>&>(messages_.top()));
            const Event& message = *msg;
            messages_.pop();

            // copy all the callback functions and release a lock not to keep callback handlers in the critical section.
            // we must copy subscribers because when iterating through them, if subscribe() adds a new one - program will crash
            if (subscribers_.contains(message.type)) {
                const auto subscribers = subscribers_[message.type];
                lock.unlock();
                for (const auto& callback : subscribers) {
                    callback(message);
                }
                lock.lock();
            }
        }
    }
}

void MessageBus::subscribe(const EventType event, std::function<void(const Event&)> handler)
{
    std::lock_guard lock(mutex_);
    subscribers_[event].push_back(std::move(handler));
}

void MessageBus::publish(Event event)
{
    std::lock_guard lock(mutex_);
    messages_.push(std::make_unique<Event>(std::move(event)));
    cv_.notify_one();
}