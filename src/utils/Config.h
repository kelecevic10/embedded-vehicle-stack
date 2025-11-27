//
// Created by nemanja on 11/26/25.
//

#ifndef ECU_CAR_CONFIG_H
#define ECU_CAR_CONFIG_H
#include <string>


class Config {
public:
    Config();
    Config(const int, const bool, const std::string);

private:
    int tcpPort;
    bool simulationMode;
    std::string logLevel;
};


#endif //ECU_CAR_CONFIG_H