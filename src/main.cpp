#include <chrono>
#include <exception>
#include <format>
#include <memory>
#include <thread>

#include "core/System/System.h"
#include "core/Event/Event.h"
#include "core/HAL/Devices/Motor/Motor.h"
#include "utils/logger/Logger.h"
#include "utils/logger/TerminalSink.h"
#include "core/Event/EventTrace.h"

using namespace std::chrono_literals;

namespace {

void publishMotorDrive(
    MessageBus* bus,
    MotorId motorId,
    MotorDirection direction,
    float dutyCycle
) {
    MotorCommandPayload payload {
        .motorId = motorId,
        .direction = direction,
        .dutyCycle = dutyCycle
    };

    bus->publish(std::make_shared<Event>(
        EventType::MOTOR_DRIVE,
        payload,
        10,
        "MotorBringUp"
    ));
}

void publishMotorStop(MessageBus* bus, MotorId motorId) {
    bus->publish(std::make_shared<Event>(
        EventType::MOTOR_STOP,
        motorId,
        10,
        "MotorBringUp"
    ));
}

void waitAfterCommand(std::chrono::milliseconds duration) {
    std::this_thread::sleep_for(duration);
}

} // namespace

int main() {
    Logger::addSink(std::make_unique<TerminalSink>());

    System system;
    Logger logger(system.getBus());

    MessageBus* bus = system.getBus();

    if (!system.start()) {
        Logger::error("System failed to start.");
        return 1;
    }

    Logger::info("System started successfully.");
    Logger::info("Starting PCA PWM motor bring-up test...");

    try {
        Logger::info("PWM test: initial STOP");
        publishMotorStop(bus, MotorId::LEFT);
        waitAfterCommand(1s);

        Logger::info("PWM test: FORWARD dutyCycle=0.25 for 2 seconds");
        publishMotorDrive(bus, MotorId::LEFT, MotorDirection::FORWARD, 0.25f);
        waitAfterCommand(2s);

        Logger::info("PWM test: STOP for 1 second");
        publishMotorStop(bus, MotorId::LEFT);
        waitAfterCommand(1s);

        Logger::info("PWM test: FORWARD dutyCycle=0.50 for 2 seconds");
        publishMotorDrive(bus, MotorId::LEFT, MotorDirection::FORWARD, 0.50f);
        waitAfterCommand(2s);

        Logger::info("PWM test: STOP for 1 second");
        publishMotorStop(bus, MotorId::LEFT);
        waitAfterCommand(1s);

        Logger::info("PWM test: FORWARD dutyCycle=0.75 for 2 seconds");
        publishMotorDrive(bus, MotorId::LEFT, MotorDirection::FORWARD, 0.75f);
        waitAfterCommand(2s);

        Logger::info("PWM test: STOP for 2 seconds before direction change");
        publishMotorStop(bus, MotorId::LEFT);
        waitAfterCommand(2s);

        Logger::info("PWM test: BACKWARD dutyCycle=0.35 for 2 seconds");
        publishMotorDrive(bus, MotorId::LEFT, MotorDirection::BACKWARD, 0.35f);
        waitAfterCommand(2s);

        Logger::info("PWM test: STOP for 1 second");
        publishMotorStop(bus, MotorId::LEFT);
        waitAfterCommand(1s);

        Logger::info("PWM test: BACKWARD dutyCycle=0.60 for 2 seconds");
        publishMotorDrive(bus, MotorId::LEFT, MotorDirection::BACKWARD, 0.60f);
        waitAfterCommand(2s);

        Logger::info("PWM test: final STOP");
        publishMotorStop(bus, MotorId::LEFT);
        waitAfterCommand(1s);

        Logger::info("PCA PWM motor bring-up test finished successfully.");
    }
    catch (const std::exception& ex) {
        Logger::error(std::format(
            "PCA PWM motor bring-up test failed: {}",
            ex.what()
        ));

        Logger::warning("Publishing emergency MOTOR_STOP event.");
        publishMotorStop(bus, MotorId::LEFT);
        waitAfterCommand(500ms);

        system.stop();
        return 1;
    }
    catch (...) {
        Logger::error("PCA PWM motor bring-up test failed with unknown exception.");

        Logger::warning("Publishing emergency MOTOR_STOP event.");
        publishMotorStop(bus, MotorId::LEFT);
        waitAfterCommand(500ms);

        system.stop();
        return 1;
    }

    Logger::info("Stopping system...");

    if (!system.stop()) {
        Logger::error("System failed to stop cleanly.");
        return 1;
    }

    Logger::info("System stopped.");
    return 0;
}