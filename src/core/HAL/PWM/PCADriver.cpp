//
// Created by nemanja on 5/23/26.
//

#include "PCADriver.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <format>

#include "utils/logger/Logger.h"

namespace {

/*
 * PCA9685 register addresses.
 *
 * These values come from the PCA9685 datasheet.
 * The PCA chip is controlled by writing values into these registers over I2C.
 */

// Main mode register.
// Controls sleep/restart/all-call/auto-increment behavior.
constexpr std::uint8_t MODE1 = 0x00;

// Output mode register.
// Controls electrical behavior of output pins.
constexpr std::uint8_t MODE2 = 0x01;

// First PWM register for channel 0.
// Every channel uses 4 registers:
// LEDn_ON_L, LEDn_ON_H, LEDn_OFF_L, LEDn_OFF_H.
constexpr std::uint8_t LED0_ON_L = 0x06;

// Prescale register.
// Used to configure global PWM frequency.
constexpr std::uint8_t PRESCALE = 0xFE;

/*
 * MODE1 bit masks.
 */

// Restart PWM logic after sleep/frequency change.
constexpr std::uint8_t MODE1_RESTART = 0x80;

// Put oscillator into sleep mode.
// Required before changing PRESCALE.
constexpr std::uint8_t MODE1_SLEEP = 0x10;

// Respond to all-call I2C address.
// Not critical for us, but common default setup.
constexpr std::uint8_t MODE1_ALLCALL = 0x01;

/*
 * MODE2 bit masks.
 */

// Use totem-pole output driver.
// Better for clear HIGH/LOW PWM signal output.
constexpr std::uint8_t MODE2_OUTDRV = 0x04;

/*
 * PCA9685 timing constants.
 */

// Typical internal oscillator frequency of PCA9685.
constexpr float PCA_OSCILLATOR_HZ = 25'000'000.0f;

// 12-bit PWM resolution: 2^12 = 4096.
constexpr float PCA_PWM_RESOLUTION = 4096.0f;

// PCA9685 has 16 channels: 0-15.
constexpr std::uint8_t PCA_CHANNEL_COUNT = 16;

constexpr std::uint8_t MODE1_AUTOINC = 0x20;

} // namespace

PCADriver::PCADriver(I2CDevice device)
    : device_(std::move(device))
{
}

bool PCADriver::isInitialized() const {
    return initialized_;
}

float PCADriver::frequency() const {
    return frequency_;
}

bool PCADriver::isValidChannel(std::uint8_t channel) {
    return channel < PCA_CHANNEL_COUNT;
}

float PCADriver::clampDutyCycle(float dutyCycle) {
    return std::clamp(dutyCycle, 0.0f, 1.0f);
}


