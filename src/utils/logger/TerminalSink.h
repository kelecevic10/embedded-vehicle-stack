//
// Created by nemanja on 12/5/25.
//

#ifndef EMBEDDED_VEHICLE_STACK_TERMINALSINK_H
#define EMBEDDED_VEHICLE_STACK_TERMINALSINK_H

#include <string_view>
#include "Sink.h"


class TerminalSink : public Sink {
public:
    void writeLog(std::string_view formatted_message) override;
};


#endif //EMBEDDED_VEHICLE_STACK_TERMINALSINK_H