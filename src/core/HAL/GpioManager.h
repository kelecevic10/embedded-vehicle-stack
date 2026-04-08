//
// Created by nemanja on 4/8/26.
//

#ifndef EMBEDDED_VEHICLE_STACK_GPIOMANAGER_H
#define EMBEDDED_VEHICLE_STACK_GPIOMANAGER_H

#include <gpiod.hpp>

#include <optional>
#include <string>
#include <unordered_map>

namespace hal {
    enum class PinDirection {
        INPUT,
        OUTPUT,
    };

    enum class PinState {
        LOW = 0,
        HIGH = 1,
    };

    enum class PinBias {
        NONE,
        PULL_UP,
        PULL_DOWN
    };

    struct OutputPinConfig {
        unsigned int pin;
        std::string consumer;
        bool activeLow {false};
        PinState initialValue {PinState::LOW};
    };

    struct InputPinConfig {
        unsigned int pin;
        std::string consumer;
        bool activeLow {false};
        PinBias bias {PinBias::NONE};
    };

    class GpioManager {
    public:
        explicit GpioManager(std::string chipPath);
        ~GpioManager();
        // Copy and move constructors are deleted because
        // GpioManager manages critical resources
        GpioManager(const GpioManager&) = delete;
        GpioManager& operator=(const GpioManager&) = delete;
        GpioManager(GpioManager&&) = delete;
        GpioManager& operator=(GpioManager&&) = delete;

        bool claimPin(const InputPinConfig&);
        bool claimPin(const OutputPinConfig&);
        void releasePin(unsigned int pin);
        void releaseAllPins();

        bool write(unsigned int pin, PinState value);
        std::optional<PinState> read(unsigned int pin);

        const std::string& chipPath() const;
        std::size_t claimedPinCount() const;
        bool isClaimed(unsigned int pin) const;
        std::optional<PinDirection> getPinDirection(unsigned int pin) const;

    private:
        struct ClaimedPin {
            PinDirection direction;
            bool activeLow {false};
            std::string consumer;
            gpiod::line_request request;
        };
        const ClaimedPin* findPin(unsigned int pin) const;
        ClaimedPin* findPin(unsigned int pin);

        std::string chipPath_;
        gpiod::chip chip_;
        std::unordered_map<unsigned int, ClaimedPin> claimedPins_;
    };
}





#endif //EMBEDDED_VEHICLE_STACK_GPIOMANAGER_H