bool PCADriver::init() {
    if (initialized_) {
        Logger::warning("PCA9685 already initialized!");
        return true;
    }

    Logger::info("Initializing PCA9685 PWM controller...");
    if (!device_.writeRegister8(MODE1, MODE1_ALLCALL | MODE1_AUTOINC)) {
        Logger::error("Failed to write MODE1 register during init");
        return false;
    }

    if (!device_.writeRegister8(MODE2, MODE2_OUTDRV)) {
        Logger::error("Failed to write MODE2 register during init");
        return false;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    if (!setPwmFrequency(frequency_)) {
        Logger::error("Failed to set default PWM frequency during init");
        return false;
    }

    for (std::uint8_t channel = 0; channel < PCA_CHANNEL_COUNT; channel++) {
        if (!forceLow(channel)) {
            Logger::error(std::format(
                "PCADriver: failed to force channel {} LOW during init",
                channel
            ));
            return false;
        }
    }

    initialized_ = true;
    Logger::info("PCA9685 initialized successfully");
    return true;
}

bool PCADriver::shutdown() {
    if (!initialized_) {
        return true;
    }

    Logger::info("Shutting down PCA9685...");

    bool valid = true;
    for (std::uint8_t channel = 0; channel < PCA_CHANNEL_COUNT; channel++) {
        valid &= forceLow(channel);
    }

    if (!valid) {
        Logger::error(" PCA9685: Failed to force all channels LOW during shutdown");
        return false;
    }

    Logger::info("PCA9685 shutdown successfully");
    initialized_ = false;
    return true;
}

bool PCADriver::setPwmFrequency(float frequency) {

    if (frequency <= 0.0f) {
        Logger::error("PCA9685: invalid PWM frequency");
        return false;
    }

    std::uint8_t oldMode = 0;
    if (!device_.readRegister8(MODE1, oldMode)) {
        Logger::error("PCA9685: failed to read MODE1 register before setting PWM frequency!");
        return false;
    }

    const float prescaleValue =
        (PCA_OSCILLATOR_HZ / (PCA_PWM_RESOLUTION * frequency)) - 1.0f;

    const auto prescale = static_cast<std::uint8_t>(
        std::round(prescaleValue)
    );

    const std::uint8_t sleepMode = static_cast<std::uint8_t>(
        (oldMode & ~MODE1_RESTART) | MODE1_SLEEP
    );

    if (!device_.writeRegister8(MODE1, sleepMode)) {
        Logger::error("PCA9685: failed to write MODE1 register during PWM frequency change!");
        return false;
    }

    if (!device_.writeRegister8(PRESCALE, prescale)) {
        Logger::error("PCA9685: failed to write PRESCALE register during PWM frequency change!");
        return false;
    }

    if (!device_.writeRegister8(MODE1, oldMode)) {
        Logger::error("PCA9685: failed to write MODE1 register after PWM frequency change!");
        return false;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    const std::uint8_t restartMode = static_cast<std::uint8_t>(
        oldMode | MODE1_RESTART
    );

    if (!device_.writeRegister8(MODE1, restartMode)) {
        Logger::error("PCA9685: failed to write MODE1 register during PWM frequency change!");
        return false;
    }

    frequency_ = frequency;

    Logger::info(std::format(
        "PCA9685: set PWM frequency to {} Hz",
        frequency
    ));

    return true;
}

bool PCADriver::setDutyCycle(std::uint8_t channel, float dutyCycle) {
    if (!isValidChannel(channel)) {
        Logger::error(std::format(
            "PCA9685: invalid channel {} specified for duty cycle",
            channel
        ));
        return false;
    }

    const float clampedDutyCycle = clampDutyCycle(dutyCycle);
    if (clampedDutyCycle != dutyCycle) {
        Logger::warning(std::format(
           "PCA9685: duty cycle {:.2f} out of range, clamped to {:.2f}",
           dutyCycle, clampedDutyCycle
        ));
    }

    if (clampedDutyCycle == 0.0f) {
        return forceLow(channel);
    }

    if (clampedDutyCycle == 1.0f) {
        return forceHigh(channel);
    }

    const auto offCount = static_cast<std::uint16_t>(
        std::round(clampedDutyCycle * (PCA_PWM_RESOLUTION - 1.0f))
    );

    return setPwm(channel, 0, offCount);
}

bool PCADriver::forceLow(std::uint8_t channel) {
    if (!isValidChannel(channel)) {
        Logger::error(std::format(
            "PCA9685: invalid channel {} specified for LOW",
            channel
        ));
        return false;
    }

    const std::uint8_t baseReg = static_cast<std::uint8_t>(
        LED0_ON_L + channel * 4
    );

    return device_.writeBytes({
        baseReg,
        0x00,
        0x00,
        0x00,
        0x10      // full OFF bit
    });
}

bool PCADriver::forceHigh(std::uint8_t channel) {
    if (!isValidChannel(channel)) {
        Logger::error(std::format(
            "PCA9685: invalid channel {} specified for forceHigh",
            channel
        ));
        return false;
    }

    const std::uint8_t baseReg = static_cast<std::uint8_t>(
        LED0_ON_L + 4 * channel
    );

    return device_.writeBytes({
        baseReg,
        0x00, 0x10,
        0x00, 0x00
    });
}

bool PCADriver::setPwm(std::uint8_t channel, std::uint16_t onCount, std::uint16_t offCount) {
    if (!isValidChannel(channel)) {
        Logger::error(std::format(
            "PCA9685: invalid channel {} specified for PWM",
            channel
        ));
        return false;
    }

    onCount = std::min<std::uint16_t>(onCount, 4095);
    offCount = std::min<std::uint16_t>(offCount, 4095);

    const std::uint8_t baseReg = static_cast<std::uint8_t>(
        LED0_ON_L + channel * 4
    );

    const std::uint8_t onLow = static_cast<std::uint8_t>(onCount & 0xFF);
    const std::uint8_t onHigh = static_cast<std::uint8_t>((onCount >> 8) & 0x0F);

    const std::uint8_t offLow = static_cast<std::uint8_t>(offCount & 0xFF);
    const std::uint8_t offHigh = static_cast<std::uint8_t>((offCount >> 8) & 0x0F);

    return device_.writeBytes({
        baseReg,
        onLow,
        onHigh,
        offLow,
        offHigh
    });
}

