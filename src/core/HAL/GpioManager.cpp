//
// Created by nemanja on 4/8/26.
//

#include "GpioManager.h"

#include "utils/logger/Logger.h"

namespace hal {
    namespace {
        gpiod::line::value toGpiodValue(const PinState state) {
            return state == PinState::HIGH
                ? gpiod::line::value::ACTIVE
                : gpiod::line::value::INACTIVE;
        }

        PinState fromGpiodValue(const gpiod::line::value value) {
            return value == gpiod::line::value::ACTIVE
                ? PinState::HIGH
                : PinState::LOW;
        }

        gpiod::line::bias toGpiodBias(const PinBias bias) {
            switch (bias) {
                case PinBias::AS_IS:
                    return gpiod::line::bias::AS_IS;
                case PinBias::PULL_UP:
                    return gpiod::line::bias::PULL_UP;
                case PinBias::PULL_DOWN:
                    return gpiod::line::bias::PULL_DOWN;
            }

            return gpiod::line::bias::AS_IS;
        }
    }
}

using namespace hal;

GpioManager::GpioManager(std::string chipPath)
    : chipPath_(std::move(chipPath)), chip_(chipPath_)
{

}

GpioManager::~GpioManager()
{
    releaseAllPins();
}

bool GpioManager::claimPin(const InputPinConfig& config) {
    if (isClaimed(config.pin)) {
        return false;
    }
    try {
        gpiod::line_settings settings;
        settings
        .set_direction(gpiod::line::direction::INPUT)
        .set_active_low(config.activeLow)
        .set_bias(toGpiodBias(config.bias));

        gpiod::line_config lineConf;
        lineConf.add_line_settings(config.pin, settings);

        gpiod::request_config requestConf;
        requestConf.set_consumer(config.consumer);

        auto request = chip_
            .prepare_request()
            .set_request_config(requestConf)
            .set_line_config(lineConf)
            .do_request();

        claimedPins_.emplace(config.pin, ClaimedPin{
            PinDirection::INPUT,
            config.activeLow,
            config.consumer,
            (std::move(request))}
            );

        return true;
    } catch (...) {
        return false;
    }
}

bool GpioManager::claimPin(const OutputPinConfig& config) {
    if (isClaimed(config.pin)) {
        return false;
    }
    try {
        gpiod::line_settings settings;
        settings
        .set_direction(gpiod::line::direction::OUTPUT)
        .set_active_low(config.activeLow)
        .set_output_value(toGpiodValue(config.initialValue));

        gpiod::line_config lineConf;
        lineConf.add_line_settings(config.pin, settings);

        gpiod::request_config requestConf;
        requestConf.set_consumer(config.consumer);

        auto request = chip_
            .prepare_request()
            .set_request_config(requestConf)
            .set_line_config(lineConf)
            .do_request();

        claimedPins_.emplace(config.pin, ClaimedPin{
            PinDirection::OUTPUT,
            config.activeLow,
            config.consumer,
            (std::move(request))}
            );

        return true;
    } catch (...) {
        return false;
    }
}

void GpioManager::releasePin(unsigned int pin) {
    const auto it = claimedPins_.find(pin);
    if (it == claimedPins_.end()) {
        return;
    }

    try {
        gpiod::line_request& lineRequest = it->second.request;
        if (!lineRequest) {
            Logger::warning(std::format(
                "Pin {} is tracked in claimedPins_, but its line_request is invalid",
                pin
            ));
        } else {
            lineRequest.release();
        }
        claimedPins_.erase(it);
    } catch (std::exception& e) {
        claimedPins_.erase(it);
        throw std::runtime_error(std::format("Could not release pin {}, exception: {}", pin, e.what()));
    }
}

void GpioManager::releaseAllPins() {
    std::vector<unsigned int> pins;
    pins.reserve(claimedPins_.size());

    for (const auto& [pin, claimedPin] : claimedPins_) {
        pins.push_back(pin);
    }

    for (const auto& pin : pins) {
        try {
            releasePin(pin);
        } catch (const std::exception& e) {
            Logger::warning(std::format("Could not release pin {}, exception: {}", pin, e.what()));
        }
    }

    claimedPins_.clear();
}

bool GpioManager::write(unsigned int pin, PinState value) {
    auto* claimedPin = findPin(pin);
    if (claimedPin == nullptr) {
        return false;
    }

    if (claimedPin->direction != PinDirection::OUTPUT) {
        Logger::warning(std::format("Cannot write to pin {} because it is not an output", pin));
        return false;
    }

    try {
        claimedPin->request.set_value(pin, toGpiodValue(value));
        return true;
    } catch (...) {
        return false;
    }
}

std::optional<PinState> GpioManager::read(unsigned int pin) {
    auto* claimedPin = findPin(pin);
    if (claimedPin == nullptr) {
        return std::nullopt;
    }

    if (claimedPin->direction != PinDirection::INPUT) {
        return std::nullopt;
    }

    try {
        const auto rawValue = claimedPin->request.get_value(pin);
        return fromGpiodValue(rawValue);
    } catch (...) {
        return std::nullopt;
    }
}

const std::string& GpioManager::chipPath() const {
    return chipPath_;
}

std::size_t GpioManager::claimedPinCount() const {
    return claimedPins_.size();
}

bool GpioManager::isClaimed(const unsigned int pin) const {
    return claimedPins_.contains(pin);
}

std::optional<PinDirection> GpioManager::getPinDirection(unsigned int pin) {
    auto* claimedPin = findPin(pin);
    if (claimedPin == nullptr) {
        return std::nullopt;
    }
    return claimedPin->direction;
}

// const GpioManager::ClaimedPin *GpioManager::findPin(unsigned int pin) const {
//     const auto it = claimedPins_.find(pin);
//     if (it == claimedPins_.end()) {
//         return nullptr;
//     }
//     return &it->second;
// }

GpioManager::ClaimedPin * GpioManager::findPin(unsigned int pin) {
    const auto it = claimedPins_.find(pin);
    if (it == claimedPins_.end()) {
        return nullptr;
    }
    return &it->second;
}
