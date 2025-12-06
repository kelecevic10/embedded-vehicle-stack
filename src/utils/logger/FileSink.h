//
// Created by nemanja on 12/5/25.
//

#ifndef EMBEDDED_VEHICLE_STACK_FILESINK_H
#define EMBEDDED_VEHICLE_STACK_FILESINK_H

#include <fstream>
#include "Sink.h"

class FileSink : public Sink {
public:
    FileSink(const std::string& file_name);
    void writeLog(std::string_view formatted_message) override;

private:
    std::ofstream file;
};

#endif //EMBEDDED_VEHICLE_STACK_FILESINK_H