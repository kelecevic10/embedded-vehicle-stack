//
// Created by nemanja on 12/3/25.
//

#ifndef EMBEDDED_VEHICLE_STACK_LOGGERSINKS_H
#define EMBEDDED_VEHICLE_STACK_LOGGERSINKS_H
#include <fstream>
#include <string_view>


class Sink {
public:
    virtual ~Sink() = default;
    virtual void writeLog (std::string_view formatted_mess) = 0;
};


#endif //EMBEDDED_VEHICLE_STACK_LOGGERSINKS_H