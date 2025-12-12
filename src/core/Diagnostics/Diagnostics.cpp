//
// Created by nemanja on 12/7/25.
//

#include "Diagnostics.h"

#include <format>

#include "utils/TimeUtils.h"
#include "utils/logger/Logger.h"


void Diagnostics::start() {
    if (running) {
        Logger::warning("Diagnostics already running!");
        return ;
    }

    // if the thread already exists and is joinable - avoid the double start
    if (diagThread.joinable()) {
        Logger::warning("Diagnostics thread already created but still running!");
        return ;
    }

    running = true;
    diagThread = std::thread(diagnosisLoop);

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
}

void Diagnostics::buildSnapShot (
    const double cpu_usage, const double ram_usage,
    const double disk_usage, const double cpu_temp,
    const double network_rtt
    ) {
    std::lock_guard<std::mutex> lock(snapShotMutex);

    snapShot.cpu_usage = cpu_usage;
    snapShot.ram_usage = ram_usage;
    snapShot.disk_usage = disk_usage;
    snapShot.cpu_temp = cpu_temp;
    snapShot.network_rtt = network_rtt;
    snapShot.timestamp = TimeUtils::get_time_in_ms();
}

void Diagnostics::diagnosisLoop() {
    auto next = std::chrono::steady_clock::now();
    while (running.load(std::memory_order_relaxed)) {
        double cpu_usage = updateCPU();
        double ram_usage = updateRam();
        double disk_usage = updateDisk();
        double cpu_temp = updateTemp();
        double network_stats = updateNetwork();

        buildSnapShot(cpu_usage, ram_usage, disk_usage, cpu_temp, network_stats);
        writeSnapShotToFile();

        next += std::chrono::milliseconds(sleepingTime);
        std::this_thread::sleep_until(next);
    }
}

void Diagnostics::setDiagFrequency(int frequency) {
    sleepingTime = frequency;
}

void Diagnostics::writeSnapShotToFile() {
    SnapShot copy;

    {
        std::lock_guard<std::mutex> lock(snapShotMutex);
        copy = snapShot;
    }

    Logger::info("================= SNAPSHOT =================");
    Logger::info(SnapShotToString(copy));
}

std::string SnapShotToString(const SnapShot& snap) {
    std::string time_str = TimeUtils::format_time(snap.timestamp);
    return std::format(
        "TIME: {} CPU: {:.2f} RAM: {:.2f} DISK: {:.2f} CPU_TEMP: {:.2f} RTT: {:.2f}",
        time_str, snap.cpu_usage, snap.ram_usage, snap.disk_usage, snap.cpu_temp, snap.network_rtt
        );

}

bool Diagnostics::isHealthy() {
    // maybe warning is ok too
    // TODO: the * implementation here
    return health == DiagHealth::OK;
}

SnapShot Diagnostics::getSnapShot() {
    return snapShot;
}