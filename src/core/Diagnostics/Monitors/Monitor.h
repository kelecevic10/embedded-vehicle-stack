//
// Created by nemanja on 12/17/25.
//

#ifndef EMBEDDED_VEHICLE_STACK_MONITOR_H
#define EMBEDDED_VEHICLE_STACK_MONITOR_H

#pragma once

#include <string_view>
#include <ostream>

enum class MonitorStatus {
    OK,
    WARNING,
    ERROR
};

inline std::string_view toString(const MonitorStatus status) {
    switch (status) {
        case MonitorStatus::OK: return "OK";
        case MonitorStatus::WARNING: return "WARNING";
        case MonitorStatus::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

inline std::ostream &operator<<(std::ostream &os, const MonitorStatus &status) {
    return os << toString(status);
}

class Monitor {
public:
    Monitor(): monitorStatus(MonitorStatus::OK) {}
    virtual ~Monitor() = default;

    // called periodically by the Diagnostics loop
    // monitor refresh its internal state here
    virtual void update() = 0;

    // write relevant information to the corresponding snap_shot's fields
    // virtual void populateSnapShot(SnapShot& snap_shot) = 0;

    // expose the human-readable Monitor name as a reference
    virtual std::string_view getName() const = 0;

    // Diag checks whether a monitor's state has a huge impact on system health
    virtual bool isCritical() const = 0;

    // each monitor should provide double usage
    virtual double getUsage() const = 0;

    // monitor can expose internal health state to the other modules
    virtual MonitorStatus status() = 0;
    // if a monitor encounters a fatal failure, it can report
    // error details here for diagnostics to log
    // virtual std::optional<std::string> getLastError() const = 0;

    // set the refresh interval (in milliseconds) at which this monitor should update its state
    // virtual void setRefreshInterval(int interval) {refreshInterval = interval;}

protected:
    MonitorStatus monitorStatus;
private:

};

#endif //EMBEDDED_VEHICLE_STACK_MONITOR_H