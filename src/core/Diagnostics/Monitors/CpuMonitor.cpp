//
// Created by nemanja on 12/17/25.
//

#include "CpuMonitor.h"

#include <chrono>
#include <fstream>
#include <thread>

#include "core/Diagnostics/Diagnostics.h"
#include "utils/logger/Logger.h"

CpuMonitor::CpuMonitor() : cpuUsage_()
{
    this->prevTimes_ = {};
    this->currTimes_ = readCpuTimes();
}


void CpuMonitor::update()
{
    prevTimes_ = currTimes_;
    currTimes_ = readCpuTimes();
    cpuUsage_ = computeCpuUsage(prevTimes_, currTimes_);
}

std::string_view CpuMonitor::getName() const
{
    return "CPU Monitor";
}

bool CpuMonitor::isCritical() const
{
    return true;
}

MonitorStatus CpuMonitor::status()
{
    if (cpuUsage_ > 90)
        monitorStatus = MonitorStatus::ERROR;
    else if (cpuUsage_ > 80)
        monitorStatus = MonitorStatus::WARNING;
    else
        monitorStatus = MonitorStatus::OK;
    return monitorStatus;
}

CpuTimes CpuMonitor::readCpuTimes() {
    std::ifstream file("/proc/stat");
    if (!file.is_open()) {
        Logger::error("CPU Monitor:: Could not open /proc/stat");
        monitorStatus = MonitorStatus::ERROR;
        return {};
    }

    std::string cpu;
    CpuTimes t {};
    file >> cpu >> t.user >> t.nice >> t.system >> t.idle >> t.iowait >> t.irq >> t.softirq >> t.steal;
    return t;
}


// This function computes the CPU usage percentage by sampling two consecutive
// readings of cumulative CPU time from /proc/stat. The Linux kernel exposes
// monotonically increasing counters representing how many jiffies the CPU has
// spent in various modes since boot. We classify these into IDLE time
// (idle + iowait) and NON-IDLE time (user, nice, system, irq, softirq, steal).
// By taking the difference between the current and previous samples, we obtain
// how much total CPU time has passed in the interval (totald) and how much of
// that time the CPU was idle (idled). The CPU usage in that interval is then
// computed as (totald - idled) / totald, representing the fraction of time the
// CPU was actually working rather than idle.
double CpuMonitor::computeCpuUsage(const CpuTimes& before, const CpuTimes& current) {
    const uint64_t idlePrev = before.idle + before.iowait;
    const uint64_t idleCurr = current.idle + current.iowait;

    const uint64_t nonIdlePrev = before.user + before.nice + before.system + before.irq + before.softirq + before.steal;
    const uint64_t nonIdleCurr = current.user + current.nice + current.system + current.irq + current.softirq + current.steal;

    const uint64_t totalPrev = idlePrev + nonIdlePrev;
    const uint64_t totalCurr = idleCurr + nonIdleCurr;

    // calculate deltas for the interval
    const uint64_t total_delta = totalCurr - totalPrev;
    const uint64_t idle_delta = idleCurr - idlePrev;

    if (total_delta == 0) return 0.0;

    // usage = (active_time) / (total time)
    const double cpuUsage = (static_cast<double>(total_delta - idle_delta) / static_cast<double>(total_delta)) * 100.0;
    return cpuUsage;
}