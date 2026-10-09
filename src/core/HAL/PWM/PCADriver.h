//
// Created by nemanja on 5/23/26.
//

#ifndef EMBEDDED_VEHICLE_STACK_PCADRIVER_H
#define EMBEDDED_VEHICLE_STACK_PCADRIVER_H
#include "core/HAL/I2C/I2CDevice.h"


class PCADriver {
public:
    PCADriver(I2CDevice device);

    bool init();
    bool shutdown();

    bool setPwmFrequency(float frequency);

    bool setDutyCycle(std::uint8_t channel, float dutyCycle);

    bool forceLow(std::uint8_t channel);
    bool forceHigh(std::uint8_t channel);

    [[nodiscard]] bool isInitialized() const;
    [[nodiscard]] float frequency() const;

private:
    static bool isValidChannel(std::uint8_t channel);
    static float clampDutyCycle(float dutyCycle);

    bool setPwm(std::uint8_t channel, std::uint16_t onCount, std::uint16_t offCount);
private:
    I2CDevice device_;
    bool initialized_ {false};
    float frequency_ {50.0f};   // Hz
};


#endif //EMBEDDED_VEHICLE_STACK_PCADRIVER_H
