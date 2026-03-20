//
// Created by nemanja on 12/7/25.
//

#include "Diagnostics.h"
#include <format>

#include "Monitors/CpuMonitor.h"
#include "Monitors/DiskMonitor.h"
#include "Monitors/RamMonitor.h"
#include "utils/TimeUtils.h"
#include "utils/logger/Logger.h"

// static member definitions
std::vector<std::unique_ptr<Monitor>> Diagnostics::monitors_ {};

CpuMonitor* Diagnostics::cpu_monitor_ = nullptr;
RamMonitor* Diagnostics::ram_monitor_ = nullptr;
DiskMonitor* Diagnostics::disk_monitor_ = nullptr;

SnapShot Diagnostics::snapShot_ {};
std::mutex Diagnostics::snapShotMutex_;

void Diagnostics::initMonitors() {
    cpu_monitor_ = registerMonitor(std::make_unique<CpuMonitor>());
    disk_monitor_ = registerMonitor(std::make_unique<DiskMonitor>());
    ram_monitor_ = registerMonitor(std::make_unique<RamMonitor>());
}


void Diagnostics::start() {
    if (running.load()) {
        Logger::warning("Diagnostics already running!");
        return ;
    }

    if (monitors_.empty())
        initMonitors();

    // if the thread already exists - avoid the double start
    // this serves different purposes than atomic running - it manages the code logic
    // whether the whole system should be turned on/off
    // we would set running = false just to indicate the system to shut down and
    // signal the module to join its thread
    if (diagThread.joinable()) {
        Logger::warning("Diagnostics thread already created!");
        return ;
    }

    running = true;

    try {
        diagThread = std::thread(diagnosisLoop);
    } catch (const std::system_error &e) {
        Logger::error(std::format("Failed to create diagnostics thread: {}", e.what()));
        running = false;
        monitors_.clear();
        cpu_monitor_ = nullptr;
        ram_monitor_ = nullptr;
        disk_monitor_ = nullptr;
        return;
    }

    Logger::info("Diagnostics started!");
}

void Diagnostics::stop() {
    if (!running) {
        Logger::warning("Diagnostics stop() called but module is not running!");
        return ;
    }

    running = false;

    if (diagThread.joinable()) {
        diagThread.join();
        Logger::info("Diagnostics stopped!");
    } else {
        Logger::warning("Diagnostics thread not running!");
    }

    monitors_.clear();
    cpu_monitor_ = nullptr;
    disk_monitor_ = nullptr;
    ram_monitor_ = nullptr;
}


void Diagnostics::diagnosisLoop() {
    auto next = std::chrono::steady_clock::now();
    while (running.load()) {
        health_ = DiagHealth::OK;

        // iterate over all monitors to check system health and collect metrics for the snapshot
        for (const auto& monitor : monitors_) {
            Monitor* m_ptr = monitor.get();
            m_ptr->update();

            switch (m_ptr->status()) {
                case MonitorStatus::ERROR:
                    if (m_ptr->isCritical())
                        health_ = DiagHealth::ERROR;
                    else
                        health_ = (health_ == DiagHealth::ERROR) ? health_ : DiagHealth::WARNING;
                    break;
                case MonitorStatus::WARNING:
                    health_ = (health_ == DiagHealth::OK) ? DiagHealth::WARNING : health_;
                    break;
                default:
                    break;
            }
        }

        // building & saving a snapshot
        buildSnapShot();
        writeSnapShotToFile();

        // the Diagnostics system's update interval
        next += std::chrono::milliseconds(diagFrequency);
        std::this_thread::sleep_until(next);
    }
}

DiagHealth Diagnostics::health() {
    return health_;
}

void Diagnostics::setDiagFrequency(int frequency) {
    diagFrequency = frequency;
}

void Diagnostics::buildSnapShot () {
    std::lock_guard<std::mutex> lock(snapShotMutex_);
    snapShot_.cpu_usage = cpu_monitor_->getCpuUsage();
    snapShot_.ram_usage = ram_monitor_->getRamUsage();
    snapShot_.disk_usage = disk_monitor_->getDiskUsage();
    snapShot_.timestamp = TimeUtils::get_time_in_ms();
}

void Diagnostics::writeSnapShotToFile() {
    SnapShot copy {};

    {
        std::lock_guard<std::mutex> lock(snapShotMutex_);
        copy = snapShot_;
    }

    Logger::info(
        "\n================= SNAPSHOT =================\n" +
            SnapShotToString(copy) +
            "\n============================================\n"
        );
}

std::string SnapShotToString(const SnapShot& snap) {
    std::string time_str = TimeUtils::format_time(snap.timestamp);
    return std::format(
        "Time: {}\n CPU: {:.2f}\n RAM: {:.2f}\n DISK: {:.2f}\n",
        time_str, snap.cpu_usage, snap.ram_usage, snap.disk_usage
        );
}

SnapShot Diagnostics::getSnapShot() {
    std::lock_guard<std::mutex> lock(snapShotMutex_);
    return snapShot_;
}
