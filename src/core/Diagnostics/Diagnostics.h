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

#include "core/Diagnostics/Monitors/Monitor.h"
#include "core/MessageBus/MessageBus.h"
#include "core/System/Subsystem.h"
#include "Monitors/CpuMonitor.h"
#include "Monitors/DiskMonitor.h"
#include "Monitors/RamMonitor.h"

#include "utils/enums/DiagTypes.h"


std::string SnapShotToString(const SnapShot&);

class Diagnostics : public Subsystem {
public:
    Diagnostics(MessageBus* bus)
        : messageBus_(bus),
        diagFrequency(3000),
        monitors_{},
        running(false),
        cpu_monitor_(nullptr),
        ram_monitor_(nullptr),
        disk_monitor_(nullptr),
        snapShot_{},
        health_(DiagHealth::OK)
    {
        initMonitors();
    }

    ~Diagnostics() override;

    // Subsystem api
    bool start() override;
    bool stop() override;
    std::string_view name() const override { return "Diagnostics"; };

    DiagHealth health() const;
    void setDiagFrequency(int frequency);
    SnapShot getSnapShot();

    template<typename T>
    T* registerMonitor(std::unique_ptr<T> monitor) {
        T* raw_ptr = monitor.get();
        monitors_.push_back(std::move(monitor));
        return raw_ptr;
    }
private:
    // class fields
    MessageBus* messageBus_;
    std::atomic<bool> running;
    int diagFrequency;

    std::vector<std::unique_ptr<Monitor>> monitors_;
    CpuMonitor* cpu_monitor_;
    RamMonitor* ram_monitor_;
    DiskMonitor* disk_monitor_;

    SnapShot snapShot_;
    std::mutex snapShotMutex_;

    DiagHealth health_;
    std::jthread diagThread;

    // methods
    void diagnosisLoop();
    void buildSnapShot();
    void initMonitors();
    void emitEvent(EventType, std::string_view, double, int) const;
};


#endif //EMBEDDED_VEHICLE_STACK_DIAGNOSTICS_H