//
// Created by nemanja on 12/7/25.
//

#ifndef EMBEDDED_VEHICLE_STACK_DIAGNOSTICS_H
#define EMBEDDED_VEHICLE_STACK_DIAGNOSTICS_H
#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "utils/enums/enums.h"
#include "core/Diagnostics/Monitors/Monitor.h"
#include "Monitors/CpuMonitor.h"
#include "Monitors/DiskMonitor.h"
#include "Monitors/RamMonitor.h"

// global health of all registered monitors
enum struct DiagHealth {
    OK,
    WARNING,
    ERROR
};

std::string SnapShotToString(const SnapShot&);

class Diagnostics {
public:
    Diagnostics() = delete;

    // public module api
    static void start();
    static void stop();
    static DiagHealth health();

    static void setDiagFrequency(int frequency);
    static void writeSnapShotToFile();
    static SnapShot getSnapShot();

    template<typename T>
    static T* registerMonitor(std::unique_ptr<T> monitor) {
        T* raw_ptr = monitor.get();
        monitors_.push_back(std::move(monitor));
        return raw_ptr;
    }
private:
    // used to decide whether the diag should be active
    static inline std::atomic<bool> running;
    static inline int diagFrequency {3000};   // ms
    static inline std::thread diagThread;
    static void diagnosisLoop();
    static void buildSnapShot();
    static void initMonitors();

    // class fields
    static std::vector<std::unique_ptr<Monitor>> monitors_;
    static CpuMonitor* cpu_monitor_;
    static RamMonitor* ram_monitor_;
    static DiskMonitor* disk_monitor_;

    static SnapShot snapShot_;
    static std::mutex snapShotMutex_;

    static inline DiagHealth health_ {DiagHealth::OK};
};


#endif //EMBEDDED_VEHICLE_STACK_DIAGNOSTICS_H