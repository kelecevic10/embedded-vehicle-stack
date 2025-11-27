//
// Created by nemanja on 11/26/25.
//

#include "Config.h"

Config::Config(): tcpPort(5000), simulationMode(true), logLevel("INFO") {};

Config::Config(const int tcpPort, const bool simulationMode, const std::string& logLevel):
    tcpPort(tcpPort), simulationMode(simulationMode), logLevel(logLevel)
{};
