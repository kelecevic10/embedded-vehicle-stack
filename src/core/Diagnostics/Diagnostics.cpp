//
// Created by nemanja on 12/7/25.
//

#include "Diagnostics.h"
#include <format>

#include "core/MessageBus/MessageBus.h"
#include "Monitors/CpuMonitor.h"
#include "Monitors/DiskMonitor.h"
#include "Monitors/RamMonitor.h"
#include "utils/TimeUtils.h"
#include "utils/logger/Logger.h"


void Diagnostics::initMonitors() {
    cpu_monitor_ = registerMonitor(std::make_unique<CpuMonitor>());
    disk_monitor_ = registerMonitor(std::make_unique<DiskMonitor>());
    ram_monitor_ = registerMonitor(std::make_unique<RamMonitor>());
}


Diagnostics::~Diagnostics() {
    if (running.load()) {
        Diagnostics::stop();
    }
}

bool Diagnostics::start() {
    if (running.load()) {
        Logger::warning("Diagnostics already running!");
        return false;
    }

    if (monitors_.empty()) {
        initMonitors();
    }

    // if the thread already exists - avoid the double start
    // this serves different purposes than atomic running - it manages the code logic
    // whether the whole system should be turned on/off
    // we would set running = false just to indicate the system to shut down and
    // signal the module to join its thread
    // if (diagThread.joinable()) {
    //     Logger::warning("Diagnostics thread already created!");
    //     return false;
    // }

    try {
        running = true;
        diagThread = std::jthread(&Diagnostics::diagnosisLoop, this);
        this->startEventLoop();
        // HERE goes the list of specific event types
        // eg. this->subscribe(Event_type::some_event, this)
        // this->subscribe(Event_type::some_event_2, this)
        // ...
    } catch (const std::system_error &e) {
        Logger::error(std::format("Failed to create diagnostics thread: {}", e.what()));
        running = false;
        monitors_.clear();
        cpu_monitor_ = nullptr;
        ram_monitor_ = nullptr;
        disk_monitor_ = nullptr;
        return false;
    }
    return true;
}

bool Diagnostics::stop() {
    if (!running.load()) {
        Logger::warning("Diagnostics stop() called but module is already stopped.");
        return true;
    }

    running = false;
    if (diagThread.joinable()) {
        diagThread.join();
    }

    // HERE goes the reversed list of the unsubscribed calls
    this->stopEventLoop();

    monitors_.clear();
    cpu_monitor_ = nullptr;
    ram_monitor_ = nullptr;
    disk_monitor_ = nullptr;

    Logger::info("Diagnostics stopped!");
    return true;
}

/**
 * TODO: Diagnostics Production-Phase Refactoring
 * * 1. [LOGIC] Implement State-Change Detection (Edge-Triggering):
 * - Current version emits events every iteration (3000ms), causing "Alarm Flooding."
 * - Strategy: Cache the previous MonitorStatus and only call messageBus_->publish()
 * when a status transition occurs (e.g., OK -> WARNING).
 * * 2. [LOGIC] Multi-Monitor Event Transparency:
 * - Current version prioritizes the first ERROR/WARNING found in a pass,
 * potentially masking other concurrent monitor failures.
 * - Strategy: Ensure every monitor can emit its own independent event
 * before the global health_ summary is calculated.
 * * 3. [CONCURRENCY] std::jthread & Stop Token Integration:
 * - Current version uses a manual atomic 'running' flag which does not
 * interrupt the 3000ms sleep during system shutdown.
 * - Strategy: Use jthread's get_stop_token() and sleep_for logic that
 * responds immediately to a shutdown request.
 * * 4. [PAYLOAD] Structured Alert Data:
 * - Current version formats alerts as plain strings: "Monitor: usage X%".
 * - Strategy: Update EventPayload to support a 'DiagnosticAlert' struct
 * so subscribers can process raw numerical data without string parsing.
 * * 5. [OPTIMIZATION] Dynamic Frequency Scaling:
 * - Current version is fixed at 3000ms.
 * - Strategy: If health_ is ERROR, automatically increase polling frequency
 * (e.g., to 500ms) for high-resolution failure tracking.
 */

void Diagnostics::diagnosisLoop() {
    auto next = std::chrono::steady_clock::now();
    while (running.load()) {
        health_ = DiagHealth::OK;

        // iterate over all monitors to check system health and collect metrics for the snapshot
        for (const auto& monitor : monitors_) {
            Monitor *m_ptr = monitor.get();
            MonitorStatus status_prev = m_ptr->status();

            m_ptr->update();
            switch (m_ptr->status()) {
                case MonitorStatus::ERROR:
                    if (m_ptr->isCritical())
                    {
                        health_ = DiagHealth::ERROR;
                        emitMonitorUpdate(EventType::DIAGNOSTICS_ALERT, m_ptr->getName(), m_ptr->getUsage(), 1);
                    }
                    else
                    {
                        health_ = (health_ == DiagHealth::ERROR) ? health_ : DiagHealth::WARNING;
                        emitMonitorUpdate(EventType::DIAGNOSTICS_WARNING, m_ptr->getName(), m_ptr->getUsage(), 5);
                    }
                    break;
                case MonitorStatus::WARNING:
                    if (health_ == DiagHealth::OK)
                    {
                        health_ = DiagHealth::WARNING;
                    }
                    if (status_prev != MonitorStatus::WARNING)
                        emitMonitorUpdate(EventType::DIAGNOSTICS_WARNING, m_ptr->getName(), m_ptr->getUsage(), 1);
                    break;
                default:
                    break;
            }
        }

        // building & publishing snapshot
        buildSnapShot();

        // the Diagnostics system's update interval
        next += std::chrono::milliseconds(diagFrequency);
        std::this_thread::sleep_until(next);
    }
}

void Diagnostics::registerHandlers() {
    // no events to subscribe to yet
}

DiagHealth Diagnostics::health() const {
    return health_;
}

void Diagnostics::setDiagFrequency(int frequency) {
    diagFrequency = frequency;
}

void Diagnostics::buildSnapShot() {
    {
        std::lock_guard lock(snapShotMutex_);
        snapShot_.cpu_usage = cpu_monitor_->getUsage();
        snapShot_.ram_usage = ram_monitor_->getUsage();
        snapShot_.disk_usage = disk_monitor_->getUsage();
        snapShot_.diag_health = health_;
        snapShot_.timestamp = TimeUtils::get_time_in_ms();
    }

    Event diagUpdate(
        EventType::DIAGNOSTICS_UPDATE,
        snapShot_,
        10
        );

    publish(std::move(diagUpdate));
}

std::string SnapShotToString(const SnapShot& snap) {
    std::string time_str = TimeUtils::format_time(snap.timestamp);
    return std::format(
        "CPU: {:.2f}\nRAM: {:.2f}\nDISK: {:.2f}\nTime: {}\n",
        snap.cpu_usage, snap.ram_usage, snap.disk_usage, time_str
        );
}

SnapShot Diagnostics::getSnapShot() {
    std::lock_guard lock(snapShotMutex_);
    return snapShot_;
}

void Diagnostics::emitMonitorUpdate(const EventType t, std::string_view monitor_name, double usage, int priority) const {
    const Event event(t, std::format("{}: usage {}%", monitor_name, usage), priority);
    publish(std::move(event));
}
