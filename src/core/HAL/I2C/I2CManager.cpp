//
// Created by nemanja on 5/23/26.
//

#include "I2CManager.h"
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include "utils/logger/Logger.h"

I2CManager::I2CManager(std::string devicePath)
    : devicePath_(std::move(devicePath))
{
}

I2CManager::~I2CManager() {
    closeBus();
}

bool I2CManager::openBus() {
    std::scoped_lock lock(mutex_);

    if (fd_ >= 0) {
        Logger::warning("I2C bus already open!");
        return true;
    }

    // :: for global namespace
    fd_ = ::open(devicePath_.c_str(), O_RDWR);
    if (fd_ < 0) {
        Logger::error(std::format("Failed to open I2C bus: {}", devicePath_));
        return false;
    }

    Logger::info(std::format("Opened I2C bus successfully: {}", devicePath_));
    return true;
}

bool I2CManager::closeBus() {
    std::scoped_lock lock(mutex_);

    if (fd_ < 0) {
        return true;
    }

    if (::close(fd_) < 0) {
        Logger::error("Failed to close I2C bus!");
        fd_ = -1;
        return false;
    }

    fd_ = -1;
    Logger::info("I2CManager: Closed I2C bus successfully!");
    return true;
}

bool I2CManager::isOpen() {
    std::scoped_lock lock(mutex_);
    return fd_ >= 0;
}

const std::string& I2CManager::devicePath() const {
    return devicePath_;
}

bool I2CManager::selectDevice(std::uint8_t address) {
    if (fd_ < 0) {
        Logger::error("I2CManager: cannot select device because bus is not open.");
        return false;
    }

    if (::ioctl(fd_, I2C_SLAVE, address) < 0) {
        Logger::error(std::format(
            "I2CManager: failed to select device at address 0x{:20X}",
            address
        ));
        return false;
    }
    return true;
}

bool I2CManager::writeByte(std::uint8_t address, std::uint8_t value) {
    std::scoped_lock lock(mutex_);

    if (!selectDevice(address)) {
        return false;
    }

    const std::uint8_t buffer[1] = { value };
    if (::write(fd_, buffer, 1) != 1) {
        Logger::error(std::format(
            "I2CManager: failed to write byte to device at address 0x{:20X}",
            address
        ));
        return false;
    }

    return true;
}

bool I2CManager::writeRegister8(std::uint8_t address, std::uint8_t reg, std::uint8_t value) {
    std::scoped_lock lock(mutex_);

    if (!selectDevice(address)) {
        return false;
    }

    const std::uint8_t buffer[2] = { reg, value };
    if (::write(fd_, buffer, 2) != 2) {
        Logger::error(std::format(
            "I2CManager: failed to write value 0x{:02X} to register 0x{:02X} to device at address 0x{:02X}",
            value, reg, address
        ));
        return false;
    }

    return true;
}

bool I2CManager::writeBytes(std::uint8_t address, const std::vector<std::uint8_t> &data) {

    if (data.empty()) {
        Logger::warning("I2CManager: writeBytes called with empty data");
        return true;
    }

    std::scoped_lock lock(mutex_);

    if (!selectDevice(address)) {
        return false;
    }

    const auto bytesWritten = ::write(fd_, data.data(), data.size());
    if (bytesWritten != static_cast<ssize_t>(data.size())) {
        Logger::error(std::format(
            "I2CManager: failed to write {} bytes to device at address 0x{:20X}",
            data.size(), address
        ));
        return false;
    }

    return true;
}

bool I2CManager::readByte(std::uint8_t address, std::uint8_t &value) {
    std::scoped_lock lock(mutex_);

    if (!selectDevice(address)) {
        return false;
    }

    if (::read(fd_, &value, 1) != 1) {
        Logger::error(std::format(
            "I2CManager: failed to read byte from device at address 0x{:20X}",
            address
        ));
        return false;
    }

    return true;
}

bool I2CManager::readRegister8(std::uint8_t address, std::uint8_t reg, std::uint8_t &value) {
    std::scoped_lock lock(mutex_);

    if (!selectDevice(address)) {
        return false;
    }

    // first write the register address we want to read from
    if (::write(fd_, &reg, 1) != 1) {
        Logger::error(std::format(
            "I2CManager: failed to write register address 0x{:02X} to device at address 0x{:02X}",
            reg, address
        ));
        return false;
    }

    // then read one byte from that register
    if (::read(fd_, &value, 1) != 1) {
        Logger::error(std::format(
            "I2CManager: failed to read byte from device at address 0x{:20X}",
            address
        ));
        return false;
    }

    return true;
}

bool I2CManager::readBytes(std::uint8_t address, std::vector<std::uint8_t> &buffer) {
    if (buffer.empty()) {
        Logger::warning("I2CManager: readBytes called with empty buffer");
        return true;
    }

    std::scoped_lock lock(mutex_);

    if (!selectDevice(address)) {
        return false;
    }

    const auto bytesRead = ::read(fd_, buffer.data(), buffer.size());
    if (bytesRead != static_cast<ssize_t>(buffer.size())) {
        Logger::error(std::format(
            "I2CManager: failed to read {} bytes from device at address 0x{:20X}",
            buffer.size(), address
        ));
        return false;
    }

    return true;
}


