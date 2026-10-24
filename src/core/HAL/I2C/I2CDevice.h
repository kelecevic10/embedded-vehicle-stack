//
// Created by nemanja on 5/23/26.
//

#ifndef EMBEDDED_VEHICLE_STACK_I2CDEVICE_H
#define EMBEDDED_VEHICLE_STACK_I2CDEVICE_H
#include "I2CBus.h"


class I2CDevice {
public:
    I2CDevice(I2CBus& manager, uint8_t address);

    bool writeRegister8(uint8_t reg, uint8_t value);
    bool readRegister8(uint8_t reg, uint8_t& value);

    bool writeBytes(const std::vector<uint8_t>& data);
    bool writeByte(uint8_t value);
    bool readBytes(std::vector<uint8_t>& buffer);
    bool readByte(uint8_t& value);

    [[nodiscard]] uint8_t address() const;
private:
    I2CBus& i2cBus_;
    uint8_t address_;
};


#endif //EMBEDDED_VEHICLE_STACK_I2CDEVICE_H
