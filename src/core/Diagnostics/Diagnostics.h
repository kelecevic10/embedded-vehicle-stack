//
// Created by nemanja on 12/7/25.
//

#ifndef EMBEDDED_VEHICLE_STACK_DIAGNOSTICS_H
#define EMBEDDED_VEHICLE_STACK_DIAGNOSTICS_H
#include <atomic>
#include <memory>
#include <string>
#include <thread>

enum struct DiagHealth {
    OK,
    WARNING,
    ERROR
};

struct SnapShot {
    double cpu_usage;
    double ram_usage;
    double disk_usage;
    double cpu_temp;
    double network_rtt;

    uint64_t timestamp;
};

std::string SnapShotToString(const SnapShot&);

class Diagnostics {
public:
    Diagnostics() = delete;
    ~Diagnostics() = delete;

    static void start();
    static void stop();
    static void setDiagFrequency(int frequency);
    static void writeSnapShotToFile();

    static SnapShot getSnapShot();

    // TODO: Define health thresholds for snapshot metrics. *
    static bool isHealthy();

    static SnapShot snapShot;
    static std::mutex snapShotMutex;

    static inline DiagHealth health {DiagHealth::OK};
private:
    static inline std::atomic<bool> running {false};
    static inline int sleepingTime {500};   // ms
    static inline std::thread diagThread;

    static void diagnosisLoop();
    static void buildSnapShot(double, double, double, double, double);
};


#endif //EMBEDDED_VEHICLE_STACK_DIAGNOSTICS_H