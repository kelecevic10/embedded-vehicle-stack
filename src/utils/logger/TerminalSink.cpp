//
// Created by nemanja on 12/5/25.
//

#include "TerminalSink.h"
#include <iostream>

void TerminalSink::writeLog(std::string_view formatted_message) {
    std::cout << formatted_message << '\n';
}
