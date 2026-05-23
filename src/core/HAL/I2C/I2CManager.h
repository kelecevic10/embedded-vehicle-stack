//
// Created by nemanja on 5/23/26.
//

#ifndef EMBEDDED_VEHICLE_STACK_I2CMANAGER_H
#define EMBEDDED_VEHICLE_STACK_I2CMANAGER_H
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>


class I2CManager {
public:
    explicit I2CManager(std::string devicePath);
    ~I2CManager();

    I2CManager(const I2CManager&) = delete;
    I2CManager& operator=(const I2CManager&) = delete;
    I2CManager(I2CManager&&) = delete;
    I2CManager& operator=(I2CManager&&) = delete;

    bool openBus();
    bool closeBus();

    [[nodiscard]] bool isOpen();
    [[nodiscard]] const std::string& devicePath() const;

    bool writeByte(std::uint8_t address, std::uint8_t value);
    bool writeRegister8(std::uint8_t address, std::uint8_t reg, std::uint8_t value);
    bool writeBytes(std::uint8_t address, const std::vector<std::uint8_t>& data);

    bool readByte(std::uint8_t address, std::uint8_t& value);
    bool readRegister8(std::uint8_t address, std::uint8_t reg, std::uint8_t& value);
    bool readBytes(std::uint8_t address, std::vector<std::uint8_t>& buffer);

private:
    bool selectDevice(std::uint8_t address);

private:
    std::string devicePath_;
    int fd_ {-1};

    // One physical I2C bus can be used by multiple devices.
    // this mutex prevents 2 threads from mixing I2C transactions.
    std::mutex mutex_;
};


#endif //EMBEDDED_VEHICLE_STACK_I2CMANAGER_H
