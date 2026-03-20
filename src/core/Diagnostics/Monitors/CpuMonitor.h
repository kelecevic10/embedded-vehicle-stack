//
// Created by nemanja on 12/17/25.
//

#ifndef EMBEDDED_VEHICLE_STACK_CPUMONITOR_H
#define EMBEDDED_VEHICLE_STACK_CPUMONITOR_H
#include "Monitor.h"


/** CPU_TIMES:
 * user: CPU time spent in user mode executing user processes.
 * nice: CPU time spent in user mode with low-priority (nice) processes.
 * system: CPU time spent in kernel mode executing system calls and kernel tasks.
 * idle: CPU time spent idle with no processes waiting to run.
 * iowait: CPU time spent waiting for I/O operations to complete.
 * irq: CPU time spent servicing hardware interrupts.
 * softirq: CPU time spent servicing software interrupts.
 * steal: CPU time stolen by the hypervisor for other virtual machines.
 * each time is measured in special units - jiffies since BOOT
 */
struct CpuTimes {
    uint64_t user, nice, system, idle, iowait, irq, softirq, steal;
};

class CpuMonitor : public Monitor {
public:
    ~CpuMonitor() override = default;
    CpuMonitor();

    // getter
    double getCpuUsage() const { return cpuUsage; }

    // updates monitors internal state
    void update() override;

    // void populateSnapShot(SnapShot &snap_shot) override;
    std::string_view getName() const override;

    // Monitor Status & Importance
    bool isCritical() const override;
    MonitorStatus status() override;

private:
    // helper functions
    CpuTimes readCpuTimes();
    static double computeCpuUsage(const CpuTimes&, const CpuTimes&);

    // cpuUsage stats
    CpuTimes prevTimes {};
    CpuTimes currTimes {};
    double cpuUsage = 0;
};


#endif //EMBEDDED_VEHICLE_STACK_CPUMONITOR_H