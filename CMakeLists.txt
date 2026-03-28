cmake_minimum_required(VERSION 3.22)
project(embedded-vehicle-stack)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# add source dir
include_directories(src)

# recursively collect all .cpp files inside src/
file (GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
    "src/*.cpp"
)

add_executable(embedded-vehicle-stack ${SRC_FILES})