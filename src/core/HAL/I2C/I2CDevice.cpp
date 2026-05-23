//
// Created by nemanja on 5/23/26.
//

#include "I2CDevice.h"

I2CDevice::I2CDevice(I2CManager &manager, uint8_t address)
    : i2cManager_(manager), address_(address)
{
}

bool I2CDevice::writeRegister8(uint8_t reg, uint8_t value) {
    return i2cManager_.writeRegister8(address_, reg, value);
}

bool I2CDevice::readRegister8(uint8_t reg, uint8_t &value) {
    return i2cManager_.readRegister8(address_, reg, value);
}

bool I2CDevice::writeBytes(const std::vector<uint8_t> &data) {
    return i2cManager_.writeBytes(address_, data);
}

bool I2CDevice::writeByte(uint8_t value) {
    return i2cManager_.writeByte(address_, value);
}

bool I2CDevice::readBytes(std::vector<uint8_t> &buffer) {
    return i2cManager_.readBytes(address_, buffer);
}

bool I2CDevice::readByte(uint8_t &value) {
    return i2cManager_.readByte(address_, value);
}

uint8_t I2CDevice::address() const {
    return address_;
}
