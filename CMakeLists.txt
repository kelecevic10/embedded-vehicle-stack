cmake_minimum_required(VERSION 4.0)
project(embedded-vehicle-stack)

set(CMAKE_CXX_STANDARD 23)

add_executable(embedded-vehicle-stack
        src/main.cpp
        src/core/ControlSystem.h
        src/core/ControlSystem.cpp
)
